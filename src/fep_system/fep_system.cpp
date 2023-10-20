/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2021 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */

#include "boost/asio.hpp"
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/algorithm/string/join.hpp>

#include <boost/range.hpp>
#include <fep_system/fep_system.h>
#include <a_util/system/system.h>
#include <service_bus_wrapper.h>
#include "a_util/process.h"
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

#include <fep3/components/clock/clock_service_intf.h>
#include <fep3/components/clock_sync/clock_sync_service_intf.h>
#include <fep3/components/scheduler/scheduler_service_intf.h>
#include <fep3/base/properties/property_type.h>
#include <fep3/base/properties/properties.h>

const uint8_t pool_size_for_parallel_ops = 6;

using namespace a_util::strings;

namespace
{
    struct ExecutionConfig
    {
        fep3::System::InitStartExecutionPolicy _policy = fep3::System::InitStartExecutionPolicy::parallel;
        uint8_t _thread_count = 4;
    };

    std::vector<std::string> getParticipantNamesByState(
        const fep3::ParticipantStates& participant_states,
        fep3::rpc::arya::IRPCParticipantStateMachine::State state)
    {
        std::vector<std::string> participant_names;

        for (const auto& participant_state : participant_states)
        {
            if (state == participant_state.second)
            {
                participant_names.emplace_back(participant_state.first);
            }
        }

        return participant_names;
    }

    std::vector<fep3::ParticipantProxy> getParticipantsByState(const std::vector<fep3::ParticipantProxy>& source_participants,
        const fep3::ParticipantStates& participant_states,
        fep3::rpc::arya::IRPCParticipantStateMachine::State state)
    {
        const auto participant_names = getParticipantNamesByState(participant_states, state);

        std::vector<fep3::ParticipantProxy> participants;

        for (const auto& participant : source_participants)
        {
            if (std::find(participant_names.begin(), participant_names.end(), participant.getName()) != participant_names.end())
            {
                participants.emplace_back(participant);
            }
        }

        return participants;
    }

    std::map<int32_t, std::vector<fep3::ParticipantProxy>> getParticipantsSortedbyStartPrio(
        const std::vector<fep3::ParticipantProxy>& participants)
    {
        std::map<int32_t, std::vector<fep3::ParticipantProxy>> participants_sorted_by_prio;
        for (const auto& part : participants)
        {
            auto prio = part.getStartPriority();
            participants_sorted_by_prio[prio].push_back(part);
        }
        return participants_sorted_by_prio;
    }

    std::map<int32_t, std::vector<fep3::ParticipantProxy>> getParticipantsSortedbyInitPrio(
    const std::vector<fep3::ParticipantProxy>& participants)
    {
        std::map<int32_t, std::vector<fep3::ParticipantProxy>> participants_sorted_by_prio;
        for (const auto& part : participants)
        {
            auto prio = part.getInitPriority();
            participants_sorted_by_prio[prio].push_back(part);
        }
        return participants_sorted_by_prio;
    }

    void for_each_ordered_reverse(std::map<int32_t, std::vector<fep3::ParticipantProxy>>& sorted_parts,
            const ExecutionConfig& execution_config,
            const std::function<void(fep3::ParticipantProxy&)>& call)
    {
        //reverse order of prio
        for (auto current_prio = sorted_parts.rbegin();
                current_prio != sorted_parts.rend();
                ++current_prio)
        {
            //normal order of parts having the same prio(unfortunately this is by name at the moment)
            auto& current_prio_parts = current_prio->second;

            switch (execution_config._policy)
            {
                case  fep3::System::InitStartExecutionPolicy::sequential:
                {
                    for (auto& part_to_call : current_prio_parts)
                    {
                        call(part_to_call);
                    }
                    break;
                }
                case  fep3::System::InitStartExecutionPolicy::parallel:
                {
                    boost::asio::thread_pool pool(execution_config._thread_count);

                    for (auto& part_to_call : current_prio_parts)
                    {
                        boost::asio::post(pool,
                            [&]()
                            {
                                call(part_to_call);
                            });
                    }
                    pool.join();
                    break;
                }
            }
        }
    }

    void for_each_ordered(std::map<int32_t, std::vector<fep3::ParticipantProxy>>& sorted_parts,
        const std::function<void(fep3::ParticipantProxy&)>& call)
    {
        //normal order of prio
        for (auto current_prio = sorted_parts.begin();
            current_prio != sorted_parts.end();
            ++current_prio)
        {
            //reverse order of parts having the same prio (unfortunatelly this is by name at the moment)
            auto& current_prio_parts = current_prio->second;
            for (auto part_to_call = current_prio_parts.rbegin();
                    part_to_call != current_prio_parts.rend();
                    ++part_to_call)
            {
                call(*part_to_call);
            }
        }
    }

