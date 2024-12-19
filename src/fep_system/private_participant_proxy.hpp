/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#pragma once
#include "system_logger_intf.h"
#include "participant_health_listener.h"

#include "rpc_services/participant_info_proxy.hpp"
#include "rpc_services/participant_statemachine_proxy.hpp"
#include "rpc_services/clock_proxy.hpp"
#include "rpc_services/data_registry_proxy.hpp"
#include "rpc_services/logging_proxy.hpp"
#include "rpc_services/configuration_proxy.hpp"
#include "rpc_services/health_proxy.hpp"
#include "rpc_services/http_server_proxy.hpp"
#include "rpc_services/rpc_passthrough.hpp"
#include "service_bus_wrapper.h"
#include "system_logger_helper.h"

#include <fep3/components/service_bus/service_bus_intf.h>

#include <math.h>
#include <string>

namespace fep3
{

struct ParticipantProxy::Implementation 
{
public:
    //this is the current type used within connect()
    //usually this type must be supported as lowest versioned types forever !
    using ConnectParticipantInfo = rpc::arya::IRPCParticipantInfo ;
    using ConnectStateMachine = rpc::catelyn::IRPCParticipantStateMachine ;
    using ConnectLoggingSinkService = rpc::arya::IRPCLoggingSinkService ;
    using ConnectConfigurationService = rpc::arya::IRPCConfiguration ;
    using ConnectHealthService = rpc::catelyn::IRPCHealthService ;
    using ConnectHttpServer = rpc::catelyn::IRPCHttpServer ;

    template<typename T>
    class RPCComponentCache
    {
    public:
        typedef T value_type;
        RPCComponentCache(ParticipantProxy::Implementation* proxy_impl) : _proxy_impl(proxy_impl)
        {
        }
        RPCComponent<T> getValue()
        {
            if (!_value)
            {
                _value = connect();
            }
            return _value;
        }
        RPCComponent<T> connect()
        {
            try
            {
                //it is very important to use arya here ...
                //because we support versioning !!
                RPCComponent<T> val;
                if (_proxy_impl->getRPCComponentProxy(T::getRPCDefaultName(),
                    T::getRPCIID(),
                    val))
                {
                    return val;
                }
                else
                {
                    FEP3_SYSTEM_LOG(_proxy_impl->getSystemLogger(), LoggerSeverity::warning,
                        a_util::strings::format("Participant %s is unreachable - RPC communication to retrieve RPC service with '%s' failed", _proxy_impl->getParticipantName().c_str(), T::getRPCIID()));
                    return {};
                }
            }
            catch (const std::runtime_error& ex)
            {
                FEP3_SYSTEM_LOG(_proxy_impl->getSystemLogger(), LoggerSeverity::fatal, a_util::strings::format("Participant %s threw runtime exception '%s'.", _proxy_impl->getParticipantName().c_str(), ex.what()));
                return {};
            }
            catch (...)
            {
                FEP3_SYSTEM_LOG(_proxy_impl->getSystemLogger(), LoggerSeverity::fatal,
                    a_util::strings::format("Participant %s threw unknown exception - RPC communication to retrieve RPC service with '%s' failed", _proxy_impl->getParticipantName().c_str(), T::getRPCIID()));
                return {};
            }
        }
        bool hasValue() const
        {
            return static_cast<bool>(_value);
        }
    private:
        ParticipantProxy::Implementation* _proxy_impl;
        RPCComponent<T> _value;
    };

    class InfoCache
    {
    public:
        std::vector<std::string> getComponentsWhichSupports(
            RPCComponent<ConnectParticipantInfo>& info,
            const std::string& iid,
            ConnectStateMachine::State state_current)
        {
            std::lock_guard<std::recursive_mutex> lock(_sync);
            auto it = _found_components_byiid.find(iid);
            if (it != _found_components_byiid.end()
                && state_current == _last_state)
            {
                return it->second;
            }
            else
            {
                _found_components_byiid.clear();
                auto found_objects = info->getRPCComponents();
                for (const auto& current_object : found_objects)
                {
                    std::vector<std::string> found_interfaces;
                    found_interfaces = info->getRPCComponentIIDs(current_object);
                    for (const auto& current_iid : found_interfaces)
                    {
                        _found_components_byiid[current_iid].push_back(current_object);
                    }
                }
                _last_state = state_current;
                return _found_components_byiid[iid];
            }
        }

