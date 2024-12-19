/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "execution_policy.h"
#include "fep_system.h"
#include "fep_system_state_transition.h"
#include "participant_proxy.h"
#include "system_logger_intf.h"

namespace fep3 {
std::string toString(const fep3::rpc::ParticipantState& participant_state);

struct ExecutionConfig {
    fep3::System::InitStartExecutionPolicy _policy =
        fep3::System::InitStartExecutionPolicy::parallel;
    uint8_t _thread_count = 4;
};

struct ProxyStateTransition {
    ProxyStateTransition(void (fep3::rpc::IRPCParticipantStateMachine::*transition_function)(),
                         std::shared_ptr<ISystemLogger> system_logger,
                         StateInfo state_info,
                         const std::string& system_name);

    ExecResult operator()(ParticipantProxy& proxy) const;

private:
    void (fep3::rpc::IRPCParticipantStateMachine::*_transition_function)();
    std::shared_ptr<ISystemLogger> _system_logger;
    const std::string _state_transition_name;
    const std::string _system_name;
};
} // namespace fep3