    std::string toString(const fep3::rpc::ParticipantState& participant_state)
    {
        switch (participant_state)
        {
        case fep3::rpc::ParticipantState::unreachable:
            return "unreachable";
        case fep3::rpc::ParticipantState::unloaded:
            return "unloaded";
        case fep3::rpc::ParticipantState::loaded:
            return "loaded";
        case fep3::rpc::ParticipantState::initialized:
            return "initialized";
        case fep3::rpc::ParticipantState::running:
            return "running";
        case fep3::rpc::ParticipantState::paused:
            return "paused";
        default:
            return "undefined";
        }
    }

    /**
     * @brief throws an exception if the given RPCClient does not wrap a valid RPC interface
     * 
     * @tparam RPCInterface Type defined with `#FEP_RPC_IID`
     * @param rpc_client Checked for validitiy
     */
    template<typename RPCInterface>
    void throwIfNotValid(fep3::RPCComponent<RPCInterface> rpc_client)
    {
        if (!rpc_client) {
            throw std::runtime_error(format("Could not get RPC Client \"%s\" with RPC IID %s", RPCInterface::getRPCDefaultName(), RPCInterface::getRPCIID()));
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
        explicit Implementation(const std::string& system_name)
            : Implementation(system_name, fep3::IServiceBus::ISystemAccess::_use_default_url)
        {
            //we need to use _use_default_url here => only this is to use the default
            //if we do not do this and use empty, discovery is switched off
        }

        explicit Implementation(const std::string& system_name,
                                const std::string& system_discovery_url)
            : _system_name(system_name),
              _system_discovery_url(system_discovery_url),
              _execution_config{},
              _service_bus_wrapper(getServiceBusWrapper())
        {
            _service_bus_wrapper.createOrGetServiceBusConnection(system_name, system_discovery_url);
            _logger->initRPCService(_system_name);
        }

        Implementation(const Implementation& other) = delete;
        Implementation& operator=(const Implementation& other) = delete;

        // why move constructor is not called?
        Implementation& operator=(Implementation&& other)
        {
            _system_name = std::move(other._system_name);
            _system_discovery_url = std::move(other._system_discovery_url);
            _participants = std::move(other._participants);
            _logger = std::move(other._logger);
            _service_bus_wrapper = other._service_bus_wrapper;
            return *this;
        }

        ~Implementation()
        {
            clear();
        }

        std::vector<std::string> mapToStringVec() const
        {
            std::vector<std::string> participants;
            for (const auto& p : _participants)
            {
                participants.push_back(p.getName());
            }
            return participants;
        }

        std::vector<ParticipantProxy> mapToProxyVec() const
        {
            return _participants;
        }

        void reverse_state_change(std::chrono::milliseconds,
            const std::string& logging_info,
            bool init_false_start_true,
            const std::function<void(RPCComponent<rpc::IRPCParticipantStateMachine>&)>& call_at_state,
            std::vector<fep3::ParticipantProxy>& participants,
            ExecutionConfig execution_config = ExecutionConfig())
        {
            std::mutex mutex;

            if (participants.empty())
            {
                _logger->log(LoggerSeverity::warning, "",
                    _system_name, "No participants within the current system");
                return;
            }
            std::map<int32_t, std::vector<ParticipantProxy>> sorted_part;
            if (!init_false_start_true)
            {
                sorted_part = getParticipantsSortedbyInitPrio(participants);
            }
            else
            {
                sorted_part = getParticipantsSortedbyStartPrio(participants);
            }
            for_each_ordered_reverse(sorted_part,
                execution_config,
                [&](ParticipantProxy& proxy)
                {
                    auto state_machine = proxy.getRPCComponentProxyByIID<rpc::IRPCParticipantStateMachine>();
                    if (state_machine)
                    {
                        try
                        {
                            call_at_state(state_machine);
                        }
                        catch (const std::exception& ex)
                        {
                            const auto proxy_name = proxy.getName();
                            {
                                std::lock_guard<std::mutex> lock_guard(mutex);
                                _last_transition_failed_participants.push_back(proxy_name);
                            }
                            _logger->log(LoggerSeverity::warning, "",
                                _system_name, a_util::strings::format("Participant %s threw exception: '%s',"
                                    "could not be %s successfully and remains in state '%s'.\n",
                                proxy.getName().c_str(), ex.what(), logging_info.c_str(), toString(state_machine->getState()).c_str()));

                        }
                    }
            });
            if (!_last_transition_failed_participants.empty())
            {
                _logger->log(LoggerSeverity::info, "",
                    _system_name, "System could not be " + logging_info + " in a homogeneous way. "
                    "Failed participants wont be considered for further transitions.");
            }
            else {
                _logger->log(LoggerSeverity::info, "",
                    _system_name, "System " + logging_info + " successfully.");
            }
        }

        void normal_state_change(std::chrono::milliseconds,
            const std::string& logging_info,
            bool init_false_start_true,
            const std::function<void(RPCComponent<rpc::IRPCParticipantStateMachine>&)>& call_at_state,
            std::vector<fep3::ParticipantProxy>& participants)
        {
            if (_participants.empty())
            {
                _logger->log(LoggerSeverity::warning, "",
                    _system_name, "No participants within the current system");
                return;
            }
            std::map<int32_t, std::vector<ParticipantProxy>> sorted_part;
            if (!init_false_start_true)
            {
                sorted_part = getParticipantsSortedbyInitPrio(participants);
            }
            else
            {
                sorted_part = getParticipantsSortedbyStartPrio(participants);
            }
            for_each_ordered(sorted_part,
                [&](ParticipantProxy& proxy)
                {
                    auto state_machine = proxy.getRPCComponentProxyByIID<rpc::IRPCParticipantStateMachine>();
                    if (state_machine)
                    {
                        try
                        {
                            call_at_state(state_machine);
                        }
                        catch (const std::exception& ex)
                        {
                            const auto proxy_name = proxy.getName();
                            {
                                _last_transition_failed_participants.push_back(proxy_name);
                            }
                            _logger->log(LoggerSeverity::warning, "",
                                _system_name, a_util::strings::format("Participant %s threw exception: '%s',"
                                    "could not be %s successfully and remains in state '%s'.\n",
                                    proxy.getName().c_str(), ex.what(), logging_info.c_str(), toString(state_machine->getState()).c_str()));

                        }
                    }
                });
            if (!_last_transition_failed_participants.empty())
            {
                _logger->log(LoggerSeverity::info, "",
                    _system_name, "System could not be " + logging_info + " in a homogeneous way. "
                    "Failing participants wont be considered for further transitions.");
            }
            else {
                _logger->log(LoggerSeverity::info, "",
                    _system_name, "System " + logging_info + " successfully.");
            }
        }

        void setSystemState(
            System::AggregatedState state,
            std::chrono::milliseconds timeout)
        {
            auto tmp_participants = _participants;
            _last_transition_failed_participants.clear();
            setSystemState(state, timeout, tmp_participants);
        }

        void decreaseSystemState(
            System::State currentState,
            const ParticipantStates& states,
            const System::AggregatedState state,
            const std::chrono::milliseconds timeout,
            std::vector<ParticipantProxy>& participants)
        {
            if (currentState._state == System::AggregatedState::running)
            {
                if (state == System::AggregatedState::paused)
                {
                    currentState._homogeneous ? pause(timeout, participants) :
                        pause(timeout, getParticipantsByState(participants, states, System::AggregatedState::running));
                }
                else
                {
                    currentState._homogeneous ? stop(timeout, participants) :
                        stop(timeout, getParticipantsByState(participants, states, System::AggregatedState::running));
                }
            }
            else if (currentState._state == System::AggregatedState::paused)
            {
                currentState._homogeneous ? stop(timeout, participants) :
                    stop(timeout, getParticipantsByState(participants, states, System::AggregatedState::paused));
            }
            else if (currentState._state == System::AggregatedState::initialized)
            {
                currentState._homogeneous ? deinitialize(timeout, participants) :
                    deinitialize(timeout, getParticipantsByState(participants, states, System::AggregatedState::initialized));
            }
            else if (currentState._state == System::AggregatedState::loaded)
            {
                currentState._homogeneous ? unload(timeout, participants) :
                    unload(timeout, getParticipantsByState(participants, states, System::AggregatedState::loaded));
            }
        }

        void increaseSystemState(
            const System::State currentState,
            const ParticipantStates& states,
            const System::AggregatedState state,
            const std::chrono::milliseconds timeout,
            std::vector<ParticipantProxy>& participants)
        {
            if (currentState._state == System::AggregatedState::unloaded)
            {
                currentState._homogeneous ? load(timeout, participants) :
                    load(timeout, getParticipantsByState(participants, states, System::AggregatedState::unloaded));
            }
            else if (currentState._state == System::AggregatedState::loaded)
            {
                currentState._homogeneous ? initialize(timeout, participants) :
                    initialize(timeout, getParticipantsByState(participants, states, System::AggregatedState::loaded));
            }
            else if (currentState._state == System::AggregatedState::initialized)
            {
                if (state == System::AggregatedState::paused)
                {
                    currentState._homogeneous ? pause(timeout, participants) :
                        pause(timeout, getParticipantsByState(participants, states, System::AggregatedState::initialized));
                }
                else
                {
                    currentState._homogeneous ? start(timeout, participants) :
                        start(timeout, getParticipantsByState(participants, states, System::AggregatedState::initialized));
                }
            }
            else if (currentState._state == System::AggregatedState::paused)
            {
                currentState._homogeneous ? start(timeout, participants) :
                    start(timeout, getParticipantsByState(participants, states, System::AggregatedState::paused));
            }
        }

        void setSystemState(
            System::AggregatedState state,
            std::chrono::milliseconds timeout,
            std::vector<ParticipantProxy>& participants,
            bool reversed = false)
        {
            for (const auto& failed_participant : _last_transition_failed_participants)
            {
                participants.erase(std::remove_if(participants.begin(), participants.end(),
                    [&failed_participant](ParticipantProxy& participant_proxy) {
                        return participant_proxy.getName() == failed_participant;
                    }), participants.end());
            }

            if (System::AggregatedState::unreachable == state)
            {
                setSystemState(SystemAggregatedState::unloaded, timeout, participants, true);
                shutdown(FEP_SYSTEM_TRANSITION_TIMEOUT);
                return;
            }

            if (System::AggregatedState::undefined == state)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::error,
                    "",
                    getName(),
                    "Invalid setSystemState call at system " + getName());
            }

            auto states = getParticipantStates(timeout, participants);
            auto currentState = reversed ? getAggregatedStateReversed(states) : getAggregatedState(states);
            if (currentState._state == System::AggregatedState::unreachable)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::error,
                    "",
                    getName(),
                    "At least one participant is unreachable, can not set homogenous state of the system " + getName());
            }
            else if (currentState._state == System::AggregatedState::undefined)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::error,
                    "",
                    getName(),
                    "No participant has a statemachine, can not set homogenous state of the system " + getName());
            }
            else if (currentState._state == state)
            {
                if (currentState._homogeneous)
                {
                    return;
                }
                setSystemState(state, timeout, participants, true);
                return;
            }
            else if (currentState._state > state)
            {
                decreaseSystemState(currentState, states, state, timeout, participants);
                setSystemState(state, timeout, participants, true);
                return;
            }
            else if (currentState._state < state)
            {
                increaseSystemState(currentState, states, state, timeout, participants);
                setSystemState(state, timeout, participants);
                return;
            }
        }

        void load(std::chrono::milliseconds timeout,
            std::vector<fep3::ParticipantProxy> participants_to_transition = {})
        {
            _last_transition_failed_participants.clear();
            reverse_state_change(timeout, "loaded", false,
            [&](RPCComponent<rpc::IRPCParticipantStateMachine>& state_machine)
            {
                if (state_machine)
                {
                    state_machine->load();
                }
            },
                !participants_to_transition.empty() ? participants_to_transition : _participants);
        }

        void unload(std::chrono::milliseconds timeout,
            std::vector<fep3::ParticipantProxy> participants_to_transition = {})
        {
            _last_transition_failed_participants.clear();
            normal_state_change(timeout, "unloaded", false,
                [&](RPCComponent<rpc::IRPCParticipantStateMachine>& state_machine)
            {
                if (state_machine)
                {
                    state_machine->unload();
                }
            },
                !participants_to_transition.empty() ? participants_to_transition : _participants);
        }

        void initialize(std::chrono::milliseconds timeout,
            std::vector<fep3::ParticipantProxy> participants_to_transition = {})
        {
            _last_transition_failed_participants.clear();
            reverse_state_change(timeout, "initialized", false,
                [&](RPCComponent<rpc::IRPCParticipantStateMachine>& state_machine)
            {
                if (state_machine)
                {
                    state_machine->initialize();
                }
            },
                !participants_to_transition.empty() ? participants_to_transition : _participants,
            _execution_config);
        }

        void deinitialize(std::chrono::milliseconds timeout,
            std::vector<fep3::ParticipantProxy> participants_to_transition = {})
        {
            _last_transition_failed_participants.clear();
            normal_state_change(timeout, "deinitialized", false,
                [&](RPCComponent<rpc::IRPCParticipantStateMachine>& state_machine)
            {
                if (state_machine)
                {
                    state_machine->deinitialize();
                }
            },
                !participants_to_transition.empty() ? participants_to_transition : _participants);
        }

        void start(std::chrono::milliseconds timeout,
            std::vector<fep3::ParticipantProxy> participants_to_transition = {})
        {
            _last_transition_failed_participants.clear();
            reverse_state_change(timeout, "started", true,
                [&](RPCComponent<rpc::IRPCParticipantStateMachine>& state_machine)
            {
                if (state_machine)
                {
                    state_machine->start();
                }
            },
                !participants_to_transition.empty() ? participants_to_transition : _participants,
            _execution_config);
        }

        void pause(std::chrono::milliseconds timeout,
            std::vector<fep3::ParticipantProxy> participants_to_transition = {})
        {
            _last_transition_failed_participants.clear();
            reverse_state_change(timeout, "paused", false,
                [&](RPCComponent<rpc::IRPCParticipantStateMachine>& state_machine)
            {
                if (state_machine)
                {
                    state_machine->pause();
                }
            },
                !participants_to_transition.empty() ? participants_to_transition : _participants);
        }
        void stop(std::chrono::milliseconds timeout,
            std::vector<fep3::ParticipantProxy> participants_to_transition = {})
        {
            _last_transition_failed_participants.clear();
            normal_state_change(timeout, "stopped", false,
                [&](RPCComponent<rpc::IRPCParticipantStateMachine>& state_machine)
            {
                if (state_machine)
                {
                    state_machine->stop();
                }
            },
                !participants_to_transition.empty() ? participants_to_transition : _participants);
        }

        void shutdown(std::chrono::milliseconds)
        {
            if (_participants.empty())
            {
                _logger->log(LoggerSeverity::warning, "",
                    _system_name + ".system", "No participants within the current system");
                return;
            }
            std::string error_message;
            //shutdown has no prio
            for (auto& part : _participants)
            {
                part.deregisterLogging();
                auto state_machine = part.getRPCComponentProxyByIID<rpc::IRPCParticipantStateMachine>();
                if (state_machine)
                {
                    try
                    {
                        state_machine->shutdown();
                    }
                    catch (const std::exception& ex)
                    {
                        error_message += std::string(" ") + ex.what();
                    }
                }
            }
            if (!error_message.empty())
            {
                FEP3_SYSTEM_LOG_AND_THROW(
                    _logger,
                    LoggerSeverity::fatal,
                    "",
                    _system_name,
                    error_message);
            }
            _logger->log(LoggerSeverity::info, "",
                _system_name, "system shut down successfully");
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
        ParticipantStates getParticipantStates(std::chrono::milliseconds, const std::vector<ParticipantProxy>& participants)
        {
            ParticipantStates states;
            for (const auto& part : participants)
            {
                RPCComponent<rpc::arya::IRPCParticipantInfo> part_info;
                RPCComponent<rpc::arya::IRPCParticipantStateMachine> state_machine;
                part_info = part.getRPCComponentProxyByIID<rpc::arya::IRPCParticipantInfo>();
                state_machine = part.getRPCComponentProxyByIID<rpc::arya::IRPCParticipantStateMachine>();
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
                        states[part.getName()] = { rpc::arya::IRPCParticipantStateMachine::State::unreachable };
                    }
                    else
                    {
                        //the participant has no state machine, this is ok
                        //... i.e. a recorder will have no states and a signal listener tool will have no states
                        states[part.getName()] = { rpc::arya::IRPCParticipantStateMachine::State::unreachable };
                    }
                }
            }
            return std::move(states);
        }

        // Returns the lowest state and information whether all states are homogeneous.
        static System::State getAggregatedState(const ParticipantStates& states)
        {
            //we begin at the highest value
            SystemAggregatedState aggregated_state = SystemAggregatedState::running;
            //we have a look if at least one of it is set
            bool first_set = false;
            bool homogeneous_value = true;
            for (const auto& item : states)
            {
                rpc::arya::IRPCParticipantStateMachine::State current_part_state = item.second;
                //we were at least one times in this loop
                if (current_part_state != rpc::arya::IRPCParticipantStateMachine::State::undefined)
                {
                    if (current_part_state < aggregated_state)
                    {
                        if (first_set)
                        {
                            //is not homogenous because some of the already checked states has an higher value
                            homogeneous_value = false;
                        }
                        aggregated_state = current_part_state;
                    }
                    if (current_part_state > aggregated_state)
                    {
                        //is not homogenous because some of the already checked states has an lower value
                        homogeneous_value = false;
                    }
                    //this is to check if this loop was already entered before we change
                    first_set = true;
                }
            }
            //no participant has a statemachine, then we are also undefined
            if (!first_set)
            {
                return { rpc::arya::IRPCParticipantStateMachine::State::undefined };
            }
            else
            {
                return { homogeneous_value , aggregated_state };
            }
        }

        // Returns the highest state and information whether all states are homogeneous.
        static System::State getAggregatedStateReversed(const ParticipantStates& states)
        {
            //we begin at the lowest value
            SystemAggregatedState aggregated_state = SystemAggregatedState::unreachable;
            //we have a look if at least one of it is set
            bool first_set = false;
            bool homogeneous_value = true;
            for (const auto& item : states)
            {
                rpc::arya::IRPCParticipantStateMachine::State current_part_state = item.second;
                //we were at least one times in this loop
                if (current_part_state != rpc::arya::IRPCParticipantStateMachine::State::undefined)
                {
                    if (current_part_state > aggregated_state)
                    {
                        if (first_set)
                        {
                            //is not homogenous because some of the already checked states has an lower value
                            homogeneous_value = false;
                        }
                        aggregated_state = current_part_state;
                    }
                    if (current_part_state < aggregated_state)
                    {
                        //is not homogenous because some of the already checked states has an higher value
                        homogeneous_value = false;
                    }
                    //this is to check if this loop was already entered before we change
                    first_set = true;
                }
            }
            //no participant has a statemachine, then we are also undefined
            if (!first_set)
            {
                return { rpc::arya::IRPCParticipantStateMachine::State::undefined };
            }
            else
            {
                return { homogeneous_value , aggregated_state };
            }
        }

        System::State getSystemState(std::chrono::milliseconds timeout)
        {
            return getAggregatedState(getParticipantStates(timeout));
        }

        void registerMonitoring(IEventMonitor* monitor)
        {
            // register first in case a warning has to be logged
            _logger->registerMonitor(monitor);

            for (const auto& participant : _participants)
            {
                if (!participant.loggingRegistered())
                {
                    _logger->log(LoggerSeverity::warning, "",
                    _system_name,
                    a_util::strings::format("Participant %s has no registered logging interface.", participant.getName().c_str()));
                }
            }
        }

        void releaseMonitoring()
        {
            _logger->releaseMonitor();
        }

        void setSeverityLevel(LoggerSeverity level)
        {
            _logger->setSeverityLevel(level);
        }

        void clear()
        {
            _participants.clear();
        }

        void addAsync(const std::multimap<std::string, std::string>& participants, uint8_t pool_size)
        {
            // find if we have duplicates
            auto part_found = searchParticipant(participants, false);
            if (part_found)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::fatal,
                    "",
                    _system_name,
                    "Try to add a participant with name "
                    + part_found.getName() + " which already exists.");
            }

            boost::asio::thread_pool pool(pool_size);
            //preallocate the vector
            _participants = std::vector<ParticipantProxy>(participants.size());
            uint32_t index = 0;

            for (auto& part_to_call : participants)
            {
                boost::asio::post(pool,
                    [&, index]()
                    {
                        // we can initialize safely in multi thread execution, each thread touches a different vector index
                        _participants[index] = ParticipantProxy(part_to_call.first,
                            part_to_call.second,
                            _system_name,
                            _system_discovery_url,
                            _logger,
                            PARTICIPANT_DEFAULT_TIMEOUT);
                    });
                ++index;
            }
            pool.join();
        }

        void add(const std::string& participant_name, const std::string& participant_url)
        {
            auto part_found = getParticipant(participant_name, false);
            if (part_found)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::fatal,
                    "",
                    _system_name,
                    "Try to add a participant with name "
                    + participant_name + " which already exists.");
            }
            _participants.push_back(ParticipantProxy(participant_name,
                participant_url,
                _system_name,
                _system_discovery_url,
                _logger,
                PARTICIPANT_DEFAULT_TIMEOUT));
        }

        void remove(const std::string& participant_name)
        {
            auto found = _participants.begin();
            for (;
                found != _participants.end();
                ++found)
            {
                if (found->getName() == participant_name)
                {
                    break;
                }
            }
            if (found != _participants.end())
            {
                _participants.erase(found);
            }
        }

        ParticipantProxy getParticipant(const std::string& participant_name, bool throw_if_not_found) const
        {
            for (auto& part_found : _participants)
            {
                if (part_found.getName() == participant_name)
                {
                    return part_found;
                }
            }
            if (throw_if_not_found)
            {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::fatal,
                    "",
                    _system_name,
                    "No Participant with the name " + participant_name + " found");
            }
            return {};
        }

        ParticipantProxy searchParticipant(const std::multimap<std::string, std::string>& participants, bool throw_if_not_found) const
        {
            auto particpant_names = boost::adaptors::keys(participants);

            auto part_found = std::search(
                _participants.begin(),
                _participants.end(),
                particpant_names.begin(),
                particpant_names.end(),
                [](const ParticipantProxy & part_proxy, const std::string & part_name)
                {
                    return part_proxy.getName() == part_name;
                });

            if (part_found == _participants.end())
            {
                if (throw_if_not_found)
                {
                    FEP3_SYSTEM_LOG_AND_THROW(_logger,
                        LoggerSeverity::fatal,
                        "",
                        _system_name,
                        "No Participant with any of the names " + boost::algorithm::join(particpant_names, " ") + ", was found");
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
                throwIfNotValid<IRPCConfiguration>(config_rpc_client);

                auto props = config_rpc_client->getProperties(property_node);
                if (!props) {
                    throw std::runtime_error(format("access to properties node %s not possible", property_node.c_str()));
                }
                return std::make_pair(props, property_name);
            }
            else {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::fatal,
                    "",
                    _system_name,
                    format("participant %s within system %s not found to configure %s",
                        participant_name.c_str(),
                        _system_name.c_str(),
                        property_path.c_str()));
            }
        }

        void setParticipantProperty(const std::string& participant_name,
            const std::string& property_path,
            const std::string& property_value,
            const bool throw_if_not_found) const
        {
            std::string property_path_normalized = property_path;
            a_util::strings::replace(property_path_normalized, ".", "/");
            ParticipantProxy part = getParticipant(participant_name, throw_if_not_found);
            auto [props, property_name] = getParticipantProperties(participant_name, property_path_normalized, part);

            if (!props->setProperty(property_name, property_value, props->getPropertyType(property_name))) {
                        const auto message = format("property %s could not be set for the following participant: %s"
                    , property_path_normalized.c_str()
                    , part.getName().c_str());
                throw std::runtime_error(message);
            }
        }

        std::string getParticipantProperty(const std::string& participant_name,
                                           const std::string& property_path) const
        {
            std::string property_path_normalized = property_path;
            a_util::strings::replace(property_path_normalized, ".", "/");
            ParticipantProxy part = getParticipant(participant_name, true);
            auto [props, property_name] = getParticipantProperties(participant_name, property_path_normalized, part);
            return props->getProperty(property_name);
        }

        SystemAggregatedState getParticipantState(const std::string& participant_name)
        {
            ParticipantProxy participant(getParticipant(participant_name, true));
            ParticipantStates states = getParticipantStates(std::chrono::milliseconds(0),
                std::vector<ParticipantProxy>{participant});
            return states.empty() ? SystemAggregatedState::undefined : states[participant.getName()];
        }

        ParticipantStates getParticipantStates(std::chrono::milliseconds timeout)
        {
            return getParticipantStates(timeout, _participants);
        }

        void setParticipantState(const std::string& participant_name, const SystemAggregatedState participant_state)
        {
            auto part = getParticipant(participant_name, true);
            if (part) {
                // create a second temporary system with only one participant
                fep3::System tmp_system(this->getName());
                tmp_system.add(participant_name);

                if (participant_state == fep3::SystemAggregatedState::unreachable) {
                    tmp_system.setSystemState(fep3::SystemAggregatedState::unloaded);
                    tmp_system.shutdown();
                    _participants.erase(std::find(begin(_participants), end(_participants), part));
                }
                else {
                    tmp_system.setSystemState(participant_state);
                }
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
            a_util::strings::replace(property_normalized, ".", "/");
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
                    throwIfNotValid<IRPCConfiguration>(config_rpc_client);

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
                const auto participants = join(failing_participants, ", ");
                const auto message = format("property %s could not be set for the following participants: %s"
                    , property_normalized.c_str()
                    , participants.c_str());

                if (throw_on_failure)
                {
                    throw std::runtime_error(message);
                }

                FEP3_SYSTEM_LOG(_logger,
                    LoggerSeverity::warning,
                    "",
                    _system_name,
                    message);
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
                throwIfNotValid<IRPCConfiguration>(config_rpc_client);

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
                    FEP3_SYSTEM_LOG_AND_THROW(_logger, LoggerSeverity::fatal, "", _system_name,
                        "Multiple Participants with the name " + participant.getName() + " found");
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
                throwIfNotValid<IRPCConfiguration>(config_rpc_client);

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
                throw std::runtime_error("thread count with value 0 is not valid");
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
                try 
                {
                    throwIfNotValid<fep3::rpc::IRPCHttpServer>(http_server_rpc);
                    auto interval = http_server_rpc->getHeartbeatInterval();
                    return interval;
                }
                catch (const std::exception& ex)
                {
                    FEP3_SYSTEM_LOG_AND_THROW(_logger, LoggerSeverity::fatal, "", _system_name, ex.what());
                }
            }
            else
            {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::fatal,
                    "",
                    _system_name,
                    format("participant %s within system %s not found%s",
                        participant.c_str(),
                        _system_name.c_str()));
            }
        }

        void setHeartbeatInterval(const std::string& participant, const std::chrono::milliseconds interval_ms)
        {
            auto part = getParticipant(participant, false);
            if (part)
            {
                auto http_server_rpc = part.getRPCComponentProxyByIID<fep3::rpc::IRPCHttpServer>();
                try
                {
                    throwIfNotValid<fep3::rpc::IRPCHttpServer>(http_server_rpc);
                    http_server_rpc->setHeartbeatInterval(interval_ms);
                }
                catch (const std::exception& ex)
                {
                    FEP3_SYSTEM_LOG_AND_THROW(_logger, LoggerSeverity::fatal, "", _system_name, ex.what());
                }
            }
            else
            {
                FEP3_SYSTEM_LOG_AND_THROW(_logger,
                    LoggerSeverity::fatal,
                    "",
                    _system_name,
                    format("participant %s within system %s not found%s",
                        participant.c_str(),
                        _system_name.c_str()));
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
            for (auto& participant : _participants)
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
            for (auto& participant : _participants)
            {
                participant.setHealthListenerRunningStatus(running);
            }
        }

        std::pair<bool, bool> getHealthListenerRunningStatus() const
        {
            std::vector<bool> participant_health_listener_running(_participants.size());

            std::transform(_participants.begin(), _participants.end(), participant_health_listener_running.begin(), [](const auto& participant) {return participant.getHealthListenerRunningStatus(); });

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

        std::vector<ParticipantProxy> _participants;
        std::vector<std::string> _last_transition_failed_participants;
        std::shared_ptr<SystemLogger> _logger = std::make_shared<SystemLogger>();
        std::string _system_name;
        std::string _system_discovery_url;
        ServiceBusWrapper _service_bus_wrapper;
        ::ExecutionConfig _execution_config;
        fep3::Timestamp _liveliness_timeout = std::chrono::nanoseconds(std::chrono::seconds(20));
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
        _impl->_system_name = getSystemName();
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
        _impl->setSystemState(state, timeout);
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

    void System::registerMonitoring(IEventMonitor& pEventListener)
    {
        _impl->registerMonitoring(&pEventListener);
    }

    void System::unregisterMonitoring(IEventMonitor&)
    {
        _impl->releaseMonitoring();
    }

    void System::setSeverityLevel(LoggerSeverity severity_level)
    {
        _impl->setSeverityLevel(severity_level);
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
                    + name + "' at url '" + discover_url);
            }
           // auto participants = sys_access->discover(timeout);
            auto [error, participants_optional] = discoverSystemParticipants(*sys_access, std::forward<Args>(args)...);
            if (!participants_optional.has_value())
            {
                throw std::runtime_error(error);
            }
            std::multimap<std::string, std::string> participants = participants_optional.value();

            System discovered_system(name, discover_url);
            discovered_system.addAsync(participants);
            return discovered_system;
        }
        throw std::runtime_error("can not create a service bus connection to system '"
            + name + "' at url '" + discover_url + "'");
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
                    + discover_url);
            }
            // auto all_participants = sys_access->discover(timeout);
            auto [error, participants_optional] = discoverSystemParticipants(*sys_access, std::forward<Args>(args)...);
            if (!participants_optional.has_value())
            {
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

                    const std::string error_mesage = a_util::strings::format(
                        "Parsing error while discoverAllSystems by URL %s .Expected a string like participant_name@system_name but got %s ",
                        discover_url.c_str(),
                        part.first.c_str());
                    throw std::runtime_error(error_mesage);
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
            + discover_url + "'");
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
        return fep3::SystemAggregatedState::undefined;
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
