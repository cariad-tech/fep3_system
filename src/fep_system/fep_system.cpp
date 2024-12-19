/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <boost/asio.hpp>
#include <boost/algorithm/string/join.hpp>

#include <boost/range.hpp>
#include <fep_system/fep_system.h>
#include <service_bus_wrapper.h>
#include "system_logger.h"
#include <map>
#include <mutex>
#include <thread>
#include <algorithm>
#include <iterator>
#include <functional>
#include <numeric>
#include <boost/bimap.hpp>
#include <boost/assign.hpp>

#include "system_discovery_helper.h"
#include "participant_health_aggregator.h"
#include "fep_system_state_transition.h"

#include "state_transition/fep_participant_proxy_state_transition.h"
#include "fep_state_transition_controller.h"
#include "rpc_services/participant_statemachine_proxy.hpp"
#include "fep_system_state_transition_error_handling.h"
#include "shutdown_helper.h"
#include "base/service_bus_helper/include/participant_shutdown_listener.h"

#include <fep3/components/clock/clock_service_intf.h>
#include <fep3/components/clock_sync/clock_sync_service_intf.h>
#include <fep3/components/scheduler/scheduler_service_intf.h>
#include <fep3/base/properties/property_type.h>
#include <fep3/base/properties/properties.h>

const uint8_t pool_size_for_parallel_ops = 6;

using namespace a_util::strings;

namespace
{

    /**
     * @brief throws an exception if the given RPCClient does not wrap a valid RPC interface
     * 
     * @tparam RPCInterface Type defined with `#FEP_RPC_IID`
     * @param rpc_client Checked for validitiy
     */
    template<typename RPCInterface>
    inline void throwIfNotValid(fep3::RPCComponent<RPCInterface> rpc_client, const fep3::SystemLogger* system_logger, const std::string& participant_name)
    {
        if (!rpc_client) {
            FEP3_SYSTEM_LOG_AND_THROW(system_logger, fep3::arya::LoggerSeverity::fatal,
                a_util::strings::format("Participant %s is unreachable - RPC communication to retrieve RPC service '%s' with '%s' failed", 
                    participant_name.c_str(), RPCInterface::getRPCDefaultName(), RPCInterface::getRPCIID()));
        }
    }

    void replaceAll(std::string& orignal, const std::string& to_be_replaced, const std::string& replacement)
    {
        auto next_occurence = orignal.find(to_be_replaced);
        while (next_occurence != std::string::npos)
        {
            orignal.replace(next_occurence, to_be_replaced.length(), replacement);
            next_occurence = orignal.find(to_be_replaced);
        }
    }
}

namespace fep3
{
    static constexpr int min_timeout = 500;
    static constexpr int timeout_divident = 10;

    struct System::Implementation
    {
    public:
        using ParticipantProxyRange = std::vector<std::reference_wrapper<ParticipantProxy>>;

        explicit Implementation(const std::string& system_name)
            : Implementation(system_name, fep3::IServiceBus::ISystemAccess::_use_default_url)
        {
            //we need to use _use_default_url here => only this is to use the default
            //if we do not do this and use empty, discovery is switched off
        }

        explicit Implementation(const std::string& system_name,
                                const std::string& system_discovery_url)
            : _system_logger(std::make_shared<SystemLogger>(system_name)),
              _system_name(system_name),
              _system_discovery_url(system_discovery_url),
              _service_bus_wrapper(getServiceBusWrapper()),
              _execution_config{}
        {
            _service_bus_wrapper.createOrGetServiceBusConnection(system_name, system_discovery_url);
            _participant_logger->initRPCService(_system_name, _system_logger);

            initParticipantShutdownListener();
        }

        Implementation(const Implementation& other) = delete;
        Implementation& operator=(const Implementation& other) = delete;

        // why move constructor is not called?
        Implementation& operator=(Implementation&& other)
        {
            _system_name = std::move(other._system_name);
            _system_discovery_url = std::move(other._system_discovery_url);
            std::scoped_lock lock(other._participants._recursive_mutex, _participants._recursive_mutex);
            _participants._proxies = std::move(other._participants._proxies);

            _participant_logger = std::move(other._participant_logger);
            _system_logger = std::move(other._system_logger);
            _service_bus_wrapper = other._service_bus_wrapper;
            return *this;
        }

        ~Implementation()
        {
            if (auto system_access = _service_bus_wrapper.createOrGetServiceBusConnection(_system_name, _system_discovery_url)->getSystemAccessCatelyn(_system_name); system_access)
            {
                system_access->deregisterUpdateEventSink(_participant_shutdown_listener.get());
            }
            else {
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::fatal,
                    a_util::strings::format("Service bus returned nullptr when accessing system %s at %s. It is not possible to deregister event sink: 'participant shutdown listener'.", _system_name.c_str(), _system_discovery_url.c_str()));
            }
            clear();
        }

        void participantShutdownNotification(const std::string& participant_name) {
            
            _system_logger->log(LoggerSeverity::debug, "Received byebye event from " + participant_name);

            // participant is not reachable anymore. 
            // Set participant proxy not reachable status flag to prevent further RPC calls in d'tor. 
            std::scoped_lock lock(_participants._recursive_mutex);
            (void)std::find_if(_participants._proxies.begin(), _participants._proxies.end(), [&participant_name](const ParticipantProxy& proxy) {
                    if (proxy.getName() == participant_name) {
                        proxy.setNotReachable();
                        return true;
                    }
                    return false;
                });
            remove(participant_name);
        }

