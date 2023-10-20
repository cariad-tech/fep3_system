/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2021 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */


#pragma once
#include <string>
#include "a_util/datetime.h"
#include "a_util/strings/strings_convert.h"
#include "a_util/strings/strings_functions.h"
#include "fep_system/system_logger_intf.h"

#include "rpc_services/logging/logging_rpc_intf.h"
#include <fep3/components/service_bus/rpc/fep_rpc.h>
#include "fep_system_stubs/logging_sink_stub.h"

#include <mutex>


namespace fep3
{
    using LogSink = rpc::RPCService<rpc_proxy_stub::RPCLoggingSink, rpc::IRPCLoggingSinkClientDef>;
    class LogSinkImpl : public LogSink
    {
    private:
        ISystemLogger& _system_logger;

    public:
        explicit LogSinkImpl(ISystemLogger& system_logger) : _system_logger(system_logger)
        {
        }
        int onLog(const std::string& description,
            const std::string& logger_name,
            const std::string& participant,
            int severity,
            const std::string& timestamp) override
        {
            _system_logger.log(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::nanoseconds(a_util::strings::toInt64(timestamp))),
                static_cast<LoggerSeverity>(severity),
                participant,
                logger_name,
                description);
            return 0;
        }
    };

    class SystemLogger :
        public ISystemLogger
    {
    public:
        SystemLogger()
            : _servicebus_connection(getServiceBusWrapper())
        {
        }

        ~SystemLogger()
        {
            _system_access->getServer()->unregisterService(
                rpc::IRPCLoggingSinkClientDef::getRPCDefaultName());
        }

        void registerMonitor(IEventMonitor* monitor)
        {
            std::lock_guard<std::recursive_mutex> _lock(_synch_event_monitor);
            _monitor = monitor;
        }

        void releaseMonitor()
        {
            std::lock_guard<std::recursive_mutex> _lock(_synch_event_monitor);
            _monitor = nullptr;
        }

        void log(const std::chrono::milliseconds& time_as_ms,
            LoggerSeverity level,
            const std::string& participant_name,
            const std::string& logger_name, //depends on the Category ...
            const std::string& message) const
        {
            std::lock_guard<std::recursive_mutex> _lock(_synch_event_monitor);
            if (_monitor)
            {
                _monitor->onLog(time_as_ms, level, participant_name, logger_name, message);
            }
        }
        void log(
            LoggerSeverity level,
            const std::string& participant_name,
            const std::string& logger_name, //depends on the Category ...
            const std::string& message) const
        {
            auto log_time_point = std::chrono::system_clock::now();
            auto time_as_ms = std::chrono::milliseconds(std::chrono::system_clock::to_time_t(log_time_point));
            log(time_as_ms, level, participant_name, logger_name, message);
        }

        void setSeverityLevel(LoggerSeverity level)
        {
            _level = level;
        }

        void initRPCService(const std::string& system_name)
        {
            //we create an server that is not discoverable
            _system_access = _servicebus_connection.createOrGetServiceBusConnection(system_name,"")->getSystemAccessCatelyn(system_name);

            if (!_system_access)
            {
                throw std::runtime_error("it is not possible to create a system access for logger on system " + system_name);

            }

            const auto result_creation = _system_access->createServer("system_" + system_name + "_" + a_util::strings::toString(getId()),
                arya::IServiceBus::ISystemAccess::_use_default_url, false);
            if (!result_creation)
            {
                throw std::runtime_error("it is not possible to create or get a server for logger on system " + system_name);
            }
            _log_sink_impl = std::make_shared<LogSinkImpl>(*this);
            _system_access->getServer()->registerService(
                rpc::IRPCLoggingSinkClientDef::getRPCDefaultName(),
                _log_sink_impl);
        }

        std::string getUrl() const
        {
            if (_system_access && _system_access->getServer())
            {
                auto url = _system_access->getServer()->getUrl();
                if (url.find("http://0.0.0.0:") != std::string::npos)
                {
                    //this is a replacement for beta
                    a_util::strings::replace(url, "0.0.0.0", a_util::system::getHostname());
                }
                return url;
            }
            return{};
        }

        static int getId()
        {
            static int counter = 0;
            static std::mutex counter_sync;
            std::lock_guard<std::mutex> lock(counter_sync);
            return counter++;
        }

        LoggerSeverity _level = LoggerSeverity::info;
        IEventMonitor* _monitor = nullptr;
        mutable std::recursive_mutex _synch_event_monitor;
        std::shared_ptr<fep3::IServiceBus::ISystemAccess> _system_access;
        ServiceBusWrapper _servicebus_connection;
        std::shared_ptr<LogSinkImpl> _log_sink_impl;

    };
}
