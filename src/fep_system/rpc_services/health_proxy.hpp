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

#include "base/health/health_types.h"
#include "health_service_proxy_stub.h"
#include "rpc_services/health/health_service_rpc_intf.h"

#include <components/service_bus/rpc/fep_rpc_stubs_client.h>
#include <fep3/components/service_bus/rpc/fep_rpc.h>
#include <string>

namespace fep3
{
namespace experimental
{
//we use the namespace here to create versioned Proxies if something changes in future
namespace arya
{
class HealthServiceProxy : public rpc::arya::RPCServiceClientProxy<rpc_proxy_stub::RPCHealthServiceProxy,
    IRPCHealthService>
{
private:
    using base_type =
        rpc::RPCServiceClientProxy<rpc_proxy_stub::RPCHealthServiceProxy, IRPCHealthService>;

public:
    using base_type::GetStub;
    HealthServiceProxy(
        std::string rpc_component_name,
        std::shared_ptr<::fep3::IRPCRequester> rpc) :
        base_type(rpc_component_name, rpc)
    {
    }

    experimental::HealthState getHealth() const override
    {
        try
        {
            return static_cast<experimental::HealthState>(GetStub().getHealth());
        }
        catch (const std::exception&)
        {
            return experimental::HealthState::unknown;
        }
    }

    fep3::Result resetHealth(const std::string& message) override
    {
        try
        {
            return ::rpc::cJSONConversions::json_to_result(GetStub().resetHealth(message));
        }
        catch (const std::exception& exception)
        {
            return fep3::Result{ERR_UNKNOWN, exception.what(), 0, "", ""};
        }
    }

};
}
}
}