        void initParticipantShutdownListener()
        {
            _participant_shutdown_listener = std::make_unique<ParticipantShutdownListener>(
                _system_name, 
                [this](const std::string& participant_name)
                {
                    participantShutdownNotification(participant_name);
                });

            
            auto system_access = _service_bus_wrapper.createOrGetServiceBusConnection(_system_name, _system_discovery_url)->getSystemAccessCatelyn(_system_name);

            if (!system_access)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                    a_util::strings::format("No system connection to %s at %s possible", _system_name.c_str(), _system_discovery_url.c_str()));
            }
            else
            {
                system_access->registerUpdateEventSink(_participant_shutdown_listener.get());
            }
        }

        ExecutionTimer getExecutionTimer(std::chrono::milliseconds timeout,
                                         const StateInfo& state_info)
        {
            std::function<void()> warning_function = [&_system_logger = _system_logger,
                                                      timeout,
                                                      state_name = state_info.state_transition]() {
                FEP3_SYSTEM_LOG(_system_logger,
                                LoggerSeverity::warning,
                                a_util::strings::format(
                                    "Timeout of %lld ms exceeded for function call to state %s"
                                    "... Cannot interrupt the function call.",
                                    timeout.count(),
                                    state_name.c_str()));
            };

            return ExecutionTimer{timeout, warning_function};
        }

        std::vector<std::string> mapToStringVec() const
        {
            std::vector<std::string> participants;
            std::scoped_lock lock(_participants._recursive_mutex);
            for (const auto& p : _participants._proxies)
            {
                participants.push_back(p.getName());
            }
            return participants;
        }

        std::vector<ParticipantProxy> mapToProxyVec() const
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            return _participants._proxies;
        }

        template <typename PrioFunctionType, typename ExecPolicyType>
        bool runStateTransition(
            void (fep3::rpc::IRPCParticipantStateMachine::*transition_function)(),
            StateInfo info,
            SystemStateTransitionPrioSorting sorting,
            PrioFunctionType prio_function,
            ExecPolicyType&& policy,
            ParticipantProxyRange& participants_to_transition)
        {
            fep3::SystemStateTransition state_transition(
                prio_function, sorting, ThrowOnErrorPolicy(), std::forward<ExecPolicyType>(policy));

            return state_transition.execute(
                participants_to_transition,
                ProxyStateTransition(transition_function, _system_logger, info, _system_name),
                info);
        }

        ParticipantProxyRange getParticipantProxyRange()
        {
            ParticipantProxyRange range;
            std::scoped_lock lock(_participants._recursive_mutex);
            if (_participants._proxies.empty()) {
                FEP3_SYSTEM_LOG(_system_logger,
                                LoggerSeverity::warning,
                                "No participants within the current system");
                return range;
            }

            std::transform(_participants._proxies.begin(),
                           _participants._proxies.end(),
                           std::back_insert_iterator(range),
                           [](ParticipantProxy& proxy) { return std::ref(proxy); });

            return range;
        }

        template <typename PrioFunctionType>
        bool runStateTransition(
            void (fep3::rpc::IRPCParticipantStateMachine::*transition_function)(),
            StateInfo info,
            SystemStateTransitionPrioSorting sorting,
            PrioFunctionType prio_function,
            std::optional<ParticipantProxyRange> participants_to_transition,
            std::chrono::milliseconds timeout)
        {
            auto execution_timer = getExecutionTimer(timeout, info);
            ParticipantProxyRange range;
            if (participants_to_transition.has_value()) {
                range = participants_to_transition.value();
            }
            else {
                range = getParticipantProxyRange();
            }

            switch (_execution_config._policy) {
            case fep3::System::InitStartExecutionPolicy::parallel:
                return runStateTransition(transition_function,
                                          info,
                                          sorting,
                                          prio_function,
                                          ParallelExecutionPolicy{_execution_config._thread_count,
                                                                  std::move(execution_timer)},
                                          range);
            case fep3::System::InitStartExecutionPolicy::sequential:
                return runStateTransition(transition_function,
                                          info,
                                          sorting,
                                          prio_function,
                                          SerialExecutionPolicy{std::move(execution_timer)},
                                          range);
            }
            return false;
        }

        void setSystemState(System::AggregatedState state,
                            std::chrono::milliseconds timeout,
                            ParticipantProxyRange& participant_proxy_range)
        {
            StateTransitionErrorHandling error_handling(_system_logger, getName());

            error_handling.throwOnInvalidTargetState(state, toString(state));
            using TransitionActionPointer = decltype(&System::Implementation::load);

            static constexpr auto shutdown = &System::Implementation::shutdown;
            static constexpr auto unload = &System::Implementation::unload;
            static constexpr auto load = &System::Implementation::load;
            static constexpr auto initialize = &System::Implementation::initialize;
            static constexpr auto deinitialize = &System::Implementation::deinitialize;
            static constexpr auto start = &System::Implementation::start;
            static constexpr auto stop = &System::Implementation::stop;
            static constexpr auto pause = &System::Implementation::pause;

            static const std::vector<std::vector<TransitionActionPointer>> transition_matrix = {
              //undefined unreachable     unloaded   loaded       initialized paused    running
                {nullptr,   nullptr,     nullptr,  nullptr,       nullptr,    nullptr,  nullptr}, // undefined 0
                {nullptr,   nullptr,     nullptr,  nullptr,       nullptr,    nullptr,  nullptr},   // unreachable 1
                {nullptr,   shutdown,     nullptr, load,          nullptr,    nullptr,  nullptr},     // unloaded 2
                {nullptr,   nullptr,     unload,   nullptr,       initialize, nullptr,  nullptr}, // loaded 3
                {nullptr,   nullptr,     nullptr,  deinitialize,  nullptr,    pause,    start},  // initialized 4
                {nullptr,   nullptr,     nullptr,  nullptr,       stop,       nullptr,  start},        // paused 5
                {nullptr,   nullptr,     nullptr,  nullptr,       stop,       pause,    nullptr}};       // running 6

            //object for logging an error in case of exception
            TransitionSuccess transition_success{getName(), toString(state), _system_logger};
            while (true) {
                TransitionController transition_controller;
                // we can save this assuming the participants changed their state
                auto part_states_map = getParticipantStates(timeout, participant_proxy_range);
                if (transition_controller.homogeneousTargetStateAchieved(part_states_map, state))
                    break;
                // check if we can perform a state transition with the current participants states
                error_handling.checkParticipantsBeforeTransition(part_states_map);

                // get the participant state in which the participants should be in order to
                // participate in this transition.
                System::AggregatedState state_to_transition_from =
                    transition_controller.participantStateToTrigger(part_states_map, state);
                // get the participants that we should perform a state transition on
                std::vector<std::reference_wrapper<ParticipantProxy>> parts =
                    transition_controller.getParticipantsInState(
                        part_states_map, state_to_transition_from, participant_proxy_range);
                // get the state in which the participants should transition
                SystemAggregatedState state_to_transition_to =
                    transition_controller.getNextParticipantsState(state_to_transition_from, state);
                // we should never transition to the state we are at.
                assert(state_to_transition_from != state);
                // get the function that performs the actual transition
                auto func = transition_matrix[state_to_transition_from][state_to_transition_to];
                assert(func);
                (this->*func)(timeout, parts);
                FEP3_SYSTEM_LOG(_system_logger,
                                LoggerSeverity::info,
                                a_util::strings::format("System %s successfully.",
                                                        toString(state_to_transition_to).c_str()));
                // after shutdown there is nothing to do and the participant_proxy_range
                // is invalid since some participants were thrown out
                if (state_to_transition_to == fep3::System::AggregatedState::unreachable) {
                    break;
                }
            }
            transition_success._success = true;
        }

        void load(std::chrono::milliseconds timeout,
                  std::optional<ParticipantProxyRange> participants_to_transition = std::nullopt)
        {
            runStateTransition(&fep3::rpc::IRPCParticipantStateMachine::load,
                               {"loaded"},
                               SystemStateTransitionPrioSorting::none,
                               noPrio,
                               participants_to_transition,
                               timeout);
        }

        void unload(std::chrono::milliseconds timeout,
                    std::optional<ParticipantProxyRange> participants_to_transition = std::nullopt)
        {
            runStateTransition(&fep3::rpc::IRPCParticipantStateMachine::unload,
                               {"unloaded"},
                               SystemStateTransitionPrioSorting::none,
                               noPrio,
                               participants_to_transition,
                               timeout);
        }

        void initialize(
            std::chrono::milliseconds timeout,
            std::optional<ParticipantProxyRange> participants_to_transition = std::nullopt)
        {
            runStateTransition(
                &fep3::rpc::IRPCParticipantStateMachine::initialize,
                {"initialized"},
                SystemStateTransitionPrioSorting::decreasing,
                [](const ParticipantProxy& proxy) { return proxy.getInitPriority(); },
                participants_to_transition,
                timeout);
        }

        void deinitialize(
            std::chrono::milliseconds timeout,
            std::optional<ParticipantProxyRange> participants_to_transition = std::nullopt)
        {
            runStateTransition(
                &fep3::rpc::IRPCParticipantStateMachine::deinitialize,
                {"deinitialized"},
                SystemStateTransitionPrioSorting::increasing,
                [](const ParticipantProxy& proxy) { return proxy.getInitPriority(); },
                participants_to_transition,
                timeout);
        }

        void start(std::chrono::milliseconds timeout,
                   std::optional<ParticipantProxyRange> participants_to_transition = std::nullopt)
        {
            runStateTransition(
                &fep3::rpc::IRPCParticipantStateMachine::start,
                {"started"},
                SystemStateTransitionPrioSorting::decreasing,
                [](const ParticipantProxy& proxy) { return proxy.getStartPriority(); },
                participants_to_transition,
                timeout);
        }

        void pause(std::chrono::milliseconds timeout,
                   std::optional<ParticipantProxyRange> participants_to_transition = std::nullopt)
        {
            runStateTransition(&fep3::rpc::IRPCParticipantStateMachine::pause,
                               {"paused"},
                               SystemStateTransitionPrioSorting::none,
                               noPrio,
                               participants_to_transition,
                               timeout);
        }
        void stop(std::chrono::milliseconds timeout,
                  std::optional<ParticipantProxyRange> participants_to_transition = std::nullopt)
        {
            runStateTransition(
                &fep3::rpc::IRPCParticipantStateMachine::stop,
                {"stopped"},
                SystemStateTransitionPrioSorting::increasing,
                [](const ParticipantProxy& proxy) { return proxy.getStartPriority(); },
                participants_to_transition,
                timeout);
        }

        void shutdown(
            std::chrono::milliseconds timeout,
            std::optional<ParticipantProxyRange> participants_to_transition = std::nullopt)
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            if (_participants._proxies.empty())
            {
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning, "No participants within the current system");
                return;
            }

            auto shutdown_function = [](std::reference_wrapper<ParticipantProxy>& proxy_ref) {
                auto& part = proxy_ref.get();

                 // very important to deregister any RPC communication before shutting down
                // pending or communication after shutdown will wait for timeout
                part.deregisterLogging();
                auto state_machine =
                    part.getRPCComponentProxyByIID<rpc::IRPCParticipantStateMachine>();
                if (state_machine) {
                    try {
                        state_machine->shutdown();
                    }
                    catch (const std::exception& ex) {
                        return CREATE_ERROR_DESCRIPTION(
                            ERR_FAILED,
                            " Shut down participant %s threw an exception: %s;",
                            part.getName().c_str(),
                            ex.what());

                    }
                }
                else {
                    return CREATE_ERROR_DESCRIPTION(
                        ERR_FAILED,
                        " Participant %s is unreachable - RPC communication to retrieve RPC "
                        "service with '%s' failed;",
                        part.getName().c_str(),
                        rpc::IRPCParticipantStateMachine::getRPCIID());
                }

                return fep3::Result{};
            };

            ParticipantProxyRange proxy_range;
            if (participants_to_transition)
            {
                proxy_range = participants_to_transition.value();
            }
            else {
                proxy_range = getParticipantProxyRange();
            }
            auto timer = getExecutionTimer(timeout, {"unreachable"});
            timer.start();
            shutdownParticipants(_participants._proxies, proxy_range, shutdown_function, _system_logger);
            timer.stop();
        }

        std::string getName()
        {
            return _system_name;
        }

        std::string getUrl()
        {
            return _system_discovery_url;
        }

        //system state is aggregated
        //timeout can be only used if the RPC will
        // support the changing of it or use for every single Request

        ParticipantStates getParticipantStates(
            std::chrono::milliseconds,
            const ParticipantProxyRange& participants)
        {
            ParticipantStates states;
            for (const auto& part_ref : participants)
            {
                auto& part = part_ref.get();
                RPCComponent<rpc::arya::IRPCParticipantInfo> part_info;
                RPCComponent<rpc::catelyn::IRPCParticipantStateMachine> state_machine;
                try
                {
                    part_info = part.getRPCComponentProxyByIID<rpc::arya::IRPCParticipantInfo>();
                    state_machine = part.getRPCComponentProxyByIID<rpc::catelyn::IRPCParticipantStateMachine>();
                }
                catch (std::exception& e)
                {
                    FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                        a_util::strings::format(" Participant %s is unreachable - RPC communication failed with exception: %s.", part.getName().c_str(), e.what()));
                    states[part.getName()] = fep3::SystemAggregatedState::unreachable;
                }

                if (state_machine)
                {
                    //the participant can not be connected ... maybe it was shutdown or whatever
                    states[part.getName()] = state_machine->getState();
                }
                else
                {
                    if (!part_info)
                    {
                        //the participant can not be connected ... maybe it was shutdown or whatever
                        states[part.getName()] = { rpc::catelyn::IRPCParticipantStateMachine::State::unreachable };

                        FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                            a_util::strings::format(" Participant %s is unreachable - RPC communication to retrieve RPC service with '%s' failed;", part.getName().c_str(), rpc::IRPCParticipantInfo::getRPCIID()));
                    }
                    else
                    {
                        //the participant has no state machine, this is ok
                        //... i.e. a recorder will have no states and a signal listener tool will have no states
                        states[part.getName()] = { rpc::catelyn::IRPCParticipantStateMachine::State::unreachable };

                        FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                            a_util::strings::format(" Participant %s is unreachable - RPC communication to retrieve RPC service with '%s' failed;", part.getName().c_str(), rpc::IRPCParticipantStateMachine::getRPCIID()));
                    }
                }
            }
            return std::move(states);
        }

        // Returns the lowest state and information whether all states are homogeneous.
        static System::State getAggregatedState(const ParticipantStates& states)
        {
            TransitionController transition_controller;
            SystemAggregatedState aggregated_state =
                transition_controller.getAggregatedState(states);

            // no participant has a statemachine, then we are also unreachable
            if (aggregated_state == rpc::catelyn::IRPCParticipantStateMachine::State::unreachable) {
                return {rpc::catelyn::IRPCParticipantStateMachine::State::unreachable};
            }
            else {
                return {
                    transition_controller.homogeneousTargetStateAchieved(states, aggregated_state),
                    aggregated_state};
            }
        }

        System::State getSystemState(std::chrono::milliseconds timeout)
        {
            return getAggregatedState(getParticipantStates(timeout, getParticipantProxyRange()));
        }

        void registerMonitoring(IEventMonitor* monitor)
        {
            // register first in case a warning has to be logged
            _participant_logger->registerMonitor(monitor);
            std::scoped_lock lock(_participants._recursive_mutex);
            for (const auto& participant : _participants._proxies)
            {
                if (!participant.loggingRegistered())
                {
                    FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                        a_util::strings::format("Participant %s has no registered logging interface.", participant.getName().c_str()));
                }
            }
        }

        void releaseMonitoring(IEventMonitor* monitor)
        {
            _participant_logger->releaseMonitor(monitor);
        }

        void setSeverityLevel(LoggerSeverity level)
        {
            _participant_logger->setSeverityLevel(level);
        }

        void registerSystemMonitoring(IEventMonitor* monitor)
        {
            _system_logger->registerMonitor(monitor);
        }

        void releaseSystemMonitoring(IEventMonitor* monitor)
        {
            _system_logger->releaseMonitor(monitor);
        }

        void setSystemSeverityLevel(LoggerSeverity level)
        {
            _system_logger->setSeverityLevel(level);
        }

        void clear()
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            _participants._proxies.clear();
        }

        void addAsync(const std::multimap<std::string, std::string>& participants, uint8_t pool_size)
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            // find if we have duplicates
            auto part_found = searchParticipant(participants, false);
            if (part_found)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                    a_util::strings::format("Try to add a participant with name %s which already exists.", part_found.getName().c_str()));
            }

            boost::asio::thread_pool pool(pool_size);
            //preallocate the vector
            _participants._proxies = std::vector<ParticipantProxy>(participants.size());
            uint32_t index = 0;

            for (auto& part_to_call : participants)
            {
                boost::asio::post(pool,
                    [this, index, part_to_call]()
                    {
                        // we can initialize safely in multi thread execution, each thread touches a different vector index
                        _participants._proxies[index] = ParticipantProxy(part_to_call.first,
                            part_to_call.second,
                            _system_name,
                            _system_discovery_url,
                            _participant_logger->getUrl(),
                            _system_logger,
                            PARTICIPANT_DEFAULT_TIMEOUT);
                    });
                ++index;
            }
            pool.join();
        }

        void add(const std::string& participant_name, const std::string& participant_url)
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            [[maybe_unused]] auto [_, participant] = getParticipant(participant_name);
            (void)_;//gcc7 needs this
            if (participant)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                    a_util::strings::format("Try to add a participant with name %s which already exists.", participant_name.c_str()));
            }
            _participants._proxies.push_back(ParticipantProxy(participant_name,
                participant_url,
                _system_name,
                _system_discovery_url,
                _participant_logger->getUrl(),
                _system_logger,
                PARTICIPANT_DEFAULT_TIMEOUT));
        }

        void remove(const std::string& participant_name)
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            auto found = _participants._proxies.begin();
            for (;
                found != _participants._proxies.end();
                ++found)
            {
                if (found->getName() == participant_name)
                {
                    break;
                }
            }
            if (found != _participants._proxies.end())
            {
                _participants._proxies.erase(found);
            }
        }

        ParticipantProxy getParticipant(const std::string& participant_name, bool throw_if_not_found) const
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            for (auto& part_found : _participants._proxies)
            {
                if (part_found.getName() == participant_name)
                {
                    return part_found;
                }
            }
            auto error_message = a_util::strings::format("No Participant with the name %s found", participant_name.c_str());
            if (throw_if_not_found)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal, error_message);
            }
            else
            {
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning, error_message);
            }
            return {};
        }

        std::pair<fep3::Result, ParticipantProxy*> getParticipant(
            const std::string& participant_name)
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            for (auto& part_found: _participants._proxies) {
                if (part_found.getName() == participant_name) {
                    return {fep3::Result{}, &part_found};
                }
            }
            return {CREATE_ERROR_DESCRIPTION(fep3::ERR_NOT_FOUND,
                                             "No Participant with the name %s found",
                                             participant_name.c_str()),
                    nullptr};
        }

        ParticipantProxy searchParticipant(const std::multimap<std::string, std::string>& participants, bool throw_if_not_found) const
        {
            auto particpant_names = boost::adaptors::keys(participants);
            std::scoped_lock lock(_participants._recursive_mutex);
            auto part_found = std::search(
                _participants._proxies.begin(),
                _participants._proxies.end(),
                particpant_names.begin(),
                particpant_names.end(),
                [](const ParticipantProxy & part_proxy, const std::string & part_name)
                {
                    return part_proxy.getName() == part_name;
                });

            if (part_found == _participants._proxies.end())
            {
                if (throw_if_not_found)
                {
                    FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                        a_util::strings::format("No Participant with any of the names %s, was found", boost::algorithm::join(particpant_names, " ").c_str())); 
                }
                else
                {
                    return {};
                }
            }
            else
            {
                return *part_found;
            }
        }


        std::vector<ParticipantProxy> getParticipants() const
        {
            return mapToProxyVec();
        }

        // common parts for setParticipantProperty and getParticipantProperty
        std::pair<std::shared_ptr<IProperties>, std::string> getParticipantProperties(const std::string& participant_name,
            const std::string& property_path,
            const ParticipantProxy& part) const
        {
            if (part) {
                // split property_path in property_node and property_name
                auto pos_name = property_path.find_last_of("/");
                auto property_node = property_path.substr(0, pos_name);
                auto property_name = (pos_name == std::string::npos) ? "" : property_path.substr(pos_name + 1);

                // get properties of participant
                using IRPCConfiguration = fep3::rpc::IRPCConfiguration;
                auto config_rpc_client = part.getRPCComponentProxyByIID<IRPCConfiguration>();
                throwIfNotValid<IRPCConfiguration>(config_rpc_client, _system_logger.get(), participant_name);

                auto props = config_rpc_client->getProperties(property_node);
                if (!props) {
                    FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                        a_util::strings::format("Participant %s within system %s cannot access property node %s for property %s", participant_name.c_str(), _system_name.c_str(), property_node.c_str(), property_name.c_str())); 
                }
                return std::make_pair(props, property_name);
            }
            else {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                    a_util::strings::format("Participant %s within system %s not found to configure %s", participant_name.c_str(), _system_name.c_str(), property_path.c_str()));
            }
        }

        void setParticipantProperty(const std::string& participant_name,
            const std::string& property_path,
            const std::string& property_value,
            const bool throw_if_not_found) const
        {
            std::string property_path_normalized = property_path;

            replaceAll(property_path_normalized, ".", "/");

            ParticipantProxy part = getParticipant(participant_name, throw_if_not_found);
            auto [props, property_name] = getParticipantProperties(participant_name, property_path_normalized, part);

            if (!props->setProperty(property_name, property_value, props->getPropertyType(property_name))) 
            {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                    a_util::strings::format("Property %s could not be set for the following participant: %s", property_path_normalized.c_str(), part.getName().c_str()));
            }
        }

        std::string getParticipantProperty(const std::string& participant_name,
                                           const std::string& property_path) const
        {
            std::string property_path_normalized = property_path;
            replaceAll(property_path_normalized, ".", "/");
            ParticipantProxy part = getParticipant(participant_name, true);
            auto [props, property_name] = getParticipantProperties(participant_name, property_path_normalized, part);
            return props->getProperty(property_name);
        }

        SystemAggregatedState getParticipantState(const std::string& participant_name)
        {
            ParticipantProxy participant(getParticipant(participant_name, false));
            if (!participant)
            {
                return fep3::SystemAggregatedState::unreachable;
            }
            else
            {
                ParticipantStates states = getParticipantStates(std::chrono::milliseconds(0),
                                                                ParticipantProxyRange{std::ref(participant)});
                return states.empty() ? SystemAggregatedState::unreachable : states[participant.getName()];
            }
        }

        ParticipantStates getParticipantStates(std::chrono::milliseconds timeout)
        {
            return getParticipantStates(timeout, getParticipantProxyRange());
        }

        void setParticipantState(const std::string& participant_name, const SystemAggregatedState participant_state)
        {
            [[maybe_unused]] auto [res, participant_ptr] = getParticipant(participant_name);

            if (res) {
                ParticipantProxy& participant = *participant_ptr;
                ParticipantProxyRange participant_as_range{std::reference_wrapper(participant)};
                setSystemState(participant_state,
                               FEP_SYSTEM_TRANSITION_TIMEOUT, participant_as_range);
            }
            else {
                throw std::runtime_error(res.getDescription());
            }
        }

        void setPropertyValueToAll(const std::string& node,
            const std::string& property_name,
            const std::string& value,
            const std::string& type,
            const std::string& except_participant = std::string(),
            const bool throw_on_failure = true) const
        {
            std::string property_normalized = property_name;
            replaceAll(property_normalized, ".", "/");
            auto failing_participants = std::vector<std::string>();

            for (const ParticipantProxy& participant : getParticipants())
            {
                if (!except_participant.empty())
                {
                    if (participant.getName() == except_participant)
                    {
                        continue;
                    }
                }

                try
                {
                    using IRPCConfiguration = fep3::rpc::IRPCConfiguration;
                    auto config_rpc_client = participant.getRPCComponentProxyByIID<IRPCConfiguration>();
                    throwIfNotValid<IRPCConfiguration>(config_rpc_client, _system_logger.get(), participant.getName());

                    auto props = config_rpc_client->getProperties(node);
                    if (props)
                    {
                        const auto success = props->setProperty(property_normalized, value, type);
                        if (!success)
                        {
                            failing_participants.push_back(participant.getName());
                        }
                    }
                }
                catch (const std::exception& /*exception*/)
                {
                    failing_participants.push_back(participant.getName());
                }
            }

            if (!failing_participants.empty())
            {
                const auto participants = a_util::strings::join(failing_participants, ", ");
                const auto message = "Property " + property_normalized + " could not be set for the following participants: " + participants;

                if (throw_on_failure)
                {
                    FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal, message);
                }
                else 
                {
                    FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning, message);
                }
            }
        }

        void configureTiming(const std::string& master_clock_name, const std::string& slave_clock_name,
            const std::string& scheduler, const std::string& master_element_id, const std::string& master_time_stepsize,
            const std::string& master_time_factor, const std::string& slave_sync_cycle_time) const
        {
            setPropertyValueToAll("/",
                FEP3_CLOCKSYNC_SERVICE_CONFIG_TIMING_MASTER,
                master_element_id, fep3::base::PropertyType<std::string>::getTypeName());
            setPropertyValueToAll("/",
                FEP3_SCHEDULER_SERVICE_SCHEDULER,
                scheduler, fep3::base::PropertyType<std::string>::getTypeName());

            if (!master_element_id.empty())
            {
                setPropertyValueToAll("/",
                    FEP3_CLOCK_SERVICE_MAIN_CLOCK,
                    slave_clock_name, fep3::base::PropertyType<std::string>::getTypeName(), master_element_id);
                setParticipantProperty(master_element_id,
                    "/" FEP3_CLOCK_SERVICE_MAIN_CLOCK,
                    master_clock_name, false);
                if (!master_time_factor.empty())
                {
                    setParticipantProperty(master_element_id,
                        "/" FEP3_CLOCK_SERVICE_CLOCK_SIM_TIME_TIME_FACTOR,
                        master_time_factor, false);
                }
                if (!master_time_stepsize.empty())
                {
                    setParticipantProperty(master_element_id,
                        "/" FEP3_CLOCK_SERVICE_CLOCK_SIM_TIME_STEP_SIZE,
                        master_time_stepsize, false);
                }
                if (!slave_sync_cycle_time.empty())
                {
                    setPropertyValueToAll("/",
                        FEP3_CLOCKSYNC_SERVICE_CONFIG_SLAVE_SYNC_CYCLE_TIME,
                        slave_sync_cycle_time, fep3::base::PropertyType<int64_t>::getTypeName(), master_element_id);
                }
            }
            else
            {
                setPropertyValueToAll("/",
                    FEP3_CLOCK_SERVICE_MAIN_CLOCK, slave_clock_name, fep3::base::PropertyType<std::string>::getTypeName());
            }
        }

        std::vector<std::string> getCurrentTimingMasters() const
        {
            std::vector<std::string> timing_masters_found;
            for (ParticipantProxy participant : getParticipants())
            {
                using IRPCConfiguration = fep3::rpc::IRPCConfiguration;
                auto config_rpc_client = participant.getRPCComponentProxyByIID<IRPCConfiguration>();
                throwIfNotValid<IRPCConfiguration>(config_rpc_client, _system_logger.get(), participant.getName());

                auto props = config_rpc_client->getProperties(FEP3_CLOCKSYNC_SERVICE_CONFIG);
                if (props)
                {
                    auto master_found = props->getProperty(FEP3_TIMING_MASTER_PROPERTY);
                    if (!master_found.empty())
                    {
                        auto found_it = std::find(timing_masters_found.cbegin(), timing_masters_found.cend(), master_found);
                        if (found_it != timing_masters_found.cend())
                        {
                            //is already in list
                        }
                        else
                        {
                            timing_masters_found.push_back(master_found);
                        }
                    }
                }
            }
            return timing_masters_found;
        }

        std::map<std::string, std::unique_ptr<IProperties>> getTimingProperties() const
        {
            std::map<std::string, std::unique_ptr<IProperties>> timing_properties;
            for (const ParticipantProxy& participant : getParticipants())
            {
                auto iterator_success = timing_properties.emplace(participant.getName(),
                    std::unique_ptr<IProperties>(new base::Properties<IProperties>()));
                if (!iterator_success.second)
                {
                    FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                        a_util::strings::format("Multiple Participants with the name %s found", participant.getName().c_str())); 
                }
                IProperties& participant_properties = *iterator_success.first->second;
                auto set_if_present = [&participant_properties](const fep3::arya::IProperties& properties, const std::string& config_name) {
                    auto value = properties.getProperty(config_name);
                    if (value.empty())
                    {
                        return false;
                    }
                    return participant_properties.setProperty(config_name, value, properties.getPropertyType(config_name));
                };

                using IRPCConfiguration = fep3::rpc::IRPCConfiguration;
                auto config_rpc_client = participant.getRPCComponentProxyByIID<IRPCConfiguration>();
                throwIfNotValid<IRPCConfiguration>(config_rpc_client, _system_logger.get(), participant.getName());

                auto clockservice_props = config_rpc_client->getProperties(FEP3_CLOCK_SERVICE_CONFIG);
                if (clockservice_props)
                {
                    set_if_present(*clockservice_props, FEP3_MAIN_CLOCK_PROPERTY);
                }
                auto clocksync_props = config_rpc_client->getProperties(FEP3_CLOCKSYNC_SERVICE_CONFIG);
                if (clocksync_props)
                {
                    if (set_if_present(*clocksync_props, FEP3_TIMING_MASTER_PROPERTY))
                    {
                        if (clockservice_props)
                        {
                            set_if_present(*clockservice_props, FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY);
                            set_if_present(*clockservice_props, FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY);
                            set_if_present(*clockservice_props, FEP3_TIME_UPDATE_TIMEOUT_PROPERTY);
                        }
                        set_if_present(*clocksync_props, FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY);
                    }

                }
                auto scheduler_props = config_rpc_client->getProperties(FEP3_SCHEDULER_SERVICE_CONFIG);
                if (scheduler_props)
                {
                    set_if_present(*scheduler_props, FEP3_SCHEDULER_PROPERTY);
                }
            }
            return timing_properties;
        }

        void setInitAndStartPolicy(ExecutionConfig execution_config)
        {
            if (execution_config._thread_count > 0)
            {
                _execution_config = execution_config;
            }
            else
            {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                    "Thread count with value 0 is not valid");
            }
        }

        ExecutionConfig getInitAndStartPolicy() const
        {
            return _execution_config;
        }

        std::chrono::milliseconds getHeartbeatInterval(const std::string& participant)
        {
            auto part = getParticipant(participant, false);
            if (part)
            {
                auto http_server_rpc = part.getRPCComponentProxyByIID<fep3::rpc::IRPCHttpServer>();

                throwIfNotValid<fep3::rpc::IRPCHttpServer>(http_server_rpc, _system_logger.get(), participant);
                auto interval = http_server_rpc->getHeartbeatInterval();
                return interval;
            }
            else
            {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                    a_util::strings::format("Participant %s within system %s not found", participant.c_str(), _system_name.c_str()));
            }
        }

        void setHeartbeatInterval(const std::string& participant, const std::chrono::milliseconds interval_ms)
        {
            auto part = getParticipant(participant, false);
            if (part)
            {
                auto http_server_rpc = part.getRPCComponentProxyByIID<fep3::rpc::IRPCHttpServer>();

                throwIfNotValid<fep3::rpc::IRPCHttpServer>(http_server_rpc, _system_logger.get(), participant);
                http_server_rpc->setHeartbeatInterval(interval_ms);
            }
            else
            {
                FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                    a_util::strings::format("Participant %s within system %s not found", participant.c_str(), _system_name.c_str()));
            }
        }

        void setHeartbeatInterval(const std::vector<std::string>& participants, const std::chrono::milliseconds interval_ms)
        {
            if (!participants.empty()) {
                for (const auto& part_name : participants)
                {
                    setHeartbeatInterval(part_name, interval_ms);
                }
            }
            else {
                for (const ParticipantProxy& participant : getParticipants())
                {
                    setHeartbeatInterval(participant.getName(), interval_ms);
                }
            }
        }

        std::map<std::string, ParticipantHealth> getParticipantsHealth()
        {
            ParticipantHealthStateAggregator health_state_aggregator(_liveliness_timeout);
            std::scoped_lock lock(_participants._recursive_mutex);
            for (const auto& participant : _participants._proxies)
            {
                health_state_aggregator.setParticipantHealth(participant.getName(), std::move(participant.getParticipantHealth()));
            }

            return health_state_aggregator.getParticipantsHealth(std::chrono::steady_clock::now());
        }

        void setLivelinessTimeout(fep3::Timestamp liveliness_timeout)
        {
            _liveliness_timeout = std::move(liveliness_timeout);
        }

        fep3::Timestamp getLivelinessTimeout()
        {
            return _liveliness_timeout;
        }

        void setHealthListenerRunningStatus(bool running)
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            for (auto& participant : _participants._proxies)
            {
                participant.setHealthListenerRunningStatus(running);
            }
        }

        std::pair<bool, bool> getHealthListenerRunningStatus() const
        {
            std::scoped_lock lock(_participants._recursive_mutex);
            std::vector<bool> participant_health_listener_running(_participants._proxies.size());

            std::transform(_participants._proxies.begin(), _participants._proxies.end(), participant_health_listener_running.begin(), [](const auto& participant) {return participant.getHealthListenerRunningStatus(); });

            int sum = std::accumulate(std::begin(participant_health_listener_running), std::end(participant_health_listener_running), 0);

            if (sum == static_cast<int>(participant_health_listener_running.size()))
            {
                return { true, true };
            }
            else if (sum == 0)
            {
                return { true, false };
            }
            else
            {
                return{ false, false };
            }
        }

        struct ParticipantProxies {
            // lock to protect list of proxies against access from service bus thread and api calls
            mutable std::recursive_mutex _recursive_mutex;
            std::vector<ParticipantProxy> _proxies;
        };
        ParticipantProxies _participants;
        std::shared_ptr<SystemLogger> _system_logger;
        std::shared_ptr<RemoteLogForwarder> _participant_logger = std::make_shared<RemoteLogForwarder>();
        std::string _system_name;
        std::string _system_discovery_url;
        ServiceBusWrapper _service_bus_wrapper;
        ExecutionConfig _execution_config;
        fep3::Timestamp _liveliness_timeout = std::chrono::nanoseconds(std::chrono::seconds(20));
        std::unique_ptr<ParticipantShutdownListener> _participant_shutdown_listener;
    };

    System::System() : _impl(new Implementation(""))
    {
    }

    System::System(const std::string& system_name) : _impl(new Implementation(system_name))
    {

    }

    System::System(const std::string& system_name,
                   const std::string& system_discovery_url) : _impl(new Implementation(system_name,
                                                                                       system_discovery_url))
    {
    }

    System::System(const System& other) : _impl(new Implementation(other.getSystemName(),
          other.getSystemUrl()))
    {
        auto proxies = other.getParticipants();
        for (const auto& proxy : proxies)
        {
            _impl->add(proxy.getName(), proxy.getUrl());
            auto new_proxy = _impl->getParticipant(proxy.getName(), true);
            proxy.copyValuesTo(new_proxy);
        }
    }

    System& System::operator=(const System& other)
    {
        _impl = std::make_unique<Implementation>(other.getSystemName(), other.getSystemUrl());
        auto proxies = other.getParticipants();
        for (const auto& proxy : proxies)
        {
            _impl->add(proxy.getName(), proxy.getUrl());
            auto new_proxy = _impl->getParticipant(proxy.getName(), true);
            proxy.copyValuesTo(new_proxy);
        }
        return *this;
    }

    System::System(System&& other)
    {
        std::swap(_impl, other._impl);
    }

    System& System::operator=(System&& other)
    {
        std::swap(_impl, other._impl);
        return *this;
    }

    System::~System()
    {
    }

    void System::setSystemState(System::AggregatedState state, std::chrono::milliseconds timeout) const
    {
        auto participants = _impl->getParticipantProxyRange();
        _impl->setSystemState(state, timeout, participants);
    }

    void System::load(std::chrono::milliseconds timeout /*= FEP_SYSTEM_TRANSITION_TIME*/) const
    {
        _impl->load(timeout);
    }

    void System::unload(std::chrono::milliseconds timeout /*= FEP_SYSTEM_TRANSITION_TIME*/) const
    {
        _impl->unload(timeout);
    }

    void System::initialize(std::chrono::milliseconds timeout /*= FEP_SYSTEM_TRANSITION_TIME*/) const
    {
        _impl->initialize(timeout);
    }
    void System::deinitialize(std::chrono::milliseconds timeout /*= FEP_SYSTEM_TRANSITION_TIME*/) const
    {
        _impl->deinitialize(timeout);
    }

    void System::start(std::chrono::milliseconds timeout /*= FEP_SYSTEM_TRANSITION_TIME*/) const
    {
        _impl->start(timeout);
    }

    void System::stop(std::chrono::milliseconds timeout/*= FEP_SYSTEM_TRANSITION_TIME*/) const
    {
        _impl->stop(timeout);
    }

    void System::pause(std::chrono::milliseconds timeout /*= FEP_SYSTEM_TRANSITION_TIME*/) const
    {
        _impl->pause(timeout);
    }

    void System::shutdown(std::chrono::milliseconds timeout /*= FEP_SYSTEM_TRANSITION_TIME*/) const
    {
        _impl->shutdown(timeout);
    }


    void System::add(const std::string& participant, const std::string& participant_url)
    {
        _impl->add(participant, participant_url);
    }

    void System::add(const std::vector<std::string>& participants)
    {
        for (const auto& participant : participants)
        {
            _impl->add(participant, std::string());
        }
    }

    void System::add(const std::multimap<std::string, std::string>& participants)
    {
        for (const auto& participant : participants)
        {
            _impl->add(participant.first, participant.second);
        }
    }

    void System::addAsync(const std::multimap<std::string, std::string>& participants)
    {
        _impl->addAsync(participants, pool_size_for_parallel_ops);
    }

    void System::addAsync(const std::multimap<std::string, std::string>& participants, uint8_t pool_size)
    {
        _impl->addAsync(participants, pool_size);
    }

    void System::remove(const std::string& participant)
    {
        _impl->remove(participant);
    }

    void System::remove(const std::vector<std::string>& participants)
    {
        for (const auto& participant : participants)
        {
            _impl->remove(participant);
        }
    }

    void System::clear()
    {
        _impl->clear();
    }

    System::State System::getSystemState(std::chrono::milliseconds timeout /*= FEP_SYSTEM_DEFAULT_TIMEOUT_MS*/) const
    {
        return _impl->getSystemState(timeout);
    }

    std::string System::getSystemName() const
    {
        return _impl->getName();
    }

    std::string System::getSystemUrl() const
    {
        return _impl->getUrl();
    }

    ParticipantProxy System::getParticipant(const std::string& participant_name) const
    {
        return _impl->getParticipant(participant_name, true);
    }

    std::vector<ParticipantProxy> System::getParticipants() const
    {
        return _impl->getParticipants();
    }

    void System::setParticipantProperty(const std::string& participant_name,
        const std::string& property_path, const std::string& property_value) {
        _impl->setParticipantProperty(participant_name, property_path, property_value, true);
    }

    std::string System::getParticipantProperty(const std::string& participant_name,
        const std::string& property_path) const 
    {
        return _impl->getParticipantProperty(participant_name, property_path);
    }

    SystemAggregatedState System::getParticipantState(const std::string& participant_name) const
    {
        return _impl->getParticipantState(participant_name);
    }

    ParticipantStates System::getParticipantStates(std::chrono::milliseconds timeout) const
    {
        return _impl->getParticipantStates(timeout);
    }

    void System::setParticipantState(const std::string& participant_name, const SystemAggregatedState participant_state) const
    {
        _impl->setParticipantState(participant_name, participant_state);
    }

    void System::registerMonitoring(IEventMonitor& event_monitor)
    {
        _impl->registerMonitoring(&event_monitor);
    }

    void System::unregisterMonitoring(IEventMonitor& event_monitor)
    {
        _impl->releaseMonitoring(&event_monitor);
    }

    void System::setSeverityLevel(LoggerSeverity severity_level)
    {
        _impl->setSeverityLevel(severity_level);
    }

    void System::registerSystemMonitoring(IEventMonitor& event_monitor)
    {
        _impl->registerSystemMonitoring(&event_monitor);
    }

    void System::unregisterSystemMonitoring(IEventMonitor& event_monitor)
    {
        _impl->releaseSystemMonitoring(&event_monitor);
    }

    void System::setSystemSeverityLevel(LoggerSeverity severity_level)
    {
        _impl->setSystemSeverityLevel(severity_level);
    }

    void System::configureTiming(const std::string& master_clock_name, const std::string& slave_clock_name,
        const std::string& scheduler, const std::string& master_element_id, const std::string& master_time_stepsize,
        const std::string& master_time_factor, const std::string& slave_sync_cycle_time) const
    {
        _impl->configureTiming(master_clock_name, slave_clock_name, scheduler, master_element_id,
            master_time_stepsize, master_time_factor, slave_sync_cycle_time);
    }

    std::vector<std::string> System::getCurrentTimingMasters() const
    {
        return _impl->getCurrentTimingMasters();
    }

    std::map<std::string, std::unique_ptr<IProperties>> System::getTimingProperties() const
    {
        return _impl->getTimingProperties();
    }

    void System::configureTiming3NoMaster() const
    {
        _impl->configureTiming("", FEP3_CLOCK_LOCAL_SYSTEM_REAL_TIME,
            FEP3_SCHEDULER_CLOCK_BASED, "", "", "", "");
    }

    void System::configureTiming3ClockSyncOnlyInterpolation(const std::string& master_element_id, const std::string& slave_sync_cycle_time) const
    {
        _impl->configureTiming(FEP3_CLOCK_LOCAL_SYSTEM_REAL_TIME,
            FEP3_CLOCK_SLAVE_MASTER_ONDEMAND,
            FEP3_SCHEDULER_CLOCK_BASED,
            master_element_id, "", "", slave_sync_cycle_time);
    }
    void System::configureTiming3ClockSyncOnlyDiscrete(const std::string& master_element_id, const std::string& slave_sync_cycle_time) const
    {
        _impl->configureTiming(FEP3_CLOCK_LOCAL_SYSTEM_REAL_TIME,
            FEP3_CLOCK_SLAVE_MASTER_ONDEMAND_DISCRETE,
            FEP3_SCHEDULER_CLOCK_BASED,
            master_element_id, "", "", slave_sync_cycle_time);
    }
    void System::configureTiming3DiscreteSteps(const std::string& master_element_id, const std::string& master_time_stepsize, const std::string& master_time_factor) const
    {
        _impl->configureTiming(FEP3_CLOCK_LOCAL_SYSTEM_SIM_TIME,
            FEP3_CLOCK_SLAVE_MASTER_ONDEMAND_DISCRETE,
            FEP3_SCHEDULER_CLOCK_BASED,
            master_element_id, master_time_stepsize, master_time_factor, "");
    }

    void System::configureTiming3AFAP(const std::string& master_element_id, const std::string& master_time_stepsize) const
    {
        _impl->configureTiming(FEP3_CLOCK_LOCAL_SYSTEM_SIM_TIME,
            FEP3_CLOCK_SLAVE_MASTER_ONDEMAND_DISCRETE,
            FEP3_SCHEDULER_CLOCK_BASED,
            master_element_id, master_time_stepsize, "0.0", "");
    }

    void System::setInitAndStartPolicy(System::InitStartExecutionPolicy policy, uint8_t thread_count)
    {
        _impl->setInitAndStartPolicy(ExecutionConfig{ policy, thread_count });
    }

    std::pair<System::InitStartExecutionPolicy, uint8_t> System::getInitAndStartPolicy()
    {
        auto exec_policy =  _impl->getInitAndStartPolicy();
        return std::make_pair(exec_policy._policy, exec_policy._thread_count);
    }

    std::map<std::string, ParticipantHealth> System::getParticipantsHealth()
    {
        return _impl->getParticipantsHealth();
    }

    void System::setLivelinessTimeout(std::chrono::nanoseconds liveliness_timeout)
    {
        _impl->setLivelinessTimeout(std::move(liveliness_timeout));
    }

    std::chrono::nanoseconds System::getLivelinessTimeout() const
    {
       return _impl->getLivelinessTimeout();
    }

    void System::setHealthListenerRunningStatus(bool running)
    {
        _impl->setHealthListenerRunningStatus(running);
    }

    std::pair<bool, bool> System::getHealthListenerRunningStatus() const
    {
        return _impl->getHealthListenerRunningStatus();
    }

    void System::setHeartbeatInterval(const std::vector<std::string>& participants, const std::chrono::milliseconds interval_ms)
    {
        _impl->setHeartbeatInterval(participants, interval_ms);

    }

    std::chrono::milliseconds System::getHeartbeatInterval(const std::string& participant) const
    {
        return _impl->getHeartbeatInterval(participant);
    }

