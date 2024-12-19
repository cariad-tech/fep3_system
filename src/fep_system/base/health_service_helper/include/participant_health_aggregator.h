/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "fep_system/healthiness_types.h"
#include <tuple>
#include <map>

namespace fep3
{
    class ParticipantHealthStateAggregator
    {
    public:
        ParticipantHealthStateAggregator(std::chrono::nanoseconds liveliness_timeout);

        void setParticipantHealth(const std::string& participant_name, ParticipantHealthUpdate jobs_healthiness);

        std::map<std::string, ParticipantHealth> getParticipantsHealth(const std::chrono::time_point<std::chrono::steady_clock>& current_system_time);

    private:
        ParticipantRunningState getParticipantRunningState(
            const std::chrono::time_point<std::chrono::steady_clock>& current_system_time,
            const std::chrono::time_point<std::chrono::steady_clock>& last_update_time);

        const std::chrono::nanoseconds _liveliness_timeout;
        std::map<std::string, ParticipantHealthUpdate> _participant_health;
    };
}
