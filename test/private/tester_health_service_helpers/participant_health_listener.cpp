/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "participant_health_listener.h"
#include "tester_health_service_helpers_common.h"
#include <gmock/gmock.h>

using namespace ::testing;
using namespace std::literals::chrono_literals;

class RpcHealthServiceMock : public fep3::rpc::IRPCHealthService
{
public:
    MOCK_METHOD(std::vector<fep3::JobHealthiness>, getHealth, (), (const, override));
    MOCK_METHOD(fep3::Result, resetHealth, (), (override));
};


struct ParticipantHealthListenerTest : public TesterHealthServiceHelpersCommon
{
    void onLog()
    {
        _logging_called = true;
    }

    ::testing::StrictMock<RpcHealthServiceMock> _rpc_health_service_mock;
    const std::string _participant_name = "test_participant";
    const std::string _system_name = "sytem_name";
    fep3::ParticipantHealthListener _health_listener{ &_rpc_health_service_mock ,_participant_name, _system_name, [&](fep3::LoggerSeverity, const std::string&) {onLog();} };
    std::vector<fep3::JobHealthiness> _jobs_healthiness{ _job_healthiness_1, _job_healthiness_2 };
    bool _logging_called = false;
};

TEST_F(ParticipantHealthListenerTest, updateEvent)
{
    EXPECT_CALL(_rpc_health_service_mock, getHealth()).WillOnce(Return(_jobs_healthiness));

    _health_listener.updateEvent(
        fep3::IServiceBus::ServiceUpdateEvent{
            _participant_name ,
            _system_name,
            "url",
            fep3::IServiceBus::ServiceUpdateEventType::notify_alive});

    auto participant_health = _health_listener.getParticipantHealth();

    ASSERT_EQ(participant_health.jobs_healthiness.size(), 2);
    ASSERT_NO_FATAL_FAILURE(check_healthiness_equality(participant_health.jobs_healthiness.at(0), _job_healthiness_1));
    ASSERT_NO_FATAL_FAILURE(check_healthiness_equality(participant_health.jobs_healthiness.at(1), _job_healthiness_2));

    ASSERT_TRUE(_logging_called);
}

TEST_F(ParticipantHealthListenerTest, updateDifferentSystemName)
{
    _health_listener.updateEvent(
        fep3::IServiceBus::ServiceUpdateEvent{
            _participant_name ,
            "another_system",
            "url",
            fep3::IServiceBus::ServiceUpdateEventType::notify_alive });

    auto participant_health = _health_listener.getParticipantHealth();

    ASSERT_TRUE(participant_health.jobs_healthiness.empty());
    ASSERT_FALSE(_logging_called);
}

TEST_F(ParticipantHealthListenerTest, updateDifferentParticipantName)
{
    _health_listener.updateEvent(
        fep3::IServiceBus::ServiceUpdateEvent{
            "another_participant" ,
            _system_name,
            "url",
            fep3::IServiceBus::ServiceUpdateEventType::notify_alive });

    auto participant_health = _health_listener.getParticipantHealth();

    ASSERT_TRUE(participant_health.jobs_healthiness.empty());
    ASSERT_FALSE(_logging_called);
}

TEST_F(ParticipantHealthListenerTest, deactivateLogging)
{
    EXPECT_CALL(_rpc_health_service_mock, getHealth()).WillOnce(Return(_jobs_healthiness));

    _health_listener.deactivateLogging();
    _health_listener.updateEvent(
        fep3::IServiceBus::ServiceUpdateEvent{
            _participant_name ,
            _system_name,
            "url",
            fep3::IServiceBus::ServiceUpdateEventType::notify_alive });

    auto participant_health = _health_listener.getParticipantHealth();

    ASSERT_EQ(participant_health.jobs_healthiness.size(), 2);
    ASSERT_NO_FATAL_FAILURE(check_healthiness_equality(participant_health.jobs_healthiness.at(0), _job_healthiness_1));
    ASSERT_NO_FATAL_FAILURE(check_healthiness_equality(participant_health.jobs_healthiness.at(1), _job_healthiness_2));

    ASSERT_FALSE(_logging_called);
}
