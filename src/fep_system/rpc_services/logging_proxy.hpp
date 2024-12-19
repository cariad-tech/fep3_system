/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#pragma once

#include <fep3/components/service_bus/rpc/fep_rpc.h>

#include "rpc_services/logging/logging_rpc_intf.h"
#include "fep_system_stubs/logging_service_proxy_stub.h"
#include "fep_system_stubs/logging_sink_service_stub.h"
#include "system_logger_intf.h"


namespace fep3
{
namespace rpc
{
namespace arya
{

using LoggingServiceClientProxy
    = RPCServiceClientProxy< rpc_proxy_stub::RPCLoggingServiceProxy,
                             IRPCLoggingService >;

using LoggingServiceClient
= RPCServiceClient< rpc_proxy_stub::RPCLoggingServiceProxy,
    IRPCLoggingService >;


class LoggingServiceProxy : public LoggingServiceClientProxy
{
private:

public:
    using LoggingServiceClientProxy::GetStub;
    LoggingServiceProxy(
        const std::string& rpc_component_name,
        const std::shared_ptr<IRPCRequester>& rpc,
        const std::string& participant_name,
        ISystemLogger& logger) :
        LoggingServiceClientProxy(rpc_component_name, rpc),
        _participant_name(participant_name),
        _logger(logger),
        _rpc_component_name(rpc_component_name),
        _rpc(rpc)
    {
    }

    bool setLoggerFilter(const std::string& logger_name,
        const LoggerFilter& configuration) const override
    {
        try
        {
            int32_t retval = GetStub().setLoggerFilter(a_util::strings::join(configuration._enabled_logging_sinks, ","),
                logger_name,
                static_cast<int>(configuration._severity));
            if (retval)
            {
                _logger.logProxyError(
                    LoggerSeverity::error,
                    _participant_name,
                    "logging_service",
                    std::string("Setting logger filter failed with error code: ") + std::to_string(retval));
                return false;
            }
        }
        catch (...)
        {
            _logger.logProxyError(
                LoggerSeverity::fatal,
                _participant_name,
                "logging_service",
                std::string("Setting logger filter failed: RPC communication failed"));
            return false;
        }
        return true;
    }


    LoggerFilter getLoggerFilter(const std::string& logger_name) const
    {
        try
        {
            auto retval = GetStub().getLoggerFilter(logger_name);
            LoggerFilter ret_struct;
            ret_struct._enabled_logging_sinks = a_util::strings::split(retval["enable_sinks"].asString(), ",");
            ret_struct._severity = static_cast<LoggerSeverity>(retval["severity"].asInt());
            return ret_struct;
        }
        catch (...)
        {
            _logger.logProxyError(
                LoggerSeverity::fatal,
                _participant_name,
                "logging_service",
                std::string("getting logger filter failed: RPC communication failed"));
            return {};
        }
    }

    std::vector<std::string> getLoggers() const override
    {
        try
        {
            auto retval = GetStub().getLoggers();
            return a_util::strings::split(retval, ",");
        }
        catch (...)
        {
            _logger.logProxyError(
                LoggerSeverity::fatal,
                _participant_name,
                "logging_service",
                std::string("getting loggers failed: RPC communication failed"));
            return {};
        }
    }

    std::vector<std::string> getSinks() const override
    {
        try
        {
            auto retval = GetStub().getSinks();
            return a_util::strings::split(retval, ",");
        }
        catch (...)
        {
            _logger.logProxyError(
                LoggerSeverity::fatal,
                _participant_name,
                "logging_service",
                std::string("getting sinks failed: RPC communication failed"));
            return {};
        }
    }

