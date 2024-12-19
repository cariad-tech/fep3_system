
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fep_system/rpc_services/participant_statemachine/participant_statemachine_rpc_intf.h"

#include "fep_system_state_transition_helper.h"

#include <boost/range/adaptors.hpp>

#include <map>
#include <vector>

namespace fep3 {
class ISystemLogger;

struct ThrowOnErrorPolicy {
    bool operator()(std::vector<TransitionFunctionMetaData>& transition_meta_data);
};

struct StateTransitionErrorHandling {
    StateTransitionErrorHandling(std::shared_ptr<fep3::ISystemLogger> logger,
                                 const std::string& system_name);

    void checkParticipantsBeforeTransition(
        const std::map<std::string, fep3::rpc::ParticipantState>& part_states_map);
    void throwOnInvalidTargetState(fep3::rpc::ParticipantState target_state,
                          const std::string& target_state_name);

private:
    std::shared_ptr<fep3::ISystemLogger> _logger;
    const std::string _system_name;
};

/// @brief This class ensures the correct logging message is printed at the end of the transition
/// If the struct is destructed before the success flag was set, this means an exception
/// was thrown and the destructor was called during stack unwinding.
/// Otherwise if the destructor was called after flag was set, no exception occurred.
struct TransitionSuccess {
    const std::string _system_name;
    const std::string _final_state_name;
    std::shared_ptr<fep3::ISystemLogger> _logger;
    bool _success = false;
    ~TransitionSuccess();

    TransitionSuccess(const TransitionSuccess&) = delete;
    TransitionSuccess(TransitionSuccess&&) = delete;
    TransitionSuccess& operator=(const TransitionSuccess&) = delete;
    TransitionSuccess& operator=(TransitionSuccess&&) = delete;
};
} // namespace fep3