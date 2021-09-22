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

#include <fep3/components/service_bus/rpc/fep_rpc.h>
#include <event_listener_registration_service_stub.h>
#include "event_listener_rpc_intf_def.h"

namespace fep3
{
namespace test
{
using EventListenerRegistrationService = rpc::RPCService<fep3::rpc_stubs::RPCEventListenerRegistrationServiceStub,
    fep3::rpc::IRPCEventListenerRegistrationDef>;

class LocalSystemSimClockWithTimeout;

class EventListenerRegistrationServiceImpl : public EventListenerRegistrationService
{
public:
    explicit EventListenerRegistrationServiceImpl(LocalSystemSimClockWithTimeout& clock);
    int registerEventListenerRPCService(const std::string& address) override;
    int unregisterEventListenerRPCService(const std::string& address) override;

private:
    LocalSystemSimClockWithTimeout& _clock;
};
} // namespace test
} // namespace fep3