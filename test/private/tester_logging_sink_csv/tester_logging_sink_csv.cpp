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

#include <fep_system/logging_sink_csv.h>
#include "../../test/function/utils/common/fep_test_logging.h"
#include "../../test/function/utils/common/mock_log_file_wrapper.h"
#include <gtest/gtest.h>

using LogFileWrapperMock = ::testing::NiceMock<fep3::mock::LogFileWrapper>;
using ::testing::Return;
using ::testing::_;
using ::testing::HasSubstr;

struct LoggingSinkCsv : public ::testing::Test
{
    LoggingSinkCsv()
        : _log_file_mock(std::make_unique<LogFileWrapperMock>())
        , _log_file_mock_ptr(_log_file_mock.get())
        , _test_log_file_path(fs::canonical("").append("some_logfile.txt"))
    {
    }

    void checkLogging(fep3::LoggingSinkCsv& logging_sink_csv)
    {
        auto expected_log_message = a_util::strings::format("\t%lld\t%s\t%s\t%s",
            _timestamp.count(),
            _logger_name.c_str(),
            getString(_logger_severity).c_str(),
            _log_message_string.c_str());
        EXPECT_CALL(*_log_file_mock_ptr, write(HasSubstr(expected_log_message))).Times(1);
        EXPECT_CALL(*_log_file_mock_ptr, fail()).WillOnce(Return(false));
        EXPECT_CALL(*_log_file_mock_ptr, flush()).Times(1);

        logging_sink_csv.onLog(_timestamp, _logger_severity, "", _logger_name, _log_message_string);
    }

protected:
    std::unique_ptr<LogFileWrapperMock> _log_file_mock;
    LogFileWrapperMock* _log_file_mock_ptr;
    fs::path _test_log_file_path;
    const std::chrono::milliseconds _timestamp = std::chrono::milliseconds(1681215840350);
    const std::string _logger_name = "logger_name";
    const fep3::LoggerSeverity _logger_severity{ fep3::LoggerSeverity::debug };
    const std::string _log_message_string = "A log message";
};

struct DefaultLoggingSinkCsv : public LoggingSinkCsv
{
    DefaultLoggingSinkCsv() = default;

    void SetUp() override
    {
        EXPECT_CALL(*_log_file_mock_ptr, is_open()).WillOnce(Return(false));
        EXPECT_CALL(*_log_file_mock_ptr, open(_test_log_file_path.string(), std::fstream::out | std::fstream::app)).Times(1);
        EXPECT_CALL(*_log_file_mock_ptr, fail()).Times(2).WillRepeatedly(Return(false));
        EXPECT_CALL(*_log_file_mock_ptr, write(fep3::csv_log_header)).Times(1);
        EXPECT_CALL(*_log_file_mock_ptr, flush()).Times(1);

        _logging_sink_csv = std::make_unique<fep3::LoggingSinkCsv>(_test_log_file_path.string(), std::move(_log_file_mock));
    }

protected:
    std::unique_ptr<fep3::LoggingSinkCsv> _logging_sink_csv;

};

/**
* Test logging a csv message in a new file
*/
TEST_F(DefaultLoggingSinkCsv, TestLogNewFile)
{
    checkLogging(*_logging_sink_csv);
}

/**
* Test logging a csv message in an existing file
*/
TEST_F(LoggingSinkCsv, TestLogExistingFile)
{
    // create empty log file
    {
        if (!fs::exists(_test_log_file_path.string()))
        {
            std::ofstream output(_test_log_file_path.string());
        }
    }

    EXPECT_CALL(*_log_file_mock_ptr, is_open()).WillOnce(Return(false));
    EXPECT_CALL(*_log_file_mock_ptr, open(_test_log_file_path.string(), std::fstream::out | std::fstream::app)).Times(1);
    EXPECT_CALL(*_log_file_mock_ptr, fail()).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(*_log_file_mock_ptr, write("\n")).Times(1);
    EXPECT_CALL(*_log_file_mock_ptr, flush()).Times(2);
    EXPECT_CALL(*_log_file_mock_ptr, write(fep3::csv_log_header)).Times(1);

    fep3::LoggingSinkCsv logging_sink_csv(_test_log_file_path.string(), std::move(_log_file_mock));
    
    checkLogging(logging_sink_csv);

    //remove temporary log file
    fs::remove(_test_log_file_path);
}

/**
* Test logging a csv message in an open file
*/
TEST_F(LoggingSinkCsv, TestLogOpenFile)
{
    EXPECT_CALL(*_log_file_mock_ptr, is_open()).WillOnce(Return(true));
    EXPECT_CALL(*_log_file_mock_ptr, close()).Times(1);
    EXPECT_CALL(*_log_file_mock_ptr, open(_test_log_file_path.string(), std::fstream::out | std::fstream::app)).Times(1);
    EXPECT_CALL(*_log_file_mock_ptr, fail()).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*_log_file_mock_ptr, write(fep3::csv_log_header)).Times(1);
    EXPECT_CALL(*_log_file_mock_ptr, flush()).Times(1);

    fep3::LoggingSinkCsv logging_sink_csv(_test_log_file_path.string(), std::move(_log_file_mock));
    
    checkLogging(logging_sink_csv);
}

/**
* Test logging sink csv using an empty path
*/
TEST_F(LoggingSinkCsv, TestEmptyPath)
{
    EXPECT_THROW(fep3::LoggingSinkCsv logging_sink_csv(""), std::runtime_error);
}

/**
* Test logging sink csv if opening the csv file fails
*/
TEST_F(LoggingSinkCsv, TestFileOpenFails)
{
    EXPECT_CALL(*_log_file_mock_ptr, is_open()).WillOnce(Return(false));
    EXPECT_CALL(*_log_file_mock_ptr, open(_test_log_file_path.string(), std::fstream::out | std::fstream::app)).Times(1);
    EXPECT_CALL(*_log_file_mock_ptr, fail()).WillOnce(Return(true));

    EXPECT_THROW(fep3::LoggingSinkCsv logging_sink_csv(_test_log_file_path.string(), std::move(_log_file_mock)), std::runtime_error);
}

/**
* Test logging a csv message throws in case of failure
*/
TEST_F(DefaultLoggingSinkCsv, TestLogFails)
{
    auto expected_log_message = a_util::strings::format("\t%lld\t%s\t%s\t%s",
        _timestamp.count(),
        _logger_name.c_str(),
        getString(_logger_severity).c_str(),
        _log_message_string.c_str());
    EXPECT_CALL(*_log_file_mock_ptr, write(HasSubstr(expected_log_message))).Times(1);
    EXPECT_CALL(*_log_file_mock_ptr, fail()).WillOnce(Return(true));
    EXPECT_CALL(*_log_file_mock_ptr, flush()).Times(0);

    EXPECT_THROW(_logging_sink_csv->onLog(_timestamp, _logger_severity, "", _logger_name, _log_message_string), std::runtime_error);
}

/**
* Test logging a csv message in a new file
*/
TEST_F(SystemLibraryFileLogging, TestCsvWriteNewFile)
{
    fep3::LoggingSinkCsv logging_sink_csv(_test_log_file_string);
    logging_sink_csv.onLog(_timestamp, _logger_severity, "", _logger_name, _log_message_string);

    checkCsvContent(fep3::LoggerSeverity::debug, _logger_name);
}
