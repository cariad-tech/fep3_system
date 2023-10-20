/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2022 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */
#include "participant_health_aggregator.h"
#include <stdexcept>

#include <boost/bimap.hpp>
#include <boost/assign.hpp>

namespace fep3
{

    ParticipantHealthStateAggregator::ParticipantHealthStateAggregator(std::chrono::nanoseconds liveliness_timeout)
        : _liveliness_timeout(std::move(liveliness_timeout))
    {}

    void ParticipantHealthStateAggregator::setParticipantHealth(const std::string& participant_name, ParticipantHealthUpdate jobs_healthiness)
    {
        _participant_health[participant_name] = std::move(jobs_healthiness);
    }

    std::map<std::string, ParticipantHealth> ParticipantHealthStateAggregator::getParticipantsHealth(const std::chrono::time_point<std::chrono::steady_clock>& current_system_time)
    {
        std::map<std::string, ParticipantHealth> participants_health;
        for (const auto& [participant_name, participant_health_update] : _participant_health)
        {
            participants_health[participant_name] = ParticipantHealth{
                getParticipantRunningState(current_system_time, participant_health_update.system_time),
                participant_health_update.jobs_healthiness
            };
        }

        return participants_health;
    }

#pragma warning(push)
#pragma warning(disable: 4834 4996)
    namespace
    {
        using bimapParticipantRunningState = boost::bimap<ParticipantRunningState, std::string>;
        const bimapParticipantRunningState participant_running_state =
            boost::assign::list_of<bimapParticipantRunningState::relation>
            (ParticipantRunningState::offline, "offline")
            (ParticipantRunningState::online, "online");
    }

    ParticipantRunningState ParticipantHealthStateAggregator::getParticipantRunningState(
        const std::chrono::time_point<std::chrono::steady_clock>& current_system_time,
        const std::chrono::time_point<std::chrono::steady_clock>& last_update_time)
    {
        if ((current_system_time - last_update_time) <= _liveliness_timeout)
        {
            return ParticipantRunningState::online;
        }
        else
        {
            return ParticipantRunningState::offline;
        }
    }

    ParticipantRunningState participantRunningStateFromString(const std::string& state_string)
    {
        auto it = participant_running_state.right.find(state_string);
        if (it != participant_running_state.right.end())
        {
            return it->second;
        }
        throw std::domain_error("ParticipantRunningState must be either offline or online, " + state_string + " was given for conversion.");
    }

    std::string participantRunningStateToString(const ParticipantRunningState state)
    {
        auto it = participant_running_state.left.find(state);
        if (it != participant_running_state.left.end())
        {
            return it->second;
        }
        return "NOT RESOLVABLE";
    }
#pragma warning(pop)
}
