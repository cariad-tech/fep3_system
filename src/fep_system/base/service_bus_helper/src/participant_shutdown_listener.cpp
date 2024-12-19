/**
* Copyright 2024 CARIAD SE.
*
* This Source Code Form is subject to the terms of the Mozilla
* Public License, v. 2.0. If a copy of the MPL was not distributed
* with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include "participant_shutdown_listener.h"
namespace fep3
{
    ParticipantShutdownListener::ParticipantShutdownListener(
        const std::string& system_name,
        const ShutdownNotifyFunction& shutdown_notify_function)
        : _system_name(system_name)
        , _shutdown_notify_function(shutdown_notify_function)
    {
    }

    void ParticipantShutdownListener::updateEvent(const fep3::IServiceBus::ServiceUpdateEvent& service_update_event)
    {
        if ((_system_name == service_update_event.system_name) && 
            (service_update_event.event_type == fep3::IServiceBus::ServiceUpdateEventType::notify_byebye)) {
            
            _shutdown_notify_function(service_update_event.service_name);
        }
    }
}