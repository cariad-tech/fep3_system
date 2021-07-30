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


#include "event_listener.h"

#include <event_listener_registration_client_stub.h>
#include "event_listener_rpc_intf.h"
#include <event_listener_service_stub.h>
#include "gmock_async_helper.h"

#include <a_util/strings.h>
#include <a_util/system.h>
#include <fep3/components/service_bus/rpc/fep_rpc_stubs_client.h>
#include <fep3/components/service_bus/rpc/fep_rpc_stubs_service.h>
//#include <fep_system/service_bus_factory.h>
#include <fep3/components/service_bus/service_bus_intf.h> // Can be removed when service_bus_factory.h is used

using EventListenerRegistrationProxy
= fep3::rpc::RPCServiceClientProxy<fep3::rpc_stubs::RPCEventListenerRegistrationClientStub,
    fep3::rpc::IRPCEventListenerRegistration>;

class EventListenerRegistrationClient : public EventListenerRegistrationProxy
{
protected:
    using EventListenerRegistrationProxy::GetStub;

public:
    EventListenerRegistrationClient(const std::string& component_name, const std::shared_ptr<fep3::IRPCRequester>& rpc) :
        EventListenerRegistrationProxy(component_name, rpc)
    {
    }

    int registerRPCClient(const std::string& url) override
    {
        try
        {
            return GetStub().registerEventListenerRPCService(url);
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
            return GetStub().unregisterEventListenerRPCService(url);
        }
        catch (...)
        {
            return -1;
        }
    }
};

// Threadsafe Meyers' Singleton available to the file scope
static ::test::helper::Notification& getNotification()
{
    static ::test::helper::Notification done;
    return done;
}

using EventListenerService = fep3::rpc::RPCService<fep3::rpc_stubs::RPCEventListenerServiceStub,
    fep3::rpc::IRPCEventListenerDef>;

class EventListenerServiceImpl : public EventListenerService
{
public:
    explicit EventListenerServiceImpl()
    {
    }

    int notify()
    {
        getNotification().notify();
        return 0;
    }
};

namespace fep3
{

/**
* Dummy Replacement for ServiceBusFactory since it is not exported. Remove this when actual ServiceBusFactory is used!!
*/
namespace arya
{
class IServiceBusConnection : public fep3::arya::IServiceBus
{
public:
    ~IServiceBusConnection() = default;
};
}

class ServiceBusFactory
{
private:
    ServiceBusFactory() = default;
public:
    static ServiceBusFactory& get()
    {
        static ServiceBusFactory serv_bus_fac;
        return serv_bus_fac;
    }

    std::shared_ptr<arya::IServiceBusConnection> createOrGetServiceBusConnection(
        const std::string& system_name,
        const std::string& system_url)
    {
        return nullptr;
    }
};
/**
* End Dummy ServiceBusFactory
*/

namespace experimental
{
class EventListener::Impl
{
public:
    Impl() = default;

    ~Impl()
    {
        _system_access->getServer()->unregisterService(fep3::rpc::IRPCEventListenerDef::getRPCDefaultName());
        _registration_clients.clear();
    }

    void registerParticipant(std::string participant_name)
    {
        if (_system_access)
        {
            auto requester = _system_access->getRequester(participant_name);
            _registration_clients[participant_name].reset(new EventListenerRegistrationClient(fep3::rpc::IRPCEventListenerRegistrationDef::getRPCDefaultName(),
                requester));
            _registration_clients[participant_name]->registerRPCClient(getUrl());
        }
    }

    void initRPCService(const std::string& system_name)
    {
        //we create an service bus connection without appaering as server that is discoverable
        //so use a separate one!
        _servicebus_connection = fep3::ServiceBusFactory::get().createOrGetServiceBusConnection(system_name, "");
        if (!_servicebus_connection)
        {
            throw std::runtime_error("it is not possible to create or get a service bus connection for event listener on system " + system_name);
        }
        _system_access = _servicebus_connection->getSystemAccess(system_name);
        if (!_system_access)
        {
            auto res = _servicebus_connection->createSystemAccess(system_name, "", false);
            if (isFailed(res))
            {
                throw std::runtime_error("it is not possible to create or get a system access for event listener on system " + system_name);
            }
            else
            {
                _system_access = _servicebus_connection->getSystemAccess(system_name);
            }
        }

        auto result_creation = _system_access->createServer("system_" + system_name + "_" + a_util::strings::toString(getId()),
            fep3::IServiceBus::ISystemAccess::_use_default_url);
        if (isFailed(result_creation))
        {
            throw std::runtime_error("it is not possible to create or get a server for event listener on system " + system_name);
        }
        _system_access->getServer()->registerService(
            fep3::rpc::IRPCEventListenerDef::getRPCDefaultName(),
            std::make_shared<EventListenerServiceImpl>());
    }

    void waitForNotification()
    {
        getNotification().waitForNotification();
    }

private:
    std::string getUrl() const
    {
        if (_system_access)
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

private:
    std::shared_ptr<fep3::IServiceBus::ISystemAccess> _system_access;
    std::shared_ptr<fep3::arya::IServiceBusConnection> _servicebus_connection;

    std::map<std::string, std::unique_ptr<EventListenerRegistrationClient>> _registration_clients;
};

EventListener::EventListener()
    : _impl(std::make_unique<Impl>())
{
}

void EventListener::registerParticipant(std::string participant_name)
{
    _impl->registerParticipant(participant_name);
}

void EventListener::initRPCService(const std::string& system_name)
{
    _impl->initRPCService(system_name);
}

void EventListener::waitForNotification()
{
    _impl->waitForNotification();
}

} // namespace experimental
} // namespace fep3