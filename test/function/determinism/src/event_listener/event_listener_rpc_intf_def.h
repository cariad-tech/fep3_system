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

#ifndef _FEP3_RPC_EVENT_LISTENER_INTF_DEF_H_
#define _FEP3_RPC_EVENT_LISTENER_INTF_DEF_H_

//very important to have this relative! system library!
#include "fep3/rpc_services/base/fep_rpc_iid.h"

namespace fep3
{
namespace rpc
{
namespace arya
{
/**
 * @brief Definition of the service interface of the RPC event listener registration which is provided by the participant
 * @see event_listener_registration.json
 */
class IRPCEventListenerRegistrationDef
{
protected:
    /// DTOR
    ~IRPCEventListenerRegistrationDef() = default;

public:
    ///definition of the FEP rpc event listener registration service iid for the participant
    FEP_RPC_IID("event_listener_registration.arya.fep3.iid", "event_listener_registration");
};
/**
 * @brief Definition of the client interface of the RPC event listener which is used by the
 *        participant to send events to.
 * @see event_listener.json
 */
class IRPCEventListenerDef
{
protected:
    /// DTOR
    ~IRPCEventListenerDef() = default;

public:
    ///definition of the FEP rpc event listener client iid for the participant
    FEP_RPC_IID("event_listener.arya.fep3.iid", "event_listener");
};
} // namespace arya
using arya::IRPCEventListenerRegistrationDef;
using arya::IRPCEventListenerDef;
} // namespace rpc
} // namespace fep3

#endif //_FEP3_RPC_EVENT_LISTENER_INTF_DEF_H_