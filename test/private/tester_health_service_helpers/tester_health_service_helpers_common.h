/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "participant_health_aggregator.h"
#include <gtest/gtest.h>


using namespace std::chrono_literals;

struct TesterHealthServiceHelpersCommon : public testing::Test
{
protected:

    void check_execute_error_equality(const fep3::JobHealthiness::ExecuteError& lhs, const fep3::JobHealthiness::ExecuteError& rhs)
    {
        ASSERT_EQ(lhs.error_count, rhs.error_count);
        ASSERT_EQ(lhs.simulation_time, rhs.simulation_time);
        ASSERT_EQ(lhs.last_error.error_code, rhs.last_error.error_code);
    }

    void check_healthiness_equality(const fep3::JobHealthiness& lhs, const fep3::JobHealthiness& rhs)
    {
        ASSERT_EQ(lhs.job_name, rhs.job_name);
        ASSERT_EQ(std::get<fep3::JobHealthiness::ClockTriggeredJobInfo>(lhs.job_info).cycle_time, std::get<fep3::JobHealthiness::ClockTriggeredJobInfo>(rhs.job_info).cycle_time);
        ASSERT_EQ(lhs.simulation_time, rhs.simulation_time);
        ASSERT_NO_FATAL_FAILURE(check_execute_error_equality(lhs.execute_data_in_error, rhs.execute_data_in_error));
        ASSERT_NO_FATAL_FAILURE(check_execute_error_equality(lhs.execute_error, rhs.execute_error));
        ASSERT_NO_FATAL_FAILURE(check_execute_error_equality(lhs.execute_data_out_error, rhs.execute_data_out_error));
    }

    const std::string _job_name1{ "test_job_1" };
    const std::string _job_name2{ "test_job_2" };

    const std::chrono::nanoseconds  _cycle_time{ 2ms };
    std::chrono::nanoseconds _simulation_time = std::chrono::nanoseconds(20ns);
    fep3::JobHealthiness::ExecuteError _execute_data_in_error = { 22, 40ns, -1 };
    fep3::JobHealthiness::ExecuteError _execute_error = { 33, 10ns, -2 };
    fep3::JobHealthiness::ExecuteError _execute_data_out_error = { 11, 0ns, -3 };

    fep3::JobHealthiness _job_healthiness_1{ _job_name1, fep3::JobHealthiness::ClockTriggeredJobInfo{_cycle_time}, 34ns, _execute_data_in_error, _execute_error,  _execute_data_out_error };
    fep3::JobHealthiness _job_healthiness_2{ _job_name1, fep3::JobHealthiness::ClockTriggeredJobInfo{_cycle_time}, 34ns, _execute_data_in_error, _execute_error,  _execute_data_out_error };
};
