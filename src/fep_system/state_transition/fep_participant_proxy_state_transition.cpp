/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fep_participant_proxy_state_transition.h"

#include <a_util/strings.h>

#include "execution_policy.h"
#include "fep_system_state_transition.h"
#include "system_logger_intf.h"
#include "system_logger_helper.h"

namespace fep3 {
std::string toString(const fep3::rpc::ParticipantState& participant_state)
{
    switch (participant_state) {
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

ProxyStateTransition::ProxyStateTransition(
    void (fep3::rpc::IRPCParticipantStateMachine::*transition_function)(),
    std::shared_ptr<ISystemLogger> system_logger,
    StateInfo state_info,
    const std::string& system_name)
    : _transition_function(transition_function),
      _system_logger(system_logger),
      _state_transition_name(state_info.state_transition),
      _system_name(system_name)
{
}

ExecResult ProxyStateTransition::operator()(ParticipantProxy& proxy) const
{
    auto state_machine = proxy.getRPCComponentProxyByIID<rpc::IRPCParticipantStateMachine>();

    if (state_machine) {
        try {
            auto& state_machine_if = state_machine.getInterface();
            (state_machine_if.*_transition_function)();
        }
        catch (const std::exception& ex) {
            auto log_message = a_util::strings::format(
                "Participant '%s' in system '%s' threw exception: '%s'. Participant '%s' could not "
                "be '%s' successfully and remains in state '%s'",
                proxy.getName().c_str(),
                _system_name.c_str(),
                ex.what(),
                proxy.getName().c_str(),
                _state_transition_name.c_str(),
                toString(state_machine->getState()).c_str());

            FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning, log_message);
            return {false, std::move(log_message)};
        }
    }
    else {
        auto log_message = a_util::strings::format(
            "Participant '%s' in system '%s' is unreachable - RPC communication to retrieve RPC "
            "service with '%s' failed. Participant '%s' could not be %s successfully and remains "
            "in state '%s'",
            proxy.getName().c_str(),
            _system_name.c_str(),
            rpc::IRPCParticipantStateMachine::getRPCIID(),
            proxy.getName().c_str(),
            _state_transition_name.c_str(),
            toString(state_machine->getState()).c_str());

        FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning, log_message);
        return {false, std::move(log_message)};
    }
    return {true, ""};
    }
} // namespace fep3