        void reset()
        {
            std::lock_guard<std::recursive_mutex> lock(_sync);
            _found_components_byiid.clear();
            _last_state = ConnectStateMachine::State::unreachable;
        }

    private:
        std::map<std::string, std::vector<std::string>> _found_components_byiid;
        std::recursive_mutex _sync;
        ConnectStateMachine::State _last_state{ConnectStateMachine::State::unreachable};
    };

    Implementation(const std::string& participant_name,
        const std::string& participant_url,
        const std::string& system_name,
        const std::string& system_discovery_url,
        const std::string& rpc_server_url,
        std::shared_ptr<ISystemLogger> system_logger,
        std::chrono::milliseconds default_timeout) :
        _rpc_server_url(rpc_server_url),
        _system_logger(system_logger),
        _participant_name(participant_name),
        _system_name(system_name),
        _participant_url(participant_url),      
        _info(this),
        _state_machine(this),
        _logging(this),
        _config(this),
        _health(this),
        _http_server(this),
        _default_timeout(default_timeout),
        _service_bus_wrapper(getServiceBusWrapper())
    {

        _system_access = _service_bus_wrapper.createOrGetServiceBusConnection(system_name, system_discovery_url)->getSystemAccessCatelyn(system_name);

        if (!_system_access)
        {
            FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                a_util::strings::format("While contructing %s at %s no system connection to %s at %s possible", participant_name.c_str(), participant_url.c_str(), system_name.c_str(), system_discovery_url.c_str()));
        }
        initHealthListener(system_name);

        _info.getValue();

        //only if info hasValue ... then it makes sense to connect to the others
        //otherwise there is a huge timeout for every connecting
        if (_info.hasValue())
        {
            _state_machine_proxy_factory = createStateMachineProxy();
            _state_machine.getValue();
            _config.getValue();
            auto logging = _logging.getValue();
            if (logging)
            {
                if (const auto result = fep3::rpc::arya::RPCLoggingSinkRegistrationSingleton::getInstance()->registerRPCClient(
                    logging,
                    _rpc_server_url,
                    _participant_name,
                    _system_name
                ); result)
                {
                    _registered_logging = true;
                }
                else
                {
                    FEP3_SYSTEM_LOG(_system_logger,
                        LoggerSeverity::warning,
                        result.getDescription());
                }
            }
        }
    }

    /// @brief Deregisters the RPC Proxy from the remote participant logger
    /// and deactivates the participant health polling after the reception of a heartbeat
    void deregisterLogging()
    {
        // ensures that no logging is performed after the logger is deregistered
        _participant_health_Listener->deactivateLogging();
        if (_registered_logging &&(!_participant_proxy_not_reachable))
        {
            auto logging = _logging.getValue();
            if (logging)
            {
                if (const auto result = fep3::rpc::arya::RPCLoggingSinkRegistrationSingleton::getInstance()->unregisterRPCClient(
                    logging, 
                    _rpc_server_url,
                    _participant_name,
                    _system_name
                ); result)
                {
                    _registered_logging = false;
                }
                else
                {
                    FEP3_SYSTEM_LOG(_system_logger,
                        LoggerSeverity::warning,
                        result.getDescription());
                }
            }
        }
        if (_health_listener_running) {
            _system_access->deregisterUpdateEventSink(_participant_health_Listener.get());
            _health_listener_running = false;
        }
    }

    virtual ~Implementation()
    {
        deregisterLogging();
    }

    void copyValuesTo(Implementation& other) const
    {
        other._participant_name = _participant_name;
        other._participant_url = _participant_url;
        other._default_timeout = _default_timeout;
        other._additional_info = _additional_info;
        other._service_bus_wrapper = _service_bus_wrapper;
        other._system_access = _system_access;
        other._init_priority = _init_priority;
        other._start_priority = _start_priority;
    }


    std::string getParticipantName() const
    {
        return _participant_name;
    }

    std::string getParticipantURL() const
    {
        return _participant_url;
    }

    void setStartPriority(int32_t prio)
    {
        setPriority(FEP3_SERVICE_BUS_START_PRIORITY, prio, _start_priority);
    }

    int32_t getStartPriority() const
    {
        return getPriority(FEP3_SERVICE_BUS_START_PRIORITY, _start_priority);
    }

    void setInitPriority(int32_t prio)
    {
        setPriority(FEP3_SERVICE_BUS_INIT_PRIORITY, prio, _init_priority);
    }

