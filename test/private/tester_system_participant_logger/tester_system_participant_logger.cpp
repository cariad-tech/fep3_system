/**
 * @file
 * @copyright
 * @verbatim
Copyright 2023 CARIAD SE. 

This Source Code Form is subject to the terms of the Mozilla
Public License, v. 2.0. If a copy of the MPL was not distributed
with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

@endverbatim
 */

#include <gtest/gtest.h>
#include <fep_system/system_logger.h>
#include "../test/include/fep_system/mock_event_monitor.h"

using EventMonitorMock = ::testing::NiceMock<fep3::mock::EventMonitor>;
using fep3::LoggerSeverity;
using testing::_;

struct LogWithEventMonitor : public ::testing::Test
{
    LogWithEventMonitor()
    {
    }

protected:
    EventMonitorMock _event_monitor_mock;
    const std::string _logger_name = "logger_name";
    const std::string _system_name = "system_name";
    const std::string _log_message_string = "A log message";
};

struct SystemLoggerWithEventMonitor : public LogWithEventMonitor
{
    SystemLoggerWithEventMonitor()
    {
    }

    void SetUp() override
    {
        _system_logger.registerMonitor(&_event_monitor_mock);
    }

    void logAllSeverities()
    {
        _system_logger.log(LoggerSeverity::debug, _log_message_string);
        _system_logger.log(LoggerSeverity::info, _log_message_string);
        _system_logger.log(LoggerSeverity::warning, _log_message_string);
        _system_logger.log(LoggerSeverity::error, _log_message_string);
        _system_logger.log(LoggerSeverity::fatal, _log_message_string);
    }

    fep3::SystemLogger _system_logger{_system_name};
};

TEST_F(SystemLoggerWithEventMonitor, log__defaultSeverity__successfull)
{
    EXPECT_CALL(_event_monitor_mock, onLog(_, _, _system_name, "system_logger", _log_message_string)).Times(4);
    EXPECT_CALL(_event_monitor_mock, onLog(_, LoggerSeverity::debug, _, _, _)).Times(0);

    logAllSeverities();
}

TEST_F(SystemLoggerWithEventMonitor, log__minSeverity__successfull)
{
    _system_logger.setSeverityLevel(LoggerSeverity::debug);

    EXPECT_CALL(_event_monitor_mock, onLog(_, _, _system_name, "system_logger", _log_message_string)).Times(5);

    logAllSeverities();
}

TEST_F(SystemLoggerWithEventMonitor, log__offSeverity__successfull)
{
    _system_logger.setSeverityLevel(LoggerSeverity::off);

    EXPECT_CALL(_event_monitor_mock, onLog(_, _, _, _, _)).Times(0);

    logAllSeverities();
}

TEST_F(SystemLoggerWithEventMonitor, log__maxSeverity__successfull)
{
    _system_logger.setSeverityLevel(LoggerSeverity::fatal);

    EXPECT_CALL(_event_monitor_mock, onLog(_, _, _, _, _)).Times(0);
    EXPECT_CALL(_event_monitor_mock, onLog(_, LoggerSeverity::fatal, _system_name, "system_logger", _log_message_string)).Times(1);

    logAllSeverities();
}

TEST_F(SystemLoggerWithEventMonitor, log__monitorReleased__successfull)
{
    _system_logger.releaseMonitor(&_event_monitor_mock);

    EXPECT_CALL(_event_monitor_mock, onLog(_, _, _, _, _)).Times(0);

    logAllSeverities();
}

TEST_F(SystemLoggerWithEventMonitor, log__multipleMonitors__successfull)
{
    EventMonitorMock event_monitor_mock;
    _system_logger.setSeverityLevel(LoggerSeverity::debug);

    _system_logger.registerMonitor(&event_monitor_mock);

    EXPECT_CALL(_event_monitor_mock, onLog(_, _, _system_name, "system_logger", _log_message_string)).Times(5);
    EXPECT_CALL(event_monitor_mock, onLog(_, _, _system_name, "system_logger", _log_message_string)).Times(5);

    logAllSeverities();
}

TEST_F(SystemLoggerWithEventMonitor, logProxyError__successfull)
{
    _system_logger.setSeverityLevel(LoggerSeverity::debug);

    EXPECT_CALL(_event_monitor_mock, onLog(_, _, _system_name, "system_logger", ::testing::HasSubstr("RPC Service Proxy"))).Times(5);

    const std::string participant_name = "participant_name", component = "component";
    _system_logger.logProxyError(LoggerSeverity::debug, participant_name, component, _log_message_string);
    _system_logger.logProxyError(LoggerSeverity::info, participant_name, component, _log_message_string);
    _system_logger.logProxyError(LoggerSeverity::warning, participant_name, component, _log_message_string);
    _system_logger.logProxyError(LoggerSeverity::error, participant_name, component, _log_message_string);
    _system_logger.logProxyError(LoggerSeverity::fatal, participant_name, component, _log_message_string);
}
