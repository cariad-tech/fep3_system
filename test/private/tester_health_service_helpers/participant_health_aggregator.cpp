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
#include "tester_health_service_helpers_common.h"

using namespace std::chrono_literals;

struct ParticipantHealthStateAggregatorTest : public TesterHealthServiceHelpersCommon
{
protected:
    void SetUp() override
    {
        _health_state_aggregator.setParticipantHealth(_part_name_1, fep3::ParticipantHealthUpdate{ _system_time, { _job_healthiness_1 , _job_healthiness_2} });
        _health_state_aggregator.setParticipantHealth(_part_name_2, fep3::ParticipantHealthUpdate{ _system_time, {  _job_healthiness_2} });
    }

    const std::string _part_name_1 {"test_participant" };
    const std::string _part_name_2{ "test_participant_2" };

    std::chrono::nanoseconds _liveliness_timeout = 10s;

    const std::chrono::time_point<std::chrono::steady_clock> _system_time = std::chrono::steady_clock::now();

    fep3::ParticipantHealthStateAggregator _health_state_aggregator{ _liveliness_timeout };
};

TEST_F(ParticipantHealthStateAggregatorTest, setParticipantHealth)
{
    std::map<std::string, fep3::ParticipantHealth> parts_health = _health_state_aggregator.getParticipantsHealth(_system_time + 1s);

    ASSERT_EQ(parts_health.size(), 2);
    ASSERT_EQ(parts_health.count(_part_name_1), 1);
    ASSERT_EQ(parts_health.count(_part_name_2), 1);

    ASSERT_EQ(parts_health.at(_part_name_1).running_state, fep3::ParticipantRunningState::online);
    ASSERT_EQ(parts_health.at(_part_name_2).running_state, fep3::ParticipantRunningState::online);

    ASSERT_EQ(parts_health.at(_part_name_1).jobs_healthiness.size(), 2);
    ASSERT_EQ(parts_health.at(_part_name_2).jobs_healthiness.size(), 1);


    ASSERT_NO_FATAL_FAILURE(check_healthiness_equality(parts_health.at(_part_name_1).jobs_healthiness.at(0), _job_healthiness_1));
    ASSERT_NO_FATAL_FAILURE(check_healthiness_equality(parts_health.at(_part_name_1).jobs_healthiness.at(1), _job_healthiness_2));
    ASSERT_NO_FATAL_FAILURE(check_healthiness_equality(parts_health.at(_part_name_2).jobs_healthiness.at(0), _job_healthiness_2));
}

TEST_F(ParticipantHealthStateAggregatorTest, testParticipantOffline)
{
    std::map<std::string, fep3::ParticipantHealth> parts_health = _health_state_aggregator.getParticipantsHealth(_system_time + 2 * _liveliness_timeout);


    ASSERT_EQ(parts_health.at(_part_name_1).running_state, fep3::ParticipantRunningState::offline);
    ASSERT_EQ(parts_health.at(_part_name_2).running_state, fep3::ParticipantRunningState::offline);
}
