/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <fep3/base/component_registry/component_registry.h>

#include <fep3/components/service_bus/service_bus_intf.h>
#include <mutex>

namespace fep3
{
    class ServiceBusWrapper
    {
    public:
        ServiceBusWrapper(std::shared_ptr<fep3::ComponentRegistry> component_registry);

        fep3::IServiceBus* createOrGetServiceBusConnection(const std::string& system_name,
            const std::string& system_url);

    private:
        std::shared_ptr<fep3::ComponentRegistry> _component_registry;
        fep3::IServiceBus* _service_bus;
    };

    ServiceBusWrapper getServiceBusWrapper();
}
