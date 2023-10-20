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
#include "participant_health_listener.h"
namespace fep3
{


    ParticipantHealthListener::ParticipantHealthListener(
        fep3::rpc::IRPCHealthService* rpc_health_service,
        const std::string& participant_name,
        const std::string& system_name,
        LoggingFunction logging_function)
        : _rpc_health_service(rpc_health_service)
        , _participant_name(participant_name)
        , _system_name(system_name)
        , _logging_function(std::move(logging_function))
    {
        if (!_rpc_health_service)
        {
            _logging_function(LoggerSeverity::warning, "RPC Health service is null, connection to service probably failed for participant " + _participant_name);
        }
    }

    void ParticipantHealthListener::updateEvent(const fep3::IServiceBus::ServiceUpdateEvent& service_update_event)
    {
        // do not lock the rpc call
        if ((_participant_name == service_update_event.service_name) &&
            (_system_name == service_update_event.system_name) &&
            _rpc_health_service)
        {
            {
                auto jobs_healthiness = _rpc_health_service->getHealth();
                std::lock_guard<std::mutex> lock(_health_mutex);
                _participant_health.system_time = std::chrono::steady_clock::now();
                _participant_health.jobs_healthiness = std::move(jobs_healthiness);
                if (_logging_active)
                {
                    _logging_function(LoggerSeverity::debug, "Received update event from " + _participant_name);
                }
            }
        }
    }

    ParticipantHealthUpdate ParticipantHealthListener::getParticipantHealth() const
    {
        std::lock_guard<std::mutex> lock(_health_mutex);
        return _participant_health;
    }


    void ParticipantHealthListener::deactivateLogging()
    {
        std::lock_guard<std::mutex> lock(_health_mutex);
        _logging_active = false;
    }

}
