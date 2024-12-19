
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fep_state_transition_controller.h"

#include "fep_system_state_tree.h"

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/minmax_element.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include <assert.h>

using ParticipantState = fep3::rpc::ParticipantState;

namespace fep3 {

ParticipantState TransitionController::participantStateToTrigger(
    const std::map<std::string, ParticipantState>& part_states_map,
    ParticipantState target_state)
{
    const auto state_sorting = getParticipantStateSorting(part_states_map, target_state);
    return getAggregatedState(part_states_map, state_sorting);
}

ParticipantState TransitionController::getNextParticipantsState(
    ParticipantState state_to_transition_from, ParticipantState target_state)
{
    // get the state path the participants should transition
    const std::list<ParticipantState> transition_list =
        getStateTransitionPath(state_to_transition_from, target_state);
    // means the state we want to reach is the one we are.
    if (transition_list.size() == 1)
        return target_state;
    // should be called only for reachable states
    //  this cannot happen in production since when setting state to unreachable
    //  or from unreachable, the error handling kicks in.
    if (transition_list.empty())
        return ParticipantState::unreachable;
    // check that the algo returned as first elements the actual state
    assert(*transition_list.begin() == state_to_transition_from);
    ParticipantState state_to_transition_to = *std::next(transition_list.begin());
    return state_to_transition_to;
}

bool TransitionController::homogeneousTargetStateAchieved(
    const std::map<std::string, ParticipantState>& participant_states,
    ParticipantState target_state)
{
    if (participant_states.empty())
        return false;

    const auto part_states = participant_states | boost::adaptors::map_values;

    return boost::algorithm::all_of(
        part_states, [&target_state](const auto& state) -> bool { return target_state == state; });
}

TransitionController::StateComparisonFunction TransitionController::getParticipantStateSorting(
    const ParticipantStates& part_states, ParticipantState target_state)
{
    using StartFromLowestState = std::less<ParticipantState>;
    using StartFromHighestState = std::greater<ParticipantState>;

    const auto states = boost::adaptors::values(part_states);
    const auto[min_state, max_state] =
        boost::minmax_element(states.begin(), states.end(), StartFromLowestState{});

    // all states smaller or equal than the target state
    if (*max_state <= target_state) {
        return StartFromLowestState{};
    }
    // all states larger or equal than the target state
    else if (target_state <= *min_state) {
        return StartFromHighestState{};
    }
    // mixed
    else {
        return StartFromHighestState{};
    }
}

ParticipantState TransitionController::getAggregatedState(
    const ParticipantStates& part_states_map, StateComparisonFunction state_comparison)
{
    if (part_states_map.empty())
        return ParticipantState::unreachable;

    const auto part_states = part_states_map | boost::adaptors::map_values;
    return *boost::min_element(part_states, state_comparison);
}

} // namespace fep3