/**************************************************************
* discoveries
***************************************************************/
    template <typename ...Args>
    System discoverSystemByURLInternal(std::string name, std::string discover_url, Args&&...args)
    {
        ServiceBusWrapper _service_bus_wrapper = getServiceBusWrapper();
        fep3::IServiceBus* my_discovery_bus = _service_bus_wrapper.createOrGetServiceBusConnection(name,
            discover_url);
        if (my_discovery_bus)
        {
            //we check if that system access exists ... the service bus connection must create it for me
            auto sys_access = my_discovery_bus->getSystemAccessCatelyn(name);
            if (!sys_access)
            { 
                throw std::runtime_error("can not find a system access on service bus connection to system '"
                    + name + "' at url '" + discover_url + "'; " + std::to_string(__LINE__) + ", " + std::string(__FILE__) + ", " + std::string(A_UTIL_CURRENT_FUNCTION));
            }
           // auto participants = sys_access->discover(timeout);
            auto [error, participants_optional] = discoverSystemParticipants(*sys_access, std::forward<Args>(args)...);
            if (!participants_optional.has_value())
            {
                error.append("; " + std::to_string(__LINE__) + ", " + std::string(__FILE__) + ", " + std::string(A_UTIL_CURRENT_FUNCTION));
                throw std::runtime_error(error);
            }
            std::multimap<std::string, std::string> participants = participants_optional.value();

            System discovered_system(name, discover_url);
            discovered_system.addAsync(participants);
            return discovered_system;
        }
        throw std::runtime_error("can not create a service bus connection to system '"
            + name + "' at url '" + discover_url + "'; " + std::to_string(__LINE__) + ", " + std::string(__FILE__) + ", " + std::string(A_UTIL_CURRENT_FUNCTION));
    }


    System discoverSystem(std::string name, std::chrono::milliseconds timeout /*= FEP_SYSTEM_DISCOVER_TIME_MS*/)
    {
        return discoverSystemByURL(name, fep3::IServiceBus::ISystemAccess::_use_default_url, timeout);
    }

    System discoverSystem(std::string name,
        std::vector<std::string> participant_names,
        std::chrono::milliseconds timeout)
    {
        return discoverSystemByURL(name, fep3::IServiceBus::ISystemAccess::_use_default_url, std::move(participant_names), timeout);
    }

    System discoverSystem(std::string name,
        uint32_t participant_count,
        std::chrono::milliseconds timeout)
    {
        return discoverSystemByURL(name, fep3::IServiceBus::ISystemAccess::_use_default_url, participant_count, timeout);
    }

    System discoverSystemByURL(std::string name,
        std::string discover_url,
        std::vector<std::string> participant_names,
        std::chrono::milliseconds timeout)
    {
        return discoverSystemByURLInternal(name, discover_url, timeout, std::move(participant_names), false);
    }

    System discoverSystemByURL(std::string name,
        std::string discover_url,
        uint32_t participant_count,
        std::chrono::milliseconds timeout)
    {
       return discoverSystemByURLInternal(name, discover_url, timeout, participant_count);
    }

    System discoverSystemByURL(std::string name, std::string discover_url, std::chrono::milliseconds timeout)
    {
        return discoverSystemByURLInternal(name, discover_url, timeout);
    }

    template <typename ...Args>
    std::vector<System>discoverAllSystemsByURLInternal(std::string discover_url,
         Args&&...args)
    {
        // default discover is actually DDS with domain 0
        ServiceBusWrapper _service_bus_wrapper = getServiceBusWrapper();
        auto my_discovery_bus = _service_bus_wrapper.createOrGetServiceBusConnection(
            fep3::IServiceBus::ISystemAccess::_discover_all_systems,
            discover_url);

        if (my_discovery_bus)
        {
            //we check if that system access already exists
            auto sys_access = my_discovery_bus->getSystemAccessCatelyn(fep3::IServiceBus::ISystemAccess::_discover_all_systems);
            if (!sys_access)
            {
                throw std::runtime_error("can not create a system access on service bus connection to discover all systems at url '"
                    + discover_url + "'; " + std::to_string(__LINE__) + ", " + std::string(__FILE__) + ", " + std::string(A_UTIL_CURRENT_FUNCTION));
            }
            // auto all_participants = sys_access->discover(timeout);
            auto [error, participants_optional] = discoverSystemParticipants(*sys_access, std::forward<Args>(args)...);
            if (!participants_optional.has_value())
            {
                error.append("; " + std::to_string(__LINE__) + ", " + std::string(__FILE__) + ", " + std::string(A_UTIL_CURRENT_FUNCTION));
                throw std::runtime_error(error);
            }
            std::multimap<std::string, std::string> all_participants = participants_optional.value();

            if (all_participants.empty())
            {
                return {};
            }

            std::map<std::string, std::multimap<std::string, std::string>> systems_participants;
            for (auto& part : all_participants)
            {
                const std::string& participant_url = part.second;
                const auto opt_part_and_sys_name =  get_partictipant_and_system_name(part.first);
                if (opt_part_and_sys_name.has_value())
                {
                    const auto [participant_name, system_name] = opt_part_and_sys_name.value();
                    systems_participants[system_name].emplace(participant_name, participant_url);
                }
                else
                {
                    throw std::runtime_error("Parsing error while discoverAllSystems by URL " + discover_url + " .Expected a string like participant_name@system_name but got " + part.first +
                        "; " + std::to_string(__LINE__) + ", " + std::string(__FILE__) + ", " + std::string(A_UTIL_CURRENT_FUNCTION));
                }
            }

            // take the system names
            auto all_system_names = boost::adaptors::keys(systems_participants);
            // map that contains the discoverd system objects
            std::map<std::string, std::unique_ptr<System>> all_systems_map;
            // create system objects, only once, discarding the duplicates
            boost::range::transform(all_system_names,
                std::inserter(all_systems_map, all_systems_map.end()),
                [](const std::string& sys_name)
                {
                    return std::make_pair(sys_name, std::make_unique<System>(sys_name));
                });

            //split the total number of threads to each system
            const uint8_t pool_size = std::max(static_cast<uint8_t>(1),
                static_cast<uint8_t>(pool_size_for_parallel_ops / static_cast<uint8_t>(all_systems_map.size())));

            //add the participants to each system asynchronously
            boost::asio::thread_pool pool(all_systems_map.size());
            for (auto& system_and_participants : systems_participants)
            {
                boost::asio::post(pool,
                    [&, pool_size]()
                    {
                        all_systems_map.at(system_and_participants.first)->addAsync(system_and_participants.second, pool_size);
                    });
            }
            pool.join();

            std::vector<System> result_vector_system;
            for (auto& found_sys : all_systems_map)
            {
                result_vector_system.emplace_back(std::move(*found_sys.second.release()));
            }
            return result_vector_system;
        }
        throw std::runtime_error("can not create a service bus connection to discover all systems at url ''"
            + discover_url + "'; " + std::to_string(__LINE__) + ", " + std::string(__FILE__) + ", " + std::string(A_UTIL_CURRENT_FUNCTION));
    }

    std::vector<System> discoverAllSystems(std::chrono::milliseconds timeout /*= FEP_SYSTEM_DISCOVER_TIMEOUT*/)
    {
        return discoverAllSystemsByURL(fep3::IServiceBus::ISystemAccess::_use_default_url ,timeout);
    }

    std::vector<System> discoverAllSystems(
        uint32_t participant_count,
        std::chrono::milliseconds timeout)
    {
        return discoverAllSystemsByURL(fep3::IServiceBus::ISystemAccess::_use_default_url, participant_count, timeout);
    }

    std::vector<System> discoverAllSystemsByURL(std::string discover_url,
        uint32_t participant_count,
        std::chrono::milliseconds timeout)
    {
        return discoverAllSystemsByURLInternal(discover_url, timeout, participant_count);
    }

    std::vector<System>discoverAllSystemsByURL(std::string discover_url,
        std::chrono::milliseconds timeout /*= FEP_SYSTEM_DISCOVER_TIMEOUT*/)
    {
        return discoverAllSystemsByURLInternal(discover_url, timeout);
    }

    void preloadServiceBusPlugin()
    {
        getServiceBusWrapper();
    }