    int32_t getInitPriority() const
    {
        return getPriority(FEP3_SERVICE_BUS_INIT_PRIORITY, _init_priority);
    }

    bool getRPCComponentProxy(const std::string& component_name,
        const std::string& component_iid,
        IRPCComponentPtr& proxy_ptr) const
    {
        //this is very special and must be handled separately
        auto requester = _system_access->getRequester(_participant_name);
        if (requester == nullptr)
        {
            FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                a_util::strings::format("Participant %s is not discovered", _participant_name.c_str()));
            return false;
        }
        else if (component_iid == fep3::rpc::getRPCIID<ConnectParticipantInfo>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::ParticipantInfoProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<ConnectStateMachine>())
        {
            if (!_state_machine_proxy_factory)
            {
                _state_machine_proxy_factory = createStateMachineProxy();
            }
            return proxy_ptr.reset(_state_machine_proxy_factory->getProxy(requester));
        }
        else if (component_iid == fep3::rpc::getRPCIID<ConnectLoggingSinkService>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::LoggingSinkService>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<fep3::rpc::experimental::RPCPassthrough>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::experimental::RPCPassthrough>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<ConnectHealthService>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::catelyn::HealthServiceProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<ConnectHttpServer>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::catelyn::HttpServerProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }

        auto names = getComponentNameWhichSupports(component_iid);
        bool found_and_support = false;
        if (names.empty())
        {
            FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                a_util::strings::format("Participant %s has no RPC service with iid %s", _participant_name.c_str(), component_iid.c_str()));
            return false;
        }
        else
        {
            for (const auto& current : names)
            {
                if (current == component_name)
                {
                    found_and_support = true;
                    break;
                }
            }
            if (!found_and_support)
            {
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                    a_util::strings::format("Participant %s does not support RPC service with iid %s", _participant_name.c_str(), component_iid.c_str()));
                return false;
            }
        }
        //this is our list to raise ... maybe we need a factory in the future
        if (component_iid == fep3::rpc::getRPCIID<rpc::arya::IRPCParticipantInfo>())
        {
            //also if this type is the same like ConnectParticipantInfo
            //we check that here for future use!!
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::ParticipantInfoProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<rpc::catelyn::IRPCParticipantStateMachine>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::catelyn::ParticipantStateMachineProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<rpc::arya::IRPCClockService>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::ClockServiceProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<rpc::catelyn::IRPCDataRegistry>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::catelyn::DataRegistryProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<rpc::arya::IRPCLoggingService>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::LoggingServiceProxy>(
                component_name,
                requester,
                _participant_name, *_system_logger);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<rpc::arya::IRPCLoggingSinkService>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::LoggingSinkService>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<rpc::arya::IRPCConfiguration>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::ConfigurationProxy>(
                _participant_name,
                component_name,
                requester,
                *_system_logger);
            return proxy_ptr.reset(part_object);
        }
        else if (component_iid == fep3::rpc::getRPCIID<rpc::catelyn::IRPCHealthService>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::catelyn::HealthServiceProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
        }

        return false;
    }

    bool getRPCComponentProxyByIID(const std::string& component_iid,
        IRPCComponentPtr& proxy_ptr) const
    {
        //faster access to the state machine
        if (decltype(_info)::value_type::getRPCIID() == component_iid)
        {
            auto val = _info.getValue();
            if (val)
            {
                return proxy_ptr.reset(_info.getValue().getServiceClient());
            }
            //go ahead and search another object
        }
        //faster access to the state machine
        if (decltype(_state_machine)::value_type::getRPCIID() == component_iid)
        {
            auto val = _state_machine.getValue();
            if (val)
            {
                return proxy_ptr.reset(_state_machine.getValue().getServiceClient());
            }
            //go ahead and search another object
        }
        else if (decltype(_config)::value_type::getRPCIID() == component_iid)
        {
            auto val = _config.getValue();
            if (val)
            {
                return proxy_ptr.reset(_config.getValue().getServiceClient());
            }
            //go ahead and search another object
        }
        else if (decltype(_health)::value_type::getRPCIID() == component_iid)
        {
            auto val = _health.getValue();
            if (val)
            {
                return proxy_ptr.reset(_health.getValue().getServiceClient());
            }
            //go ahead and search another object
        }
        else if (decltype(_http_server)::value_type::getRPCIID() == component_iid)
        {
            auto val = _http_server.getValue();
            if (val)
            {
                return proxy_ptr.reset(_http_server.getValue().getServiceClient());
            }
            //go ahead and search another object
        }

        auto names = getComponentNameWhichSupports(component_iid);
        if (!names.empty())
        {
            //we use the first we found
            return getRPCComponentProxy(names[0], component_iid, proxy_ptr);
        }

        return false;
    }

    std::vector<std::string> getComponentNameWhichSupports(std::string iid) const
    {
        RPCComponent<ConnectParticipantInfo> info = _info.getValue();
        if (!info)
        {
            const std::string err_message = "Participant " + _participant_name + " threw runtime exception - RPC communication to retrieve RPC service with '" + ConnectParticipantInfo::getRPCIID() + "' failed; occured in "
                + std::string(A_UTIL_CURRENT_FUNCTION) + " - " + std::string(__FILE__) + " - line: " + std::to_string(__LINE__); 
            throw std::runtime_error(err_message);
        }
        return _info_cache.getComponentsWhichSupports(info, iid, getCurrentState());
    }

    ConnectStateMachine::State getCurrentState() const
    {
        if (_state_machine.hasValue())
        {
            return _state_machine.getValue()->getState();
        }
        else
        {
            return ConnectStateMachine::State::unreachable;
        }
    }

    void setAdditionalInfo(const std::string& key, const std::string& value)
    {
        _additional_info[key] = value;
    }

    std::string getAdditionalInfo(const std::string& key, const std::string& value_default) const
    {
        auto value = _additional_info.find(key);
        if (value != _additional_info.cend())
        {
            return value->second;
        }
        else
        {
            return value_default;
        }
    }

    bool loggingRegistered() const
    {
        return _registered_logging;
    }

    ParticipantHealthUpdate  getParticipantHealth() const
    {
        if (_health_listener_running)
        {
            return _participant_health_Listener->getParticipantHealth();
        }
        else
        {
            FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                a_util::strings::format("Participant %s health listner is deactivated. " 
                    "To activate this feature please refer to fep3 system library api method 'setHealthListenerRunningStatus()'", _participant_name.c_str()));
        }
    }

    void setHealthListenerRunningStatus(bool running)
    {
        if (_health_listener_running == running)
        {
            return;
        }
        else
        {
            _health_listener_running = running;
            if (running)
            {
                _system_access->registerUpdateEventSink(_participant_health_Listener.get());
            }
            else
            {
                _system_access->deregisterUpdateEventSink(_participant_health_Listener.get());
            }
        }
    }

    bool getHealthListenerRunningStatus() const
    {
        return _health_listener_running;
    }

    void setNotReachable() 
    {
        _participant_proxy_not_reachable = true;
    }

