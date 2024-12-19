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

#pragma once

#include <gtest/gtest.h>
#include <fep_system/fep_system.h>
#include <string>
#include <fep_system/logging_sink_csv.h>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <optional>

struct SystemLibraryFileLogging : public ::testing::Test {
    const std::string _log_path = TEST_LOGFILES_DIR;
    boost::filesystem::path _files_path;
    boost::filesystem::path _test_log_file;
    std::string _test_log_file_string;
    std::string _content;
    const std::chrono::milliseconds _timestamp = std::chrono::milliseconds(1681215840350);
    const std::string _logger_name = "logger_name";
    const fep3::LoggerSeverity _logger_severity{fep3::LoggerSeverity::debug};
    const std::string _log_message_string = "A log message";

    void SetUp()
    {
        ASSERT_NO_THROW(cleanUpBefore());
        setUpFilePaths();
    }

    void TearDown()
    {
        ASSERT_NO_THROW(boost::filesystem::remove_all(_log_path))
            << " Log file folder files still in use";
    }

    void cleanUpBefore()
    {
        if (boost::filesystem::exists(_log_path)) {
            boost::filesystem::remove_all(_log_path);
        }
        boost::filesystem::create_directory(_log_path);
    }

    void setUpFilePaths()
    {
        _files_path = boost::filesystem::canonical(_log_path);
        _test_log_file = _files_path;
        _test_log_file.append("some_logfile.txt");
        _test_log_file_string = _test_log_file.string();
    }

    testing::AssertionResult readFileContent()
    {
        if (boost::filesystem::exists(_test_log_file_string)) {
            std::ifstream ifs(_test_log_file_string);
            _content = std::string(std::istreambuf_iterator<char>{ifs}, {});
            return testing::AssertionSuccess();
        }
        else {
            return testing::AssertionFailure()
                << "Log file " << _test_log_file_string << " not found";
        }
    }

    void stringNotInFile(const std::string& unexpected_string)
    {
        ASSERT_EQ(_content.find(unexpected_string), std::string::npos)
            << "unexpected string " << unexpected_string << " found in the log file";
    }

    void findStringInFile(const std::string& expected_string)
    {
        ASSERT_NE(_content.find(expected_string), std::string::npos)
            << "expected string " << expected_string << " not found in the log file";
    }

    void checkIsoTime(const std::string& time_string)
    {
        boost::regex timestamp_iso8601_regex{
            "(\\d{4})-(\\d{2})-(\\d{2})T(\\d*):(\\d*):(\\d*),(\\d*)[\\+-](\\d{2})"};
        boost::smatch what;
        ASSERT_TRUE(boost::regex_search(time_string, what, timestamp_iso8601_regex))
            << "Json timestamp does not comply to ISO8601 time format";
    }

    void checkCsvContent(fep3::LoggerSeverity logger_severity, const std::string& logger_name )
    {
        std::ifstream log_file(_test_log_file_string);
        ASSERT_TRUE(log_file.is_open());
        std::string line;

        getline(log_file, line);
        std::string expected_header = fep3::csv_log_header;
        // erase "\n" as it is discarded by getline()
        ASSERT_EQ(line,
            expected_header.erase(expected_header.find("\n"), expected_header.length()));

        getline(log_file, line);
        const auto local_time_string{line.substr(0, line.find("\t"))};
        checkIsoTime(local_time_string);

        const auto log_without_iso_time{line.substr(line.find("\t"), line.size())};

        ASSERT_NE(log_without_iso_time.find(logger_name), std::string::npos);
        ASSERT_NE(log_without_iso_time.find(getString(logger_severity)), std::string::npos);

        log_file.close();
    }
};