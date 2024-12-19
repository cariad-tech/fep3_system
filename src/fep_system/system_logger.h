/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once
#include <string>
#include <a_util/system.h>
#include <a_util/strings.h>
#include "fep_system/system_logger_intf.h"
#include "fep_system/event_monitor_intf.h"
#include "fep3/base/properties/properties.h"
#include "rpc_services/logging/logging_rpc_intf.h"
#include <fep3/components/service_bus/rpc/fep_rpc.h>
#include "fep_system_stubs/logging_sink_stub.h"
#include "service_bus_wrapper.h"
#include "system_logger_helper.h"
#include <algorithm>
#include <mutex>

namespace fep3
{
    class LogProvider {
    public:
        void registerMonitor(IEventMonitor* monitor)
        {
            std::lock_guard<std::mutex> _lock(_sync_event_monitor);
            _event_monitors.push_back(monitor);
        }

        void releaseMonitor(IEventMonitor* monitor)
        {
            std::lock_guard<std::mutex> _lock(_sync_event_monitor);
            _event_monitors.erase(std::remove(_event_monitors.begin(), _event_monitors.end(), monitor),
                                  _event_monitors.end());
        }

        void setSeverityLevel(LoggerSeverity level)
        {
            std::lock_guard<std::mutex> _lock(_sync_event_monitor);
            _level = level;
        }

    protected:
        void forwardLogToMonitors(const std::chrono::milliseconds& timestamp,
                 LoggerSeverity level,
                 const std::string& participant_name,
                 const std::string& logger_name,
                 const std::string& message) const
        {
            std::lock_guard<std::mutex> _lock(_sync_event_monitor);
            if (_level == fep3::LoggerSeverity::off || level > _level || _event_monitors.empty()) {
                return;
            }

            for (const auto& monitor: _event_monitors) {
                monitor->onLog(timestamp, level, participant_name, logger_name, message);
            }
        }

    private:
        std::vector<IEventMonitor*> _event_monitors;
        mutable std::mutex _sync_event_monitor;
        LoggerSeverity _level = LoggerSeverity::info;
    };

    class SystemLogger : public ISystemLogger, public LogProvider
    {
    public:

        SystemLogger(std::string system_name) : _system_name(std::move(system_name))
        {
        }

        void log(
            LoggerSeverity level,
            const std::string &message) const override
        {
            const auto log_time_point = std::chrono::system_clock::now();
            const auto time_as_ms = std::chrono::milliseconds(std::chrono::system_clock::to_time_t(log_time_point));
            LogProvider::forwardLogToMonitors(
                time_as_ms, level, _system_name, _logger_name, message);
        }

       void logProxyError(LoggerSeverity level,
                      const std::string& participant_name,
                      const std::string& component,
                      const std::string& message) const override
        {
           const std::string log_message = "RPC Service Proxy of component: " + component +
                                           " from participant " + participant_name +
                                           " logged: " + message;
            log(level, log_message);
        }

    private:
        std::string _system_name;
        const std::string _logger_name = "system_logger";
    };

    using RPCLoggingClient = rpc::RPCService<rpc_proxy_stub::RPCLoggingSink, rpc::IRPCLoggingSinkClientDef>;
    /// @brief The logs that the service received will be forwarded to the
    /// IRemoteLogForwarder (that is owned by a System class instance)
    /// then the IRemoteLogForwarder will forward the logs to the registered IEventMonitor(s)
    class RPCLoggingSinkService : public RPCLoggingClient
    {
    private:
        std::vector<IRemoteLogForwarder *> _participant_log_sinks;
        mutable std::mutex _sync_event_monitor;

    public:
        void registerParticipantLoggingSink(IRemoteLogForwarder *participant_log_sink)
        {
            if (!participant_log_sink)
            {
                return;
            }
            std::lock_guard<std::mutex> _lock(_sync_event_monitor);
            _participant_log_sinks.push_back(participant_log_sink);
        }