    class RPCSinkProperties :
        public LoggingServiceClient,
        public IProperties
    {
    public:

        explicit RPCSinkProperties(const std::string& sink_name,
            const std::string& rpc_component_name,
            const std::shared_ptr<IRPCRequester>& rpc) :
            LoggingServiceClient(rpc_component_name, rpc),
            _sink_name(sink_name)
        {
        }

        bool setProperty(const std::string& name,
            const std::string& value,
            const std::string& type)
        {
            try
            {
                auto retval = GetStub().setSinkProperty(name, _sink_name, type, value);
                if (retval == 0)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            catch (...)
            {
                return false;
            }
        }

        std::string getProperty(const std::string& name) const
        {
            try
            {
                auto retval = GetStub().getSinkProperty(name, _sink_name);
                return retval["value"].asString();
            }
            catch (...)
            {
                return {};
            }
        }
        std::string getPropertyType(const std::string& name) const
        {
            try
            {
                auto retval = GetStub().getSinkProperty(name, _sink_name);
                return retval["type"].asString();
            }
            catch (...)
            {
                return {};
            }
        }
        bool isEqual(const IProperties& properties) const
        {
            try
            {
                auto names = properties.getPropertyNames();
                for (const auto& current_name : names)
                {
                    auto retval = GetStub().getSinkProperty(current_name, _sink_name);
                    if (retval["type"].asString() != properties.getPropertyType(current_name)
                        || retval["value"].asString() != properties.getProperty(current_name))
                    {
                        return false;
                    }
                }
            }
            catch (...)
            {
                return false;
            }
            return true;
        }
        void copyTo(IProperties& properties) const
        {
            auto names = getPropertyNames();
            for (const auto& current_name : names)
            {
                try
                {
                    auto retval = GetStub().getSinkProperty(current_name, _sink_name);
                    properties.setProperty(current_name, retval["value"].asString(), retval["type"].asString());
                }
                catch (...)
                {
                    //go ahead
                }
            }
        }
        std::vector<std::string> getPropertyNames() const
        {
            try
            {
                return a_util::strings::split(GetStub().getSinkProperties(_sink_name), ",");
            }
            catch (...)
            {
                return {};
            }
        }

    private:
        std::string _sink_name;
    };


    std::shared_ptr<IProperties> getProperties(const std::string& sink_name) const
    {
        return std::make_shared<RPCSinkProperties>(sink_name, _rpc_component_name, _rpc);
    }


private:
    ISystemLogger&  _logger;
    std::string _participant_name;

    //need it for properties then
    std::string _rpc_component_name;
    std::shared_ptr<IRPCRequester> _rpc;
};


using LoggingSinkServiceProxy
= RPCServiceClientProxy< rpc_proxy_stub::RPCLoggingSinkService,
    IRPCLoggingSinkService >;

// prevents deregistration of the proxy from the participant rpc logging sink
// when other logging proxies are registered.
class RPCLoggingSinkRegistrationSingleton
{
public:
    RPCLoggingSinkRegistrationSingleton(RPCLoggingSinkRegistrationSingleton const&) = delete;
    RPCLoggingSinkRegistrationSingleton& operator=(RPCLoggingSinkRegistrationSingleton const&) = delete;

    static RPCLoggingSinkRegistrationSingleton* getInstance()
    {
        // Static local variable initialization is thread-safe
        // and will be initialized only once.
        static RPCLoggingSinkRegistrationSingleton instance{};
        return &instance;
    }

    fep3::Result registerRPCClient(RPCComponent<IRPCLoggingSinkService>& logging_sink_service, const std::string &url,
        const std::string participant_name, const std::string &system_name)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        const std::string multiplexed_name = url + "@" + system_name + "@" + participant_name;

        if (const auto return_code = logging_sink_service->registerRPCClient(url); return_code == 0)
        {
            ++(_client_count[multiplexed_name]);
        }
        else
        {
            RETURN_ERROR_DESCRIPTION(
                fep3::ERR_FAILED,
                std::string("Registration of rpc client for: " + multiplexed_name + " failed with return code: " + std::to_string(return_code))
                .c_str());
        }

        return {};
    }

    fep3::Result unregisterRPCClient(RPCComponent<IRPCLoggingSinkService>& logging_sink_service, const std::string &url,
        const std::string participant_name, const std::string &system_name)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        const std::string multiplexed_name = url + "@" + system_name + "@" + participant_name;
        
        if (_client_count[multiplexed_name] == 1)
        {
            if (const auto return_code = logging_sink_service->unregisterRPCClient(url); return_code == 0)
            {
                --(_client_count[multiplexed_name]);
            }
            else
            {
                RETURN_ERROR_DESCRIPTION(
                    fep3::ERR_FAILED,
                    std::string("Deregistration of rpc client for: " + multiplexed_name + " failed with return code: " + std::to_string(return_code))
                    .c_str());
            }
        }
        else if (_client_count[multiplexed_name] > 1)
        {
            --(_client_count[multiplexed_name]);
        }

        return {};
    }

private:
    RPCLoggingSinkRegistrationSingleton() {}
    std::mutex _mutex;
    std::map<std::string, uint32_t> _client_count;
};

class LoggingSinkService : public LoggingSinkServiceProxy
{
protected:
    using LoggingSinkServiceProxy::GetStub;
public:
    LoggingSinkService(const std::string& component_name, const std::shared_ptr<IRPCRequester>& rpc) :
        LoggingSinkServiceProxy(component_name, rpc)
    {
    }

    int registerRPCClient(const std::string& url) override
    {
        try
        {
            return GetStub().registerRPCLoggingSinkClient(url, "", static_cast<int>(LoggerSeverity::info));
        }
        catch (...)
        {
            return -1;
        }
    }

    int unregisterRPCClient(const std::string& url) override
    {
        try
        {
            return GetStub().unregisterRPCLoggingSinkClient(url);
        }
        catch (...)
        {
            return -1;
        }
    }
};

} // namespace arya
} // namespace rpc
} // namespace fep3
