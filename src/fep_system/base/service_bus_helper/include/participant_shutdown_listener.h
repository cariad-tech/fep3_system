/**
* Copyright 2024 CARIAD SE.
*
* This Source Code Form is subject to the terms of the Mozilla
* Public License, v. 2.0. If a copy of the MPL was not distributed
* with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#pragma once

#include <fep3/components/service_bus/service_bus_intf.h>
#include <functional>


namespace fep3
{
    class ParticipantShutdownListener : public fep3::IServiceBus::IServiceUpdateEventSink
    {
    public:
        using ShutdownNotifyFunction = std::function<void(const std::string participant_name)>;

        ParticipantShutdownListener(
            const std::string& system_name,
            const ShutdownNotifyFunction& shutdown_notify_function);

        virtual ~ParticipantShutdownListener() = default;

        void updateEvent(const fep3::IServiceBus::ServiceUpdateEvent& service_update_event) override;

    private:
        const std::string _system_name;
        ShutdownNotifyFunction _shutdown_notify_function;
    };
}