        void releaseParticipantLoggingSink(IRemoteLogForwarder *participant_log_sink)
        {
            if (!participant_log_sink)
            {
                return;
            }
            std::lock_guard<std::mutex> _lock(_sync_event_monitor);
            _participant_log_sinks.erase(std::remove(_participant_log_sinks.begin(), _participant_log_sinks.end(), participant_log_sink), _participant_log_sinks.end());
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> _lock(_sync_event_monitor);
            return _participant_log_sinks.empty();
        }

        int onLog(const std::string &description,
                  const std::string &logger_name,
                  const std::string &participant,
                  int severity,
                  const std::string &timestamp) override
        {
            std::lock_guard<std::mutex> _lock(_sync_event_monitor);
            for (auto &participant_log_sink : _participant_log_sinks)
            {
                participant_log_sink->log(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::nanoseconds(a_util::strings::toInt64(timestamp))),
                    static_cast<LoggerSeverity>(severity),
                    participant,
                    logger_name,
                    description);
            }

            return 0;
        }
    };

    /**
     * Class to handle service bus access within the logging context globally for all FEP System instances.
     * RPC Logging Sink services are created per FEP System name and remain available as long as at least one
     * FEP System instance remains.
     * A static map is used to keep track of the FEP System instances and create/clean up needed resources as
     * needed.
     * This is needed as FEP System instances sharing the same name would interfere with each other in respect
     * of creating/cleaning up RPC resources.
     */
    class GlobalLoggingSinkClientService
    {
        /// @brief This class holds the RPC server and the Logging Sink Service instance
        /// Should be created only once for each FEP System (independent of the count of fep3::System Library instances).
        struct LoggingSinkClientServiceWrapper
        {
        private:
            std::shared_ptr<fep3::IServiceBus::IParticipantServer> _system_server;
            std::shared_ptr<RPCLoggingSinkService> _rpc_logging_sink_service;
            std::shared_ptr<ISystemLogger> _system_logger;
            const std::string _system_name;

        public:
            LoggingSinkClientServiceWrapper(const std::string &system_name,
                                            std::shared_ptr<ISystemLogger> system_logger)
                : _system_logger(system_logger), _system_name(system_name)
            {
                auto system_access = getServiceBusWrapper().createOrGetServiceBusConnection(_system_name, "")->getSystemAccessCatelyn(_system_name);
                if (!system_access)
                {
                    FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                                              a_util::strings::format("It is not possible to create a system access for logger on system %s", _system_name.c_str()));
                }
                const auto result_creation = system_access->createServer("system_" + _system_name + "_" + std::to_string(getId()),
                                                                         arya::IServiceBus::ISystemAccess::_use_default_url, false);
                if (!result_creation)
                {
                    FEP3_SYSTEM_LOG_AND_THROW(_system_logger, LoggerSeverity::fatal,
                                              a_util::strings::format("It is not possible to create or get a server for logger on system %s", _system_name.c_str()));
                }
                _system_server = system_access->getServer();
                _rpc_logging_sink_service = std::make_shared<RPCLoggingSinkService>();
                const auto result_registration = _system_server->registerService(
                    rpc::IRPCLoggingSinkClientDef::getRPCDefaultName(),
                    _rpc_logging_sink_service);

                if (!result_registration)
                {
                    FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::warning,
                                    a_util::strings::format("It is not possible to register a server for logger on system %s. "
                                                            "Error: %s  - %s : occured in: %s - %s line: %s",
                                                            _system_name.c_str(), std::to_string(result_registration.getErrorCode()).c_str(),
                                                            result_registration.getDescription(), result_registration.getFunction(), result_registration.getFile(), std::to_string(result_registration.getLine()).c_str()));
                }
                else
                {
                    FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::debug,
                                    a_util::strings::format("Successfully registered a server for logger on system %s.", _system_name.c_str()));
                }
            }

            void registerParticipantsLoggingSink(IRemoteLogForwarder *participant_log_sink)
            {
                _rpc_logging_sink_service->registerParticipantLoggingSink(participant_log_sink);
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::debug,
                                "Registered a participant logging sink.");
            }

            void releaseParticipantsLoggingSink(IRemoteLogForwarder *participant_log_sink)
            {
                _rpc_logging_sink_service->releaseParticipantLoggingSink(participant_log_sink);
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::debug,
                                "Released a participant logging sink.");
            }

            bool empty()
            {
                return _rpc_logging_sink_service->empty();
            }

            std::string getUrl() const
            {
                if (_system_server)
                {
                    auto url = _system_server->getUrl();
                    if (url.find("http://0.0.0.0:") != std::string::npos)
                    {
                        // this is a replacement for beta
                        const std::string to_be_replaced = "0.0.0.0";
                        url.replace(url.find(to_be_replaced), to_be_replaced.length(), a_util::system::getHostname());
                    }
                    return url;
                }
                return {};
            }

            ~LoggingSinkClientServiceWrapper()
            {
                _system_server->unregisterService(
                    rpc::IRPCLoggingSinkClientDef::getRPCDefaultName());
                FEP3_SYSTEM_LOG(_system_logger, LoggerSeverity::debug,
                                a_util::strings::format("Unregistered a server for logger on system %s.", _system_name.c_str()));
            }

        private:
            // not needed? Guaranteed one server per instance, also old server is anyway deletd
            static int getId()
            {
                static int counter = 0;
                static std::mutex counter_sync;
                std::lock_guard<std::mutex> lock(counter_sync);
                return counter++;
            }
        };

    public:
        GlobalLoggingSinkClientService(const std::string& system_name,
                                       IRemoteLogForwarder* participant_logger,
                                       std::shared_ptr<ISystemLogger> system_logger)
            : _participant_logger(participant_logger), _system_name(system_name)
        {
            if (_servers.count(system_name) == 0)
            {
                auto logging_service_wrapper = std::make_unique<LoggingSinkClientServiceWrapper>(system_name, system_logger);
                logging_service_wrapper->registerParticipantsLoggingSink(participant_logger);
                _servers[system_name] = std::move(logging_service_wrapper);
            }
            else
            {
                _servers[system_name]->registerParticipantsLoggingSink(participant_logger);
            }

            _url = _servers[system_name]->getUrl();
        }

        std::string url()
        {
            return _url;
        }

        ~GlobalLoggingSinkClientService()
        {
            if (_servers.count(_system_name) > 0)
            {
                _servers[_system_name]->releaseParticipantsLoggingSink(_participant_logger);
                if (_servers[_system_name]->empty())
                {
                    _servers.erase(_system_name);
                }
            }
        }

    private:
        std::string _url;
        IRemoteLogForwarder *_participant_logger;
        const std::string _system_name;
        static std::map<std::string, std::unique_ptr<LoggingSinkClientServiceWrapper>> _servers;
    };

    class RemoteLogForwarder : public IRemoteLogForwarder, public LogProvider
    {
    public:
        void initRPCService(const std::string &system_name, std::shared_ptr<ISystemLogger> system_logger)
        {
            _log_service = std::make_unique<GlobalLoggingSinkClientService>(system_name, this, system_logger);
            _url = _log_service->url();
        }

        std::string getUrl() const override
        {
            return _url;
        }

        void log(const std::chrono::milliseconds& timestamp,
            LoggerSeverity level,
            const std::string& participant_name,
            const std::string& logger_name, // depends on the Category ...
            const std::string& message) const
        {
            LogProvider::forwardLogToMonitors(
                timestamp, level, participant_name, logger_name, message);
        }

    private:
        std::string _url;
        std::unique_ptr<GlobalLoggingSinkClientService> _log_service;
    };
}
