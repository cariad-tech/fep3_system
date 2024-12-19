/**
 * Copyright 2024 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#include <string.h>

#include <gtest/gtest.h>

#include <fep_test_common.h>
#include <fep_system/fep_system.h>
#include <fep3/fep3_errors.h>
#include <gtest_asserts.h>

using namespace fep3;

struct ServiceBusTest : public testing::Test
{
    ServiceBusTest()
        : _system_name(makePlatformDepName("test_system"))
    {
    }

    void SetUp() override
    {
        using namespace std::chrono_literals;
        _participants = createTestParticipants({ _participant_names }, _system_name);
        _system = fep3::discoverSystem(_system_name, { _participant_names }, 10s);
    }

    fep3::System _system;
    const std::string _system_name;
    TestParticipants _participants{};
    const std::vector<std::string> _participant_names{ "test_participant_1","test_participant_2" };
};

TEST_F(ServiceBusTest, TestServiceBusParticipantShutdown) {

    using namespace std::chrono_literals;

    // force participant's d'tor call by setting the participant state to state 'unreachable' doesn't work here.
    // Participant references are moved to TestParticipants std::map<std::string, std::unique_ptr<PartStruct>>. 

    // erase participant from TestParticipants map to ensure participant's d'tor is called and the byebye notification is sent.
    _participants.erase(_participant_names.front());

    // give some time to receive the byebye notification and remove the participant from the system
    //std::this_thread::sleep_for(1s);

    ASSERT_THROW(_system.getParticipant(_participant_names.front()), std::runtime_error);

}