/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2022 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */

#include "service_bus_wrapper.h"
#include <fep3/base/component_registry/component_registry_factory.h>
#include <stdexcept>

#define FEP3_SYSTEM_COMPONENTS_FILE_PATH_ENVIRONMENT_VARIABLE "FEP3_SYSTEM_COMPONENTS_FILE_PATH"

static constexpr const char* const plugin_filename = "fep3_system.fep_components";

namespace fep3
{
    ServiceBusWrapper::ServiceBusWrapper(std::shared_ptr<fep3::ComponentRegistry> component_registry)
        : _component_registry(component_registry)
    {
        _service_bus = _component_registry->getComponent<fep3::IServiceBus>();
        if (nullptr == _service_bus)
        {
            throw std::runtime_error(
                "can not create a service bus connection. no plugin supports "
                + fep3::getComponentIID<fep3::IServiceBus>());
        }
    }

    fep3::IServiceBus* ServiceBusWrapper::createOrGetServiceBusConnection(const std::string& system_name,
        const std::string& system_url)
    {
        auto system_access = _service_bus->getSystemAccessCatelyn(system_name);
        if (!system_access)
        {
            _service_bus->createSystemAccess(system_name, system_url);
        }
        return _service_bus;
    }

    ServiceBusWrapper getServiceBusWrapper()
    {
        // here we pass the ownership to the caller and we give back the same pointer in case it is still in use
        // we cannot make a static ComponentRegistry (Service bus statically will cause static de initialization problems with dds participant)
        //(https://community.rti.com/forum-topic/ddstheparticipantfactory-deleteparticipant-hangs)
        static std::mutex _sync_get;
        static std::weak_ptr<fep3::ComponentRegistry> _component_registry;

        std::unique_lock lock{ _sync_get };

        if (auto observe = _component_registry.lock()) {
            if (!observe)
            {
                observe =  fep3::ComponentRegistryFactory::createRegistry(nullptr, FEP3_SYSTEM_COMPONENTS_FILE_PATH_ENVIRONMENT_VARIABLE, plugin_filename);
                _component_registry = observe;
            }
            return ServiceBusWrapper(observe);
        }
        else
        {
            auto component_registry = fep3::ComponentRegistryFactory::createRegistry(nullptr, FEP3_SYSTEM_COMPONENTS_FILE_PATH_ENVIRONMENT_VARIABLE, plugin_filename);
            _component_registry = component_registry;
            return ServiceBusWrapper(component_registry);
        }
    }
}
