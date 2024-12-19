/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#pragma once
#include <string>
#include <functional>

#include <components/service_bus/rpc/fep_rpc_stubs_client.h>
#include <fep_system_stubs/participant_statemachine_proxy_stub.h>
#include <fep_system_stubs/participant_statemachine_proxy_stub_arya.h>

#include "rpc_services/participant_statemachine/participant_statemachine_rpc_intf.h"
#include "rpc_services/base/fep_rpc_json_to_result.h"

namespace
{
    const std::map<std::string, fep3::rpc::IRPCParticipantStateMachine::State>& getStateMap()
    {
        static const std::map<std::string, fep3::rpc::IRPCParticipantStateMachine::State> state_map = {
            {"Loaded",      fep3::rpc::IRPCParticipantStateMachine::State::loaded},
            {"Initialized", fep3::rpc::IRPCParticipantStateMachine::State::initialized},
            {"Paused",      fep3::rpc::IRPCParticipantStateMachine::State::paused},
            {"Unloaded",    fep3::rpc::IRPCParticipantStateMachine::State::unloaded},
            {"Running",     fep3::rpc::IRPCParticipantStateMachine::State::running}
        };

        return state_map;
    }

    fep3::rpc::IRPCParticipantStateMachine::State fromStateMap(const std::string& value)
    {
        auto ret_val = getStateMap().find(value);
        if (ret_val != getStateMap().end())
        {
            return ret_val->second;
        }
        else
        {
            return fep3::rpc::IRPCParticipantStateMachine::State::unreachable;
        }

    }
}