#pragma warning(push)
#pragma warning(disable: 4834 4996)

    namespace
    {
        using bimapSystemAggregatedState = boost::bimap<SystemAggregatedState, std::string>;
        const bimapSystemAggregatedState system_aggregated_state =
            boost::assign::list_of<bimapSystemAggregatedState::relation>
            (fep3::System::AggregatedState::undefined, "undefined")
            (fep3::System::AggregatedState::unreachable, "unreachable")
            (fep3::System::AggregatedState::unloaded, "unloaded")
            (fep3::System::AggregatedState::loaded, "loaded")
            (fep3::System::AggregatedState::initialized, "initialized")
            (fep3::System::AggregatedState::paused, "paused")
            (fep3::System::AggregatedState::running, "running");
    }

    SystemAggregatedState getSystemAggregatedStateFromString(const std::string& state_string)
    {
        auto it = system_aggregated_state.right.find(state_string);
        if (it != system_aggregated_state.right.end())
        {
            return it->second;
        }
        return fep3::SystemAggregatedState::unreachable;
    }

    std::string systemAggregatedStateToString(const fep3::System::AggregatedState state)
    {
        auto it = system_aggregated_state.left.find(state);
        if (it != system_aggregated_state.left.end())
        {
            return it->second;
        }
        return "NOT RESOLVABLE";
    }

