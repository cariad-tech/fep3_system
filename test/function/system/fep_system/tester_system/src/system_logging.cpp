/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * Test Case:   TestSystemLibrary
 * Test ID:     1.0
 * Test Title:  FEP System Library base test
 * Description: Test if controlling a test system is possible
 * Strategy:    Invoke Controller and issue commands
 * Passed If:   no errors occur
 * Ticket:      -
 * Requirement: -
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fep_system/fep_system.h>
#include <string>
#include <fep_test_common.h>
#include <fep_system/logging_sink_csv.h>
#include "../test/function/utils/common/fep_test_logging.h"
#include <fep_system/mock_event_monitor.h>
#include <gmock_async_helper.h>

using namespace std::literals::chrono_literals;
using namespace testing;

using EventMonitorMock = NiceMock<fep3::mock::EventMonitor>;

struct SystemLibraryLogging : public ::testing::Test
{
    SystemLibraryLogging()
        : _log_monitor1(std::make_shared<EventMonitorMock>()), _log_monitor2(std::make_shared<EventMonitorMock>())
    {
        _system_name = makePlatformDepName("test_system");
        _test_parts = createTestParticipants(_participant_names, _system_name);
    }

protected:
    const std::vector<std::string> _participant_names{"test_participant1", "test_participant2"};
    std::string _system_name;
    TestParticipants _test_parts;
    fep3::System _system;
    std::shared_ptr<EventMonitorMock> _log_monitor1, _log_monitor2;
    test::helper::Notification _notification1, _notification2;
};

/**
 * Test whether multiple event monitors can be registered to listen to participant logs.
 */
TEST_F(SystemLibraryLogging, TestParticipantLoggerMultipleMonitors)
{
    _system = fep3::discoverSystem(_system_name, _participant_names, 60000ms);

    _system.registerMonitoring(*_log_monitor1);
    _system.registerMonitoring(*_log_monitor2);

    ASSERT_TRUE(_system.getParticipant("test_participant2").loggingRegistered());
    ASSERT_TRUE(_system.getParticipant("test_participant1").loggingRegistered());

    test::helper::Notification notification3, notification4;

    EXPECT_CALL(*_log_monitor1, onLog(_, fep3::LoggerSeverity::info, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                { _notification1.notify(); }));
    EXPECT_CALL(*_log_monitor1, onLog(_, fep3::LoggerSeverity::warning, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                   { _notification2.notify(); }));
    EXPECT_CALL(*_log_monitor1, onLog(_, fep3::LoggerSeverity::error, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                 { notification3.notify(); }));
    EXPECT_CALL(*_log_monitor1, onLog(_, fep3::LoggerSeverity::fatal, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                 { notification4.notify(); }));

    test::helper::Notification notification5, notification6, notification7, notification8;

    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::info, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                { notification5.notify(); }));
    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::warning, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                   { notification6.notify(); }));
    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::error, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                 { notification7.notify(); }));
    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::fatal, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                 { notification8.notify(); }));

    EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::loaded));
    EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::initialized));

    ASSERT_TRUE(_notification1.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(_notification2.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(notification3.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(notification4.waitForNotificationWithTimeout(5s));

    ASSERT_TRUE(notification5.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(notification6.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(notification7.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(notification8.waitForNotificationWithTimeout(5s));

    _system.unregisterMonitoring(*_log_monitor1);
    _system.unregisterMonitoring(*_log_monitor2);
}

/**
 * Test whether multiple event monitors can be registered to listen to system logs.
 */
TEST_F(SystemLibraryLogging, TestSystemLoggerMultipleMonitors)
{
    // No participants discovered yet. setSystemState fails and logs accordingly.
    {
        _system.registerSystemMonitoring(*_log_monitor1);
        _system.registerSystemMonitoring(*_log_monitor2);

        _system.setSystemSeverityLevel(fep3::LoggerSeverity::error);

        EXPECT_CALL(*_log_monitor1, onLog(_, fep3::LoggerSeverity::error, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                     { _notification1.notify(); }));
        EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::error, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                     { _notification2.notify(); }));

        EXPECT_THROW(_system.setSystemState(fep3::System::AggregatedState::loaded), std::runtime_error);

        ASSERT_TRUE(_notification1.waitForNotificationWithTimeout(5s));
        ASSERT_TRUE(_notification2.waitForNotificationWithTimeout(5s));
    }

    _system = fep3::discoverSystem(_system_name, _participant_names, 60000ms);

    // Participants discovered. setSystemState succeeds and logs accordingly.
    {
        ASSERT_TRUE(_system.getParticipant("test_participant2").loggingRegistered());
        ASSERT_TRUE(_system.getParticipant("test_participant1").loggingRegistered());

        _system.registerSystemMonitoring(*_log_monitor1);
        _system.registerSystemMonitoring(*_log_monitor2);

        _system.setSystemSeverityLevel(fep3::LoggerSeverity::info);

        EXPECT_CALL(*_log_monitor1, onLog(_, fep3::LoggerSeverity::info, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                    { _notification1.notify(); }));
        EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::info, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                    { _notification2.notify(); }));

        EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::loaded));
        EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::unloaded));

        ASSERT_TRUE(_notification1.waitForNotificationWithTimeout(5s));
        ASSERT_TRUE(_notification2.waitForNotificationWithTimeout(5s));
    }

    _system.unregisterMonitoring(*_log_monitor1);
    _system.unregisterMonitoring(*_log_monitor2);
}

