/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <chrono>
#include <fep_system/event_monitor_intf.h>
#include <a_util/strings.h>
#include <fstream>
#include <boost/date_time.hpp>
#include <fep3/fep3_filesystem.h>

namespace fep3
{
    constexpr auto csv_log_header =
        "# timestamp\tsimulation_time[ns]\tlogger_name\tseverity_level\tmessage\n";
    constexpr size_t time_zone_string_size = 3;

    class ILogFileWrapper
    {
    public:
        virtual ~ILogFileWrapper() = default;
        virtual void open(const std::string &, std::ios_base::openmode) = 0;
        virtual bool is_open() const = 0;
        virtual void close() = 0;
        virtual bool fail() const = 0;
        virtual void flush() = 0;
        virtual void write(const std::string &) = 0;
    };

    class LogFileWrapper : public ILogFileWrapper
    {
    public:
        void open(const std::string &file_path, std::ios_base::openmode file_mode = std::ios_base::in | std::ios_base::out) override
        {
            _log_file->open(file_path, file_mode);
        }
        bool is_open() const override
        {
            return _log_file->is_open();
        }
        void close() override
        {
            _log_file->close();
        }
        bool fail() const override
        {
            return _log_file->fail();
        }
        void flush() override
        {
            _log_file->flush();
        }
        void write(const std::string &message) override
        {
            _log_file->write(message.c_str(), message.size());
        }

    private:
        std::unique_ptr<std::fstream> _log_file = std::make_unique<std::fstream>();
    };

    class LoggingSinkCsv : public fep3::IEventMonitor
    {
    public:
        LoggingSinkCsv() = delete;
        LoggingSinkCsv(const LoggingSinkCsv &) = delete;
        LoggingSinkCsv(LoggingSinkCsv &&) = delete;
        LoggingSinkCsv &operator=(const LoggingSinkCsv &) = delete;
        LoggingSinkCsv &operator=(LoggingSinkCsv &&) = delete;

        LoggingSinkCsv(const std::string &log_file_path, std::unique_ptr<ILogFileWrapper> log_file = std::make_unique<LogFileWrapper>())
            : _time_zone(getTimeZone()),
              _log_file_path(fs::path(log_file_path)),
              _log_file(std::move(log_file))
        {
            if (_log_file_path.empty())
            {
                throw std::runtime_error("File path for file logger is empty.");
            }

            if (_log_file->is_open())
            {
                _log_file->close();
            }

            const auto file_exists = fs::exists(_log_file_path);

            _log_file->open(_log_file_path.string().c_str(), std::fstream::out | std::fstream::app);

            if (_log_file->fail())
            {
                throw std::runtime_error(std::string("Unable to open log file ") + log_file_path);
            }

            if (file_exists)
            {
                logMessage("\n");
            }

            logMessage(csv_log_header);
        }

        void onLog(std::chrono::milliseconds log_time,
                   LoggerSeverity severity_level,
                   const std::string &participant_name,
                   const std::string &logger_name,
                   const std::string &message)
        {
            auto log_time_string = std::to_string(log_time.count());
            auto formatted_log_message = formatLogMessage({log_time_string, severity_level, participant_name, logger_name, message});
            formatted_log_message.append("\n");

            logMessage(formatted_log_message);
        }

    private:
        std::string getTimeZone()
        {
            std::stringstream ss;
#ifdef __linux__
            std::time_t t = std::time(nullptr);
            std::tm tm = *std::localtime(&t);
            ss << std::put_time(&tm, "%z") << '\n';
#elif _WIN32
            struct tm newtime;
            time_t now = time(0);
            localtime_s(&newtime, &now);
            ss << std::put_time(&newtime, "%z") << '\n';
#endif
            std::string timeZoneString = ss.str();
            timeZoneString.resize(time_zone_string_size);
            return timeZoneString;
        }

        void checkFractionalSeconds(std::string &timeString) const
        {
            // A decimal mark, either a comma or a dot, is used as a separator between
            // the time element and its fraction. (Following ISO 80000-1 according to
            // ISO 8601:1-2019 it does not stipulate a preference except within
            // International Standards, but with a preference for a comma according to
            // ISO 8601:2004)
            // https://en.wikipedia.org/wiki/ISO_8601
            auto dotPos = timeString.find('.');
            if ((dotPos) == std::string::npos)
            {
                timeString.append(",0");
            }
            else
            {
                timeString.replace(dotPos, 1, ",");
            }
        }

        std::string getLocalTime() const
        {
            boost::posix_time::ptime localTime(boost::posix_time::microsec_clock::local_time());
            std::string isoTimeFormat = boost::posix_time::to_iso_extended_string(localTime);
            checkFractionalSeconds(isoTimeFormat);
            return isoTimeFormat;
        }

        std::string formatLogMessage(const LogMessage &log) const
        {
            std::string log_msg;

            log_msg.append(
                a_util::strings::format("%s\t", getLocalTime().append(_time_zone).c_str()));
            log_msg.append(a_util::strings::format("%s\t", log._timestamp.c_str()));
            log_msg.append(a_util::strings::format("%s\t", log._logger_name.c_str()));
            log_msg.append(a_util::strings::format("%s\t", getString(log._severity).c_str()));
            log_msg.append(log._message.c_str());

            return log_msg;
        }

        void logMessage(const std::string &log_message) const
        {
            _log_file->write(log_message);

            if (_log_file->fail())
            {
                throw std::runtime_error("Failed to write log into file");
            }
            else
            {
                // explicit flush to reduce loss of logs in case of crash
                _log_file->flush();
            }

            return;
        }

    private:
        const std::string _time_zone;
        const fs::path _log_file_path;
        std::unique_ptr<ILogFileWrapper> _log_file;
    };
}
