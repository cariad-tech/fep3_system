/**
* Copyright 2023 CARIAD SE. 
*
* This Source Code Form is subject to the terms of the Mozilla
* Public License, v. 2.0. If a copy of the MPL was not distributed
* with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#include <fep3/rpc_services/base/fep_rpc_result_to_json.h>
#include "rpc_services/participant_statemachine_proxy.hpp"
#include <gmock/gmock.h>

using namespace ::testing;

TEST(RPCParticipantStateMachineResultTester, validate_failRPCResultJsonObject)
{
    fep3::rpc::catelyn::RPCRequestResultParticipantStateMachine rpc_result;
    std::string transition = "load";
    fep3::Result result(fep3::ERR_FAILED, std::string("state machine " + transition + " denied").c_str(),  
        0, "dummy_file.x", "dummy_function");

    bool caught = false;

    try {
        rpc_result.validate(transition, fep3::rpc::arya::resultToJson(result));
        FAIL();
    }
    catch (std::logic_error e) {
        caught = true;
    }
    catch (...) {
        FAIL();
    }
    ASSERT_TRUE(caught);
}

TEST(RPCParticipantStateMachineResultTester, validate_failRPCResultInvalidJsonObject)
{
    fep3::rpc::catelyn::RPCRequestResultParticipantStateMachine rpc_result;
    std::string transition = "load";
    
    {
        Json::Value result{};
        EXPECT_THROW(rpc_result.validate(transition, result), std::exception);
    }
    {
        Json::Value result; // null
        EXPECT_THROW(rpc_result.validate(transition, result), std::exception);
    }
    {
        Json::Value result; 
        result["invalid"] = "";
        EXPECT_THROW(rpc_result.validate(transition, result), std::exception);
    }
}

TEST(RPCParticipantStateMachineResultTester, validate_successRPCResultJsonObject) {
    fep3::rpc::catelyn::RPCRequestResultParticipantStateMachine rpc_result;
    std::string transition = "load";
    fep3::Result result{};

    EXPECT_NO_THROW(rpc_result.validate(transition, fep3::rpc::arya::resultToJson(result)));
}
