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


#include <string.h>
#include <gtest/gtest.h>

#include <fep_test_common.h>
#include <fep_system/fep_system.h>
#include <fep3/fep3_errors.h>
#include "fep_system/rpc_services/service_bus/http_server_rpc_intf.h"
#include <gtest_asserts.h>

using namespace fep3;
using namespace std::chrono_literals;

struct SystemHealthTest : public testing::Test
{
    SystemHealthTest()
        : _system_name(makePlatformDepName("test_system"))
    {
    }

    void SetUp() override
    {
        _participants = createTestParticipants(
                {_participant_name_1, _participant_name_2}, 
                _system_name);

        _system = fep3::discoverSystem(
                _system_name, 
                {_participant_name_1, _participant_name_2 }, 
                10s);
    }

    fep3::System _system;
    const std::string _system_name;
    TestParticipants _participants{};
    const std::string _participant_name_1 {"test_participant_1"};
    const std::string _participant_name_2 {"test_participant_2"};
};

/**
 * @brief Test whether the system library sets the correct heartbeat interval
 */
TEST_F(SystemHealthTest, TestParticipantsGetHeartbeatInterval)
{
    const auto participant_proxy = _system.getParticipant(_participant_name_1);
    auto http_server_proxy = participant_proxy.getRPCComponentProxy<fep3::rpc::IRPCHttpServer>();

    ASSERT_EQ(http_server_proxy->getRPCDefaultName(), rpc::getRPCDefaultName<fep3::rpc::IRPCHttpServer>());
    ASSERT_EQ(http_server_proxy->getRPCIID(), rpc::getRPCIID<fep3::rpc::IRPCHttpServerDef>());
    // default heartbeat is 5000ms
    ASSERT_EQ(http_server_proxy->getHeartbeatInterval(), 5000ms);

    // destroy the server
    _participants[_participant_name_1].reset();
    ASSERT_THROW(http_server_proxy->getHeartbeatInterval(), std::runtime_error);
}

/**
 * @brief Test whether the system library gets the correct heartbeat interval
 */
TEST_F(SystemHealthTest, TestParticipantsSetHeartbeatInterval)
{
    const auto participant_proxy = _system.getParticipant(_participant_name_1);
    auto http_server_proxy = participant_proxy.getRPCComponentProxy<fep3::rpc::IRPCHttpServer>();
    ASSERT_EQ(http_server_proxy->getRPCDefaultName(), rpc::getRPCDefaultName<fep3::rpc::IRPCHttpServer>());
    ASSERT_EQ(http_server_proxy->getRPCIID(), rpc::getRPCIID<fep3::rpc::IRPCHttpServer>());
    http_server_proxy->setHeartbeatInterval(100ms);
    ASSERT_EQ(http_server_proxy->getHeartbeatInterval(), 100ms);

    // destroy the server
    _participants[_participant_name_1].reset();
    ASSERT_THROW(http_server_proxy->setHeartbeatInterval(100ms), std::runtime_error);
}

/**
 * @brief Test whether the system library sets the correct heartbeat interval for all participants
 */
TEST_F(SystemHealthTest, TestParticipantsSetHeartbeatIntervalAll)
{
    // set heartbeat for all participants
    _system.setHeartbeatInterval({}, 100ms);
    ASSERT_EQ(_system.getHeartbeatInterval(_participant_name_1), 100ms);
    ASSERT_EQ(_system.getHeartbeatInterval(_participant_name_2), 100ms);

    // set heartbeat for given participant
    _system.setHeartbeatInterval({_participant_name_1}, 50ms);
    ASSERT_EQ(_system.getHeartbeatInterval(_participant_name_1), 50ms);
    ASSERT_EQ(_system.getHeartbeatInterval(_participant_name_2), 100ms);

    // destroy the server
    _participants[_participant_name_1].reset();
    ASSERT_THROW(_system.setHeartbeatInterval({}, 100ms), std::runtime_error);
}