#pragma warning(pop)

//namespace experimental
//{
//
//    struct System::Implementation
//    {
//    public:
//        Implementation() = default;
//        Implementation(const Implementation& other) = delete;
//        Implementation(Implementation&& other) = delete;
//        Implementation& operator=(const Implementation& other) = delete;
//        Implementation& operator=(Implementation&& other) = delete;
//        ~Implementation() = default;
//
//        experimental::HealthState getHealth(ParticipantProxy participant_proxy) const
//        {
//            const auto health_service = participant_proxy.getRPCComponentProxyByIID<experimental::IRPCHealthService>();
//            if (!health_service)
//            {
//                return HealthState::unknown;
//            }
//
//            return health_service->getHealth();
//        }
//
//        typedef std::map<std::string, experimental::HealthState> PartHealthStates;
//        PartHealthStates getParticipantHealthStates(std::vector<ParticipantProxy> participant_proxies) const
//        {
//            PartHealthStates participant_health_states{};
//
//            for (const auto& participant : participant_proxies)
//            {
//                const auto health_service =
//                    participant.getRPCComponentProxyByIID<experimental::IRPCHealthService>();
//                if (!health_service)
//                {
//                    participant_health_states[participant.getName()] = experimental::HealthState::unknown;
//                }
//                else
//                {
//                    participant_health_states[participant.getName()] = health_service->getHealth();
//                }
//            }
//
//            return participant_health_states;
//        }
//
//        static experimental::SystemHealthState getAggregatedHealthState(const PartHealthStates& states)
//        {
//            //we begin at the lowest value
//            experimental::SystemAggregatedHealthState aggregated_health_state = experimental::SystemAggregatedHealthState::ok;
//            //we have a look if at least one of it is set
//            bool first_set = false;
//            bool homogeneous_value = true;
//            for (const auto& item : states)
//            {
//                experimental::HealthState current_health_state = item.second;
//                //we were at least one time in this loop
//                if (current_health_state != experimental::SystemAggregatedHealthState::unknown)
//                {
//                    if (current_health_state > aggregated_health_state)
//                    {
//                        if (first_set)
//                        {
//                            //is not homogenous because some of the already checked states has a lower value
//                            homogeneous_value = false;
//                        }
//                        aggregated_health_state = current_health_state;
//                    }
//                    if (current_health_state < aggregated_health_state)
//                    {
//                        //is not homogenous because one of the already checked states has a higher value
//                        homogeneous_value = false;
//                    }
//                }
//                //this is to check if this loop was already entered before we change
//                first_set = true;
//            }
//            //no participant has a health state, then we are also unknown
//            if (!first_set)
//            {
//                return { true, experimental::HealthState::unknown };
//            }
//            else
//            {
//                return { homogeneous_value , aggregated_health_state };
//            }
//        }
//
//        experimental::SystemHealthState getSystemHealth(std::vector<ParticipantProxy> participant_proxies) const
//        {
//            return getAggregatedHealthState(getParticipantHealthStates(participant_proxies));
//        }
//
//        fep3::Result resetHealth(ParticipantProxy participant_proxy, const std::string& message)
//        {
//            const auto health_service = participant_proxy.getRPCComponentProxyByIID<experimental::IRPCHealthService>();
//            if (!health_service)
//            {
//                return fep3::Result{ ERR_NOT_FOUND,
//                            a_util::strings::format("Health service of participant '%s' is inaccessible.", participant_proxy.getName().c_str()).c_str(),
//                            0,
//                            "",
//                            ""};
//            }
//
//            return health_service->resetHealth(message);
//        }
//    };
//
//    System::System()
//        : fep3::System()
//        ,  _impl(std::make_unique<Implementation>())
//    {
//    }
//
//    System::System(const std::string& system_name)
//        : fep3::System(system_name)
//        ,  _impl(std::make_unique<Implementation>())
//    {
//    }
//
//    System::System(const std::string& system_name,
//                   const std::string& system_discovery_url)
//        : fep3::System(system_name, system_discovery_url)
//        ,  _impl(std::make_unique<Implementation>())
//    {
//    }
//
//    System::~System()
//    {
//    }
//
//    experimental::HealthState System::getHealth(const std::string& participant_name) const
//    {
//        return _impl->getHealth(getParticipant(participant_name));
//    }
//
//    experimental::SystemHealthState System::getSystemHealth() const
//    {
//        return _impl->getSystemHealth(getParticipants());
//    }
//
//    fep3::Result System::resetHealth(const std::string& participant_name, const std::string& message)
//    {
//        return _impl->resetHealth(getParticipant(participant_name), message);
//    }
//
//}

}
