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

#include <chrono>

struct TestEnvironment : public ::testing::Environment
{
    TestEnvironment
        (bool verbose
        , uint32_t number_of_participants
        , uint32_t job_cycle_time_in_ms
        , uint32_t duration_in_ms
        , uint32_t rpc_request_period_in_ms
        , uint32_t heartbeat_interval_in_ms
        )
        : _verbose(verbose)
        , _number_of_participants(number_of_participants)
        , _job_cycle_time(job_cycle_time_in_ms)
        , _test_duration(duration_in_ms)
        , _rpc_request_period(rpc_request_period_in_ms)
        , _heartbeat_interval(heartbeat_interval_in_ms)
    {}
    bool _verbose{false};
    uint32_t _number_of_participants{0};
    std::chrono::milliseconds _job_cycle_time{0};
    std::chrono::milliseconds _test_duration{0};
    std::chrono::milliseconds _rpc_request_period{0};
    std::chrono::milliseconds _heartbeat_interval{0};
};