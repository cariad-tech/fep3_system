
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "fep_system/rpc_services/participant_statemachine/participant_statemachine_rpc_intf.h"

#include <list>
namespace fep3 {
std::list<fep3::rpc::ParticipantState> getStateTransitionPath(
    fep3::rpc::ParticipantState start_state, fep3::rpc::ParticipantState end_state);
} // namespace fep3