private:
    void initHealthListener(const std::string& system_name)
    {
        _participant_health_Listener = std::make_unique<ParticipantHealthListener>(&(_health.getValue().getInterface()),
            _participant_name,
            system_name,
            [&](LoggerSeverity severity, const std::string& message)
            {
                if (_registered_logging)
                {
                    _system_logger->log(severity, message);
                }
            });

        _system_access->registerUpdateEventSink(_participant_health_Listener.get());
    }

private:
    void setPriority(const std::string& property_name, int32_t prio, int32_t& prio_fallback) 
    {
        if (_config.hasValue())
        {
            try
            {
                // getProperties will throw runtime_error if path is not valid
                auto properties = _config.getValue()->getProperties(FEP3_SERVICE_BUS_CONFIG);
                auto res = properties->setProperty(property_name, std::to_string(prio), properties->getPropertyType(property_name));
                // check result of setter
                if (res) 
                {
                    return;
                }
                else 
                {
                    FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                        a_util::strings::format("Priority property %s could not be set in property node %s of participant %s", property_name.c_str(), FEP3_SERVICE_BUS_CONFIG, _participant_name.c_str()));
                }
            }
            catch (const std::runtime_error& ex)
            {
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                    a_util::strings::format("Runtime exception - %s : Priority property %s is not found in property node %s of participant %s. "
                        "It will be stored in system control instance", ex.what(), property_name.c_str(), FEP3_SERVICE_BUS_CONFIG, _participant_name.c_str()));

                prio_fallback= prio;
                return;
            }
            // Property not found for both new and old versions
            FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                a_util::strings::format("Priority property %s is not found in property node %s of participant %s", property_name.c_str(), FEP3_SERVICE_BUS_CONFIG, _participant_name.c_str()));
        }
        else 
        {
            FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                a_util::strings::format("Priority property %s of property node %s for participant %s set locally in system control instance", property_name.c_str(), FEP3_SERVICE_BUS_CONFIG, _participant_name.c_str()));

            prio_fallback= prio;
        }
    }

    int32_t getPriority(const std::string& property_name, const int32_t& prio_fallback) const {
        if (_config.hasValue())
        {
            try
            {
                // getProperties will throw runtime_error if path is not valid
                auto properties = _config.getValue()->getProperties(FEP3_SERVICE_BUS_CONFIG);
                auto res = properties->getProperty(property_name);
                if (res != "") 
                {
                    return std::stoi(res);
                }
            }
            catch (const std::invalid_argument& ex)  // invalid return value to convert
            {
                const std::string error_message = a_util::strings::format("Invalid argument exception - %s : "
                    "Priority property %s has an invalid return value from configuration service; logged in %s - %s - line: %s", ex.what(), property_name.c_str(), 
                    A_UTIL_CURRENT_FUNCTION, __FILE__, std::to_string(__LINE__).c_str());

                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::fatal, error_message);

                throw std::invalid_argument(error_message);

            }
            catch (const std::runtime_error& ex) // node not found in old version of participant
            {
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                    a_util::strings::format("Runtime exception - %s : Priority property %s is not found in property node %s of participant %s. "
                    "It will be stored in system control instance", ex.what(), property_name.c_str(), FEP3_SERVICE_BUS_CONFIG, _participant_name.c_str()));

                return prio_fallback;
            }
                
            // Property not found for both new and old versions
            FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                a_util::strings::format("Priority property %s is not found in property node %s of participant %s", property_name.c_str(), FEP3_SERVICE_BUS_CONFIG, _participant_name.c_str()));
        }
        else 
        {
            FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                a_util::strings::format("Priority property %s of property node %s of participant %s retrieved locally from system control instance", property_name.c_str(), FEP3_SERVICE_BUS_CONFIG, _participant_name.c_str()));

            return prio_fallback;
        }
    }

    std::unique_ptr<fep3::rpc::StateMachineProxyFactory> createStateMachineProxy() const
    {
        if (_info.hasValue())
        {
            RPCComponent<ConnectParticipantInfo>  info = _info.getValue();
            return std::make_unique<fep3::rpc::StateMachineProxyFactory>(info->getRPCComponentIIDs("participant_statemachine"),
                "participant_statemachine");

        }
        else
        {
            FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                a_util::strings::format("Participant %s is unreachable - RPC communication to retrieve RPC service with '%s' failed. "
                    "Participant state machine is instantiated with '%s'", _participant_name.c_str(), ConnectParticipantInfo::getRPCIID(), ConnectStateMachine::getRPCIID()));

            return std::make_unique<fep3::rpc::StateMachineProxyFactory>(std::vector<std::string>{}, 
                "participant_statemachine");
        }
    }

    std::shared_ptr<ISystemLogger> getSystemLogger() const
    {
        return _system_logger;
    }

    std::string getSystemName() const
    {
        return _system_access->getName();
    }

    std::string _rpc_server_url;
    std::shared_ptr<ISystemLogger> _system_logger;
    std::string _participant_name;
    std::string _system_name;
    std::string _participant_url;
    int32_t _init_priority = 0;
    int32_t _start_priority = 0;

    mutable InfoCache _info_cache;
    mutable RPCComponentCache<ConnectParticipantInfo>    _info;
    mutable RPCComponentCache<ConnectStateMachine>       _state_machine;
    mutable RPCComponentCache<ConnectLoggingSinkService> _logging;
    bool _registered_logging{ false };
    mutable RPCComponentCache<ConnectConfigurationService> _config;
    mutable RPCComponentCache<ConnectHealthService> _health;
    mutable RPCComponentCache<ConnectHttpServer> _http_server;


    std::chrono::milliseconds _default_timeout;
    std::map<std::string, std::string> _additional_info;
    //we need to make sure the service bus connection lives as locg the system access is used
    ServiceBusWrapper _service_bus_wrapper;
    std::shared_ptr<fep3::IServiceBus::ISystemAccess> _system_access;
    std::unique_ptr<ParticipantHealthListener> _participant_health_Listener;
    bool _health_listener_running = true;
    bool _participant_proxy_not_reachable = false;
    mutable std::unique_ptr <fep3::rpc::StateMachineProxyFactory> _state_machine_proxy_factory;
};

}
