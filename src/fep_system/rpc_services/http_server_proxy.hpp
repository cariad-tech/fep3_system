
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


#include "http_server_proxy_stub.h"
#include "rpc_services/service_bus/http_server_rpc_intf.h"

#include <components/service_bus/rpc/fep_rpc_stubs_client.h>
#include <fep3/components/service_bus/rpc/fep_rpc.h>
#include <string>
#include <iostream>

namespace fep3::rpc::catelyn
{

class HttpServerProxy : public rpc::arya::RPCServiceClientProxy<rpc_proxy_stub::RPCHttpServerProxy,
    IRPCHttpServer>
{
private:
    using base_type =
        rpc::RPCServiceClientProxy<rpc_proxy_stub::RPCHttpServerProxy, IRPCHttpServer>;
    using base_type::GetStub;

public:

    HttpServerProxy(
            std::string rpc_componnet_name,
            std::shared_ptr<IRPCRequester> rpc) :
            base_type(std::move(rpc_componnet_name), rpc)
    {
    }

    std::chrono::milliseconds getHeartbeatInterval() const override
    {
        try
        {
            const auto ret_value = GetStub().getHeartbeatInterval();
            return std::chrono::milliseconds(ret_value["interval_ms"].asInt64());
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error(ex.what());
        }
    }

    void setHeartbeatInterval(std::chrono::milliseconds interval_ms) override
    {
        try
        {
            GetStub().setHeartbeatInterval(static_cast<int>(interval_ms.count()));
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error(ex.what());
        }
    }

};
}
