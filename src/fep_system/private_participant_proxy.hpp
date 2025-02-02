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


#pragma once
#include <string>
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
#include <math.h>

namespace fep3
{

struct ParticipantProxy::Implementation
{
public:
    //this is the current type used within connect()
    //usually this type must be supported as lowest versioned types forever !
    typedef rpc::arya::IRPCParticipantInfo ConnectParticipantInfo;
    typedef rpc::arya::IRPCParticipantStateMachine ConnectStateMachine;
    typedef rpc::arya::IRPCLoggingSinkService ConnectLoggingSinkService;
    typedef rpc::arya::IRPCConfiguration ConnectConfigurationService;
    typedef rpc::catelyn::IRPCHealthService ConnectHealthService;
    typedef rpc::catelyn::IRPCHttpServer ConnectHttpServer;

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
                    return {};
                }
            }
            catch (...)
            {
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
            _last_state = ConnectStateMachine::State::undefined;
        }

    private:
        std::map<std::string, std::vector<std::string>> _found_components_byiid;
        std::recursive_mutex _sync;
        ConnectStateMachine::State _last_state{ ConnectStateMachine::State::undefined};
    };

    Implementation(const std::string& participant_name,
        const std::string& participant_url,
        const std::string& system_name,
        const std::string& system_discovery_url,
        std::shared_ptr<ISystemLogger> logger,
        std::chrono::milliseconds default_timeout) :
        _participant_name(participant_name),
        _participant_url(participant_url),
        _logger(logger),
        _init_priority(0),
        _start_priority(0),
        _default_timeout(default_timeout),
        _info(this),
        _state_machine(this),
        _logging(this),
        _config(this),
        _health(this),
        _http_server(this),
        _health_listener_running(true),
        _service_bus_wrapper(getServiceBusWrapper())
    {

        _system_access = _service_bus_wrapper.createOrGetServiceBusConnection(system_name, system_discovery_url)->getSystemAccessCatelyn(system_name);

        if (!_system_access)
        {
            throw std::runtime_error(std::string("While contructing ") + participant_name + " at " + participant_url
                + "no system connection to " + system_name + " at " + system_discovery_url +" possible");
        }
        initHealthListener(system_name);

        _info.getValue();
        //only if info hasValue ... then it makes sense to connect to the others
        //otherwise ther is a huge timeout for every connecting
        if (_info.hasValue())
        {
            _state_machine.getValue();
            auto logging = _logging.getValue();
            if (logging)
            {
                logging->registerRPCClient(_logger->getUrl());
                _registered_logging = true;
            }
        }
    }

    void deregisterLogging()
    {
        // ensures that no logging is performed after the logger is deregistered
        _participant_health_Listener->deactivateLogging();
        if (_registered_logging)
        {
            auto logging = _logging.getValue();
            if (logging)
            {
                logging->unregisterRPCClient(_logger->getUrl());
                _registered_logging = false;
            }
        }
    }

    virtual ~Implementation()
    {
        if (_health_listener_running)
        {
            _system_access->deregisterUpdateEventSink(_participant_health_Listener.get());
        }
        deregisterLogging();
    }

    void copyValuesTo(Implementation& other) const
    {
        other._participant_name = _participant_name;
        other._participant_url = _participant_url;
        other._init_priority = _init_priority;
        other._start_priority = _start_priority;
        other._default_timeout = _default_timeout;
        other._additional_info = _additional_info;
        other._service_bus_wrapper = _service_bus_wrapper;
        other._system_access = _system_access;
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
        _start_priority = prio;
    }

    int32_t getStartPriority() const
    {
        return _start_priority;
    }

    void setInitPriority(int32_t prio)
    {
        _init_priority = prio;
    }

    int32_t getInitPriority() const
    {
        return _init_priority;
    }

    bool getRPCComponentProxy(const std::string& component_name,
        const std::string& component_iid,
        IRPCComponentPtr& proxy_ptr) const
    {
        //this is very special and must be handled separately
        auto requester = _system_access->getRequester(_participant_name);
        if (requester == nullptr)
        {
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
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::ParticipantStateMachineProxy>(
                component_name,
                requester);
            return proxy_ptr.reset(part_object);
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
        else if (component_iid == fep3::rpc::getRPCIID<rpc::arya::IRPCParticipantStateMachine>())
        {
            std::shared_ptr<rpc::arya::IRPCServiceClient> part_object = std::make_shared<rpc::arya::ParticipantStateMachineProxy>(
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
                _participant_name,
                *_logger);
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
                *_logger);
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
        std::vector<std::string> found_objects;
        std::vector<std::string> found_objects_which_supports;
        RPCComponent<ConnectParticipantInfo> info = _info.getValue();
        if (!info)
        {
            std::string err_message = "Participant " + getParticipantName() + " is unreachable";
            _logger->log(
                LoggerSeverity::fatal,
                getParticipantName(),
                _system_access->getName(),
                err_message);
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
            return ConnectStateMachine::State::undefined;
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
            throw std::runtime_error("You cannot get participant health with a deactivated health listener");
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
                    _logger->log(severity, _participant_name, "", message);
                }
            });

        _system_access->registerUpdateEventSink(_participant_health_Listener.get());
    }

    std::shared_ptr<ISystemLogger> _logger;
    std::string _participant_name;
    std::string _participant_url;

    mutable InfoCache _info_cache;
    mutable RPCComponentCache<ConnectParticipantInfo>    _info;
    mutable RPCComponentCache<ConnectStateMachine>       _state_machine;
    mutable RPCComponentCache<ConnectLoggingSinkService> _logging;
    bool _registered_logging{ false };
    mutable RPCComponentCache<ConnectConfigurationService> _config;
    mutable RPCComponentCache<ConnectHealthService> _health;
    mutable RPCComponentCache<ConnectHttpServer> _http_server;


    int32_t _init_priority;
    int32_t _start_priority;
    std::chrono::milliseconds _default_timeout;
    std::map<std::string, std::string> _additional_info;
    //we need to make sure the service bus connection lives as locg the system access is used
    ServiceBusWrapper _service_bus_wrapper;
    std::shared_ptr<fep3::IServiceBus::ISystemAccess> _system_access;
    std::unique_ptr<ParticipantHealthListener> _participant_health_Listener;
    bool _health_listener_running;
};

}
