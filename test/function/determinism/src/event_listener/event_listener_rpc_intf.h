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

#include "event_listener_rpc_intf_def.h"

namespace fep3
{
namespace rpc
{
namespace arya
{
class IRPCEventListenerRegistration : public IRPCEventListenerRegistrationDef
{
protected:
    /// DTOR
    virtual ~IRPCEventListenerRegistration() = default;

public:
    /**
    * @brief registers the address at the service to receive notification events
    *
    * @param[in] url valid url to send the events to
    *
    * @return returns 0 if succeeded
    */
    virtual int registerRPCClient(const std::string& url) = 0;
    /**
    * @brief unregisters the address from the service to stop sending notification events to that address
    *
    * @param[in] url valid url where the events were sent to
    *
    * @return returns 0 if succeeded
    */
    virtual int unregisterRPCClient(const std::string& url) = 0;
};
} // namespace arya
using arya::IRPCEventListenerRegistration;
} // namespace rpc
} // namespace fep3