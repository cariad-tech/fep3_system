/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fep_system/fep_system.h>
#include "participant_configuration_helper.hpp"
#include <fep_test_common.h>
#include <fep_system/mock_event_monitor.h>
#include <gtest_asserts.h>
#include <fep_system/rpc_services/rpc_passthrough/rpc_passthrough_intf.h>

/**
 * @brief Experimental fep3::rpc::experimental::IRPCPassthrough test
 */
TEST(ParticipantConfiguration, TestRPCPassthoughInterface)
{
    using namespace fep3;

    constexpr const char* participant_name = "Participant1_configuration_test";
    

    System system_under_test(makePlatformDepName("Blackbox"));

    auto parts = createTestParticipants({ participant_name }, system_under_test.getSystemName());
    system_under_test.add(participant_name);

    system_under_test.load();
    system_under_test.initialize();

    auto participant = system_under_test.getParticipant(participant_name);

    fep3::rpc::RPCClient<fep3::rpc::experimental::IRPCPassthrough> rpc_passthrough;

    ASSERT_TRUE(participant.getRPCComponentProxy("data_registry", fep3::rpc::getRPCIID<fep3::rpc::experimental::IRPCPassthrough>(), rpc_passthrough));
    std::string reponse;
    rpc_passthrough->call("{\"jsonrpc\": \"2.0\", \"method\": \"getSignalInNames\", \"params\": [], \"id\": 1}", reponse);

    // The json answer should look like this "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"reader_1\"}". 
    // We are only interested in the correct result "reader_1"
    EXPECT_TRUE(reponse.find("\"result\":\"reader_1\"") != std::string::npos);
}

