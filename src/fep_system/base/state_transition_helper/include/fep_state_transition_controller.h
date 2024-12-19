
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once
#include <fep_system/rpc_services/participant_statemachine/participant_statemachine_rpc_intf.h>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include <functional>
#include <map>

namespace fep3 {
class TransitionController {
public:
    using StateComparisonFunction =
        std::function<bool(const fep3::rpc::ParticipantState&, const fep3::rpc::ParticipantState&)>;

    template <typename T>
    std::vector<std::reference_wrapper<T>> getParticipantsInState(
        const std::map<std::string, fep3::rpc::ParticipantState>& part_states_map,
        fep3::rpc::ParticipantState current_state,
        std::vector<std::reference_wrapper<T>>& participants)
    {
        auto part_names_to_transition =
            boost::adaptors::filter(part_states_map,
                                    [current_state](const auto& name_state) {
                                        return name_state.second == current_state;
                                    }) |
            boost::adaptors::map_keys;

        auto part_in_range = [&](const std::reference_wrapper<T>& proxy) {
            return boost::range::find_if(part_names_to_transition, [&](const std::string& name) {
                       return name == proxy.get().getName();
                   }) != boost::end(part_names_to_transition);
        };

        auto parts_to_transition = boost::adaptors::filter(participants, part_in_range);
        std::vector<std::reference_wrapper<T>> _parts;
        for (auto& part_to_transition: parts_to_transition) {
            _parts.push_back(std::ref(part_to_transition));
        }
        return _parts;
    }

    fep3::rpc::ParticipantState participantStateToTrigger(
        const std::map<std::string, fep3::rpc::ParticipantState>& participant_states,
        fep3::rpc::ParticipantState target_state);

    fep3::rpc::ParticipantState getNextParticipantsState(
        fep3::rpc::ParticipantState state_to_transition_from,
        fep3::rpc::ParticipantState target_state);

    bool homogeneousTargetStateAchieved(
        const std::map<std::string, fep3::rpc::ParticipantState>& participant_states,
        fep3::rpc::ParticipantState target_state);

    fep3::rpc::ParticipantState getAggregatedState(
        const std::map<std::string, fep3::rpc::ParticipantState>& part_states_map,
        StateComparisonFunction state_comparison = std::less<fep3::rpc::ParticipantState>());

private:
    using ParticipantStates = std::map<std::string, fep3::rpc::ParticipantState>;

    StateComparisonFunction getParticipantStateSorting(const ParticipantStates& part_states,
                                                       fep3::rpc::ParticipantState target_state);
};

} // namespace fep3
