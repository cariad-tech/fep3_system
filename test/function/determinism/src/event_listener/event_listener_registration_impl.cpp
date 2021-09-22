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


#include "event_listener_registration_impl.h"

#include "../clock/local_system_clock_discrete.h"

namespace fep3
{
namespace test
{

EventListenerRegistrationServiceImpl::EventListenerRegistrationServiceImpl(LocalSystemSimClockWithTimeout& clock)
    : _clock(clock)
{
}

int EventListenerRegistrationServiceImpl::registerEventListenerRPCService(const std::string& address)
{
    return _clock.registerEventListenerRPCService(address);
}

int EventListenerRegistrationServiceImpl::unregisterEventListenerRPCService(const std::string& address)
{
    return _clock.unregisterEventListenerRPCService(address);
}

} // namespace test
} // namespace fep3