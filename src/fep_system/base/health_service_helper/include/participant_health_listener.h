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
#pragma once

#include <fep3/components/service_bus/service_bus_intf.h>
#include "fep_system/rpc_services/health/health_service_rpc_intf.h"
#include "fep_system/system_logger_intf.h"
#include "fep_system/healthiness_types.h"
#include "fep_system/rpc_component_proxy.h"
#include <tuple>
#include <mutex>
#include <functional>


namespace fep3
{
    class ParticipantHealthListener : public fep3::IServiceBus::IServiceUpdateEventSink
    {
    public:
        using LoggingFunction = std::function<void(LoggerSeverity, const std::string&)>;

        ParticipantHealthListener(
            fep3::rpc::IRPCHealthService* rpc_health_service,
            const std::string& participant_name,
            const std::string& system_name,
            LoggingFunction logging_function);

        void updateEvent(const fep3::IServiceBus::ServiceUpdateEvent& service_update_event) override;

        ParticipantHealthUpdate getParticipantHealth() const;


        void deactivateLogging();
    private:
        fep3::rpc::IRPCHealthService* _rpc_health_service;
        ParticipantHealthUpdate _participant_health;
        mutable std::mutex _health_mutex;
        LoggingFunction _logging_function;
        const std::string _participant_name;
        const std::string _system_name;
        bool _logging_active = true;
    };
}