/**
 * Test whether multiple system instances using the same system name can register participant event monitoring
 * simultaneously.
 */
TEST_F(SystemLibraryLogging, TestParticipantLoggerMultipleSystems)
{
    _system = fep3::discoverSystem(_system_name, _participant_names, 60000ms);
    auto redundantSystem = fep3::discoverSystem(_system_name, _participant_names, 60000ms);

    _system.setSystemSeverityLevel(fep3::LoggerSeverity::info);
    redundantSystem.setSystemSeverityLevel(fep3::LoggerSeverity::info);

    _system.registerMonitoring(*_log_monitor1);
    redundantSystem.registerMonitoring(*_log_monitor2);

    test::helper::Notification notification3, notification4;

    {
        EXPECT_CALL(*_log_monitor1, onLog(_, _, ::testing::StrEq("test_participant1"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { _notification1.notify(); }));
        EXPECT_CALL(*_log_monitor1, onLog(_, _, ::testing::StrEq("test_participant2"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { _notification2.notify(); }));
        EXPECT_CALL(*_log_monitor2, onLog(_, _, ::testing::StrEq("test_participant1"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { notification3.notify(); }));
        EXPECT_CALL(*_log_monitor2, onLog(_, _, ::testing::StrEq("test_participant2"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { notification4.notify(); }));
        EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::loaded));
        ASSERT_TRUE(_notification1.waitForNotificationWithTimeout(5s));
        ASSERT_TRUE(_notification2.waitForNotificationWithTimeout(5s));
        ASSERT_TRUE(notification3.waitForNotificationWithTimeout(5s));
        ASSERT_TRUE(notification4.waitForNotificationWithTimeout(5s));
    }

    _system.unregisterMonitoring(*_log_monitor1);
    redundantSystem.unregisterMonitoring(*_log_monitor2);
}

/**
 * Test whether a system correctly forwards Participant logs to a registered event monitor
 * if a younger system instance using the same system name is destructed before logging happens.
 */
TEST_F(SystemLibraryLogging, TestParticipantLoggerRedundantSystemDestructed)
{
    {
        fep3::System redundant_system = fep3::discoverSystem(_system_name, _participant_names, 60000ms);
        _system = fep3::discoverSystem(_system_name, _participant_names, 60000ms);
    }

    _system.setSystemSeverityLevel(fep3::LoggerSeverity::info);

    _system.registerMonitoring(*_log_monitor1);

    {
        EXPECT_CALL(*_log_monitor1, onLog(_, _, ::testing::StrEq("test_participant1"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { _notification1.notify(); }));
        EXPECT_CALL(*_log_monitor1, onLog(_, _, ::testing::StrEq("test_participant2"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { _notification2.notify(); }));
        EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::loaded));
        ASSERT_TRUE(_notification1.waitForNotificationWithTimeout(5s));
        ASSERT_TRUE(_notification2.waitForNotificationWithTimeout(5s));
    }

    _system.unregisterMonitoring(*_log_monitor1);
}

/**
 * Test whether a copied system correctly forwards Participant logs to a registered event monitor
 * if its origin system instance is destructed before logging happens.
 */
TEST_F(SystemLibraryLogging, TestParticipantLoggerOriginalSystemDestructed)
{
    {
        fep3::System original_system = fep3::discoverSystem(_system_name, _participant_names, 60000ms);
        _system = original_system;
    }

    _system.setSystemSeverityLevel(fep3::LoggerSeverity::info);

    _system.registerMonitoring(*_log_monitor1);

    {
        EXPECT_CALL(*_log_monitor1, onLog(_, _, ::testing::StrEq("test_participant1"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { _notification1.notify(); }));
        EXPECT_CALL(*_log_monitor1, onLog(_, _, ::testing::StrEq("test_participant2"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { _notification2.notify(); }));
        EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::loaded));
        ASSERT_TRUE(_notification1.waitForNotificationWithTimeout(5s));
        ASSERT_TRUE(_notification2.waitForNotificationWithTimeout(5s));
    }

    _system.unregisterMonitoring(*_log_monitor1);
}

/**
 * Test whether a moved system correctly forwards Participant logs to a registered event monitor.
 */
TEST_F(SystemLibraryLogging, TestParticipantLoggerMovedSystem)
{
    {
        auto original_system = fep3::discoverSystem(_system_name, _participant_names, 60000ms);
        _system = std::move(original_system);
    }

    _system.setSystemSeverityLevel(fep3::LoggerSeverity::info);

    _system.registerMonitoring(*_log_monitor1);

    {
        EXPECT_CALL(*_log_monitor1, onLog(_, _, ::testing::StrEq("test_participant1"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { _notification1.notify(); }));
        EXPECT_CALL(*_log_monitor1, onLog(_, _, ::testing::StrEq("test_participant2"), _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                               { _notification2.notify(); }));
        EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::loaded));
        ASSERT_TRUE(_notification1.waitForNotificationWithTimeout(5s));
        ASSERT_TRUE(_notification2.waitForNotificationWithTimeout(5s));
    }

    _system.unregisterMonitoring(*_log_monitor1);
}

/**
 * Test whether two event monitors can be registered to listen to participant and system logs in parallel.
 */
TEST_F(SystemLibraryLogging, TestParticipantAndSystemLogger)
{
    _system = fep3::discoverSystem(_system_name, _participant_names, 60000ms);

    _system.registerSystemMonitoring(*_log_monitor1);
    _system.setSystemSeverityLevel(fep3::LoggerSeverity::debug);

    _system.registerMonitoring(*_log_monitor2);
    _system.setSeverityLevel(fep3::LoggerSeverity::debug);

    test::helper::Notification notification3, notification4, notification5;

    EXPECT_CALL(*_log_monitor1, onLog(_, fep3::LoggerSeverity::debug, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::debug, _, _, _)).Times(AnyNumber());

    EXPECT_CALL(*_log_monitor1, onLog(_, fep3::LoggerSeverity::info, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                { _notification1.notify(); }));

    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::info, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                { _notification2.notify(); }));
    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::warning, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                   { notification3.notify(); }));
    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::error, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                 { notification4.notify(); }));
    EXPECT_CALL(*_log_monitor2, onLog(_, fep3::LoggerSeverity::fatal, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                 { notification5.notify(); }));

    EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::initialized));
    EXPECT_NO_THROW(_system.setSystemState(fep3::System::AggregatedState::unloaded));

    _system.unregisterMonitoring(*_log_monitor2);
    _system.unregisterSystemMonitoring(*_log_monitor1);

    ASSERT_TRUE(_notification1.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(_notification2.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(notification3.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(notification4.waitForNotificationWithTimeout(5s));
    ASSERT_TRUE(notification5.waitForNotificationWithTimeout(5s));
}

TEST_F(SystemLibraryFileLogging, TestLoggingSinkCsv)
{
    fep3::LoggingSinkCsv logging_sink_csv(_test_log_file_string);
    auto _system_name = makePlatformDepName("test_system");
    fep3::System system(_system_name);
    system.registerSystemMonitoring(logging_sink_csv);
    system.setSystemSeverityLevel(fep3::LoggerSeverity::debug);

    EXPECT_THROW(system.setSystemState(fep3::System::AggregatedState::loaded), std::runtime_error);

    checkCsvContent(fep3::LoggerSeverity::warning, "system_logger");

    system.unregisterSystemMonitoring(logging_sink_csv);
}

/* Tests the case that the Event Monitoring is registered before the participant
    is completely ready. See FEPSDK-2982 and SUP-4001
*/
TEST(SystemLibrary, TestEarlyEventMonitorRegister)
{
    auto _system_name = makePlatformDepName("test_system");
    const std::string not_ready_participant{"test_participant1"};

    fep3::System _system(_system_name);
    _system.add(not_ready_participant);

    ASSERT_FALSE(_system.getParticipant(not_ready_participant).loggingRegistered());

    auto event_monitor_mock = std::make_shared<EventMonitorMock>();
    test::helper::Notification notification;
    EXPECT_CALL(*event_monitor_mock, onLog(_, fep3::LoggerSeverity::warning, _, _, _)).WillRepeatedly(InvokeWithoutArgs([&]()
                                                                                                                                                                          { notification.notify(); }));

    _system.registerSystemMonitoring(*event_monitor_mock);
    _system.registerMonitoring(*event_monitor_mock);

    ASSERT_TRUE(notification.waitForNotificationWithTimeout(5s));

    _system.unregisterMonitoring(*event_monitor_mock);
    _system.unregisterSystemMonitoring(*event_monitor_mock);
}
