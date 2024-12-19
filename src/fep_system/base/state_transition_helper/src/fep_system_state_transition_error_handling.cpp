
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fep_system_state_transition_error_handling.h"
#include "fep_system_state_transition_helper.h"
#include "fep_system/system_logger_intf.h"
#include "system_logger_helper.h"

#include <a_util/strings.h>

#include <boost/algorithm/cxx11/any_of.hpp>

namespace fep3 {
bool ThrowOnErrorPolicy::operator()(std::vector<TransitionFunctionMetaData>& transition_meta_data)
{
    auto transition_meta_data_with_errors =
        boost::adaptors::filter(transition_meta_data, [](const TransitionFunctionMetaData& data) {
            return data.hasError();
        });

    if (boost::empty(transition_meta_data_with_errors))
        return true;
    else {
        std::string execption_result = "Errors during state transition ";

        for (const auto& data: transition_meta_data_with_errors) {
            execption_result.append(data._exec_result._error_description + " ");
        }

        throw std::runtime_error(execption_result);
    }
}

StateTransitionErrorHandling::StateTransitionErrorHandling(
    std::shared_ptr<fep3::ISystemLogger> logger, const std::string& system_name)
    : _logger(logger), _system_name(system_name)
{
}

void StateTransitionErrorHandling::checkParticipantsBeforeTransition(
    const std::map<std::string, fep3::rpc::ParticipantState>& part_states_map)
{
    using SystemState = fep3::rpc::ParticipantState;
    auto part_states = part_states_map | boost::adaptors::map_values;

    auto is_a_participant_unreachable =
        boost::algorithm::any_of(part_states, [](const auto& state) -> bool {
            return SystemState::unreachable == state || SystemState::undefined == state;
        });

    if (part_states_map.empty())
        FEP3_SYSTEM_LOG_AND_THROW(
            _logger,
            LoggerSeverity::error,
            a_util::strings::format("No participant in system '%s' is reachable.",
                                    _system_name.c_str()));
    if (is_a_participant_unreachable)
        FEP3_SYSTEM_LOG_AND_THROW(
            _logger,
            LoggerSeverity::error,
            a_util::strings::format("At least one participant in system '%s' is not reachable.",
                                    _system_name.c_str()));
}

void StateTransitionErrorHandling::throwOnInvalidTargetState(
    fep3::rpc::ParticipantState target_state, const std::string& target_state_name)
{
    using SystemState = fep3::rpc::ParticipantState;
    if (SystemState::undefined == target_state) {
        FEP3_SYSTEM_LOG_AND_THROW(
            _logger,
            LoggerSeverity::error,
            a_util::strings::format(
                "Call setSystemState at system %s with invalid value for argument 'state': '%s' ",
                _system_name.c_str(),
                target_state_name.c_str()));
    }
}

TransitionSuccess::~TransitionSuccess()
{
    if (_success) {
        FEP3_SYSTEM_LOG(
            _logger,
            fep3::LoggerSeverity::info,
            a_util::strings::format("System '%s' transition to state '%s' completed successfully",
                                    _system_name.c_str(),
                                    _final_state_name.c_str()));
    }
    else {
        FEP3_SYSTEM_LOG(_logger,
                        fep3::LoggerSeverity::warning,
                        a_util::strings::format("Cannot set homogenous state of the system '%s'",
                                                _system_name.c_str()));
    }
}

} // namespace fep3