namespace fep3
{
namespace rpc
{
//we use the namesapce here to create versiones Proxies if something changed in future
namespace catelyn
{
    class RPCRequestResultParticipantStateMachine {
    public:
        void validate(const std::string& transition, const Json::Value& rpc_result) {
                // fep3::catelyn::ParticipantStateMachine RPC interface
                auto result = fep3::rpc::arya::jsonToResult(rpc_result);
                if(!result){
                    throw std::logic_error(a_util::strings::format("state machine '%s' denied. RPC service '%s' %s() returns: Error: %s - %s : occured in: %s - %s line: %s", 
                        transition.c_str(), transition.c_str(), catelyn::IRPCParticipantStateMachineDef::getRPCIID(), std::to_string(result.getErrorCode()).c_str(), result.getDescription(), result.getFunction(), result.getFile(), std::to_string(result.getLine()).c_str()));
                }
        }
    };


class ParticipantStateMachineProxy : public RPCServiceClientProxy< rpc_proxy_stub::catelyn::RPCStateMachineProxy,
    IRPCParticipantStateMachine>
{
private:
    typedef rpc::RPCServiceClientProxy< rpc_proxy_stub::catelyn::RPCStateMachineProxy,
        IRPCParticipantStateMachine > base_type;


    Json::Value changeState(const std::string& transition, std::function<Json::Value()> call) {
        try { 
            return call();
        }  catch(const std::exception& ex) { 
            throw std::runtime_error("error while " + transition  + ": " + ex.what()); 
        }
    }

    RPCRequestResultParticipantStateMachine _rpc_result;

public:
    using base_type::GetStub;
    ParticipantStateMachineProxy(
        std::string rpc_component_name,
        std::shared_ptr<IRPCRequester> rpc) :
        base_type(rpc_component_name, rpc)
    {

    }

    IRPCParticipantStateMachine::State getState() const
    {
        try
        {
            return fromStateMap(GetStub().getCurrentStateName());
        }
        catch (...)
        {
            return rpc::IRPCParticipantStateMachine::State::unreachable;
        }
    }
    void load() override
    {
        Json::Value value = changeState( "loading", [&](){return GetStub().load(); });
        _rpc_result.validate("load", value);
    }

    void unload() override
    {
        Json::Value value = changeState("unloading", [&]() {return GetStub().unload(); });
        _rpc_result.validate("unload", value);
    }

    void initialize() override
    {
        Json::Value value = changeState("initializing", [&]() {return GetStub().initialize(); });
        _rpc_result.validate("initialize", value);
    }
    void deinitialize() override
    {
        Json::Value value = changeState("deinitializing", [&]() {return GetStub().deinitialize(); });
        _rpc_result.validate("deinitialize", value);
    }
    void start() override
    {
        Json::Value value = changeState("starting", [&]() {return GetStub().start(); });
        _rpc_result.validate("start", value);
    }
    void stop() override
    {
        Json::Value value = changeState("stopping", [&]() {return GetStub().stop(); });
        _rpc_result.validate("stop", value);
    }
    void pause() override
    {
        Json::Value value = changeState("pausing", [&]() {return GetStub().pause(); });
        _rpc_result.validate("pause", value);
    }

    void shutdown() override
    {
        Json::Value value = changeState("shutdowning", [&]() {return GetStub().exit(); });
        _rpc_result.validate("shutdown", value);
    }

};
} // namespace catelyn

namespace arya
{
    class ParticipantStateMachineProxy : public RPCServiceClientProxy< rpc_proxy_stub::arya::RPCStateMachineProxy,
        IRPCParticipantStateMachine>
    {
    private:
        typedef rpc::RPCServiceClientProxy< rpc_proxy_stub::arya::RPCStateMachineProxy,
            IRPCParticipantStateMachine > base_type;

        bool changeState(const std::string& transition, std::function<bool()> call) {
            try {
                return call();
            }
            catch (const std::exception& ex) {
                throw std::runtime_error("error while " + transition + ": " + ex.what());
            }
        }

    public:
        using base_type::GetStub;
        ParticipantStateMachineProxy(
            std::string rpc_component_name,
            std::shared_ptr<IRPCRequester> rpc) :
            base_type(rpc_component_name, rpc)
        {

        }

        IRPCParticipantStateMachine::State getState() const
        {
            try
            {
                return fromStateMap(GetStub().getCurrentStateName());
            }
            catch (...)
            {
                return rpc::IRPCParticipantStateMachine::State::unreachable;
            }
        }
        void load() override
        {
            bool change_succeeded =
                changeState("loading", [&]() { return GetStub().load(); });
            if (!change_succeeded)
            {
                throw std::logic_error(a_util::strings::format("state machine 'load' denied. RPC service '%s' load() returns: 'false'", arya::IRPCParticipantStateMachineDef::getRPCIID()));
            }
        }
        void unload() override
        {
            bool change_succeeded =
                changeState("unloading", [&]() { return GetStub().unload(); });
            if (!change_succeeded)
            {
                throw std::logic_error(a_util::strings::format("state machine 'unload' denied. RPC service '%s' unload() returns: 'false'", arya::IRPCParticipantStateMachineDef::getRPCIID()));
            }
        }

        void initialize() override
        {
            bool change_succeeded =
                changeState("initializing", [&]() { return GetStub().initialize(); });
            if (!change_succeeded)
            {
                throw std::logic_error(a_util::strings::format("state machine 'initialize' denied. RPC service '%s' initialize() returns: 'false'", arya::IRPCParticipantStateMachineDef::getRPCIID()));
            }
        }
        void deinitialize() override
        {
            bool change_succeeded =
                changeState("deinitializing", [&]() { return GetStub().deinitialize(); });
            if (!change_succeeded)
            {
                throw std::logic_error(a_util::strings::format("state machine 'deinitialize' denied. RPC service '%s' deinitialize() returns: 'false'", arya::IRPCParticipantStateMachineDef::getRPCIID()));
            }
        }
        void start() override
        {
            bool change_succeeded =
                changeState("starting", [&]() { return GetStub().start(); });
            if (!change_succeeded)
            {
                throw std::logic_error(a_util::strings::format("state machine 'start' denied. RPC service '%s' start() returns: 'false'", arya::IRPCParticipantStateMachineDef::getRPCIID()));
            }
        }
        void stop() override
        {
            bool change_succeeded =
                changeState("stopping", [&]() { return GetStub().stop(); });
            if (!change_succeeded)
            {
                throw std::logic_error(a_util::strings::format("state machine 'stop' denied. RPC service '%s' stop() returns: 'false'", arya::IRPCParticipantStateMachineDef::getRPCIID()));
            }
        }
        void pause() override
        {
            bool change_succeeded =
                changeState("pausing", [&]() { return GetStub().pause(); });
            if (!change_succeeded)
            {
                throw std::logic_error(a_util::strings::format("state machine 'pause' denied. RPC service '%s' pause() returns: 'false'", arya::IRPCParticipantStateMachineDef::getRPCIID()));
            }
        }

        void shutdown() override
        {
            bool change_succeeded =
                changeState("shutdowning", [&]() { return GetStub().exit(); });
            if (!change_succeeded)
            {
                throw std::logic_error(a_util::strings::format("state machine 'shutdown' denied. RPC service '%s' shutdown() returns: 'false'", arya::IRPCParticipantStateMachineDef::getRPCIID()));
            }
        }

    };
} // namespace arya

class StateMachineProxyFactory
{
public:
    StateMachineProxyFactory(const std::vector<std::string>& state_machine_interfaces,
        const std::string& component_name) 
        : _component_name(component_name)
    {
        if (state_machine_interfaces.empty())
        {
            // we should still return something, in case a function is called
            // the error that the service is unreachable will be reported
            _interface_iid = fep3::rpc::catelyn::IRPCParticipantStateMachineDef::getRPCIID();
        }
        else
        {
            _interface_iid = state_machine_interfaces.front();
        }
    }

    std::shared_ptr<rpc::arya::IRPCServiceClient> getProxy(
        std::shared_ptr<IRPCRequester> rpc) const
    {
        if (_interface_iid == fep3::rpc::arya::IRPCParticipantStateMachineDef::getRPCIID())
        {
            return std::make_shared<rpc::arya::ParticipantStateMachineProxy>(
                _component_name,
                rpc);
        }
        else if (_interface_iid == fep3::rpc::catelyn::IRPCParticipantStateMachineDef::getRPCIID())
        {
            return std::make_shared<rpc::catelyn::ParticipantStateMachineProxy>(
                _component_name,
                rpc);
        }
        else
        {
            //exception or error?
            throw std::runtime_error("Cannot create RPC Client for state machine,State Machine RPC server returned unsupported IID " + _interface_iid);
        }
    }
private:
    const std::string _component_name;
    std::string _interface_iid;
};

}
}
