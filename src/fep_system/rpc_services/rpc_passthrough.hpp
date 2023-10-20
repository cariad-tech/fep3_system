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
#include <string>

#include <components/service_bus/rpc/fep_rpc_stubs_client.h>
#include <fep_system_stubs/participant_info_proxy_stub.h>

#include "rpc_services/rpc_passthrough/rpc_passthrough_intf.h"
#include <a_util/strings.h>

namespace fep3
{
namespace rpc
{
namespace experimental
{

class RPCPassthrough: public arya::IRPCServiceClient, public IRPCPassthrough
{
public:

    /**
    * CTOR
    *
    * @param [in] service_name The name of the RPC service / RPC component we connect to
    * @param [in] rpc The RPC implementation
    */
    explicit RPCPassthrough(const std::string& service_name,
        const std::shared_ptr<IRPCRequester>& rpc_requester):
        _service_name(service_name),
        _rpc_requester(rpc_requester)
    {
    }

    class ResponseHelper : public IRPCRequester::IRPCResponse
    {
    public:
        ResponseHelper(std::string & response) : _response(response)
        {

        }

        fep3::Result set(const std::string& response)
        {
            _response = response;
            return {};
        }
    private:
        std::string& _response;
    };

    bool call(const std::string & request, std::string & response)
    {
        ResponseHelper response_helper(response);
        return static_cast<bool>(_rpc_requester->sendRequest(_service_name, request, response_helper));
    }

public: // implement IRPCServiceClient
                
    std::string getRPCServiceIID() const override
    {
        return getRPCIID();
    }

    std::string getRPCServiceDefaultName() const override
    {
        return getRPCDefaultName();
    }

    std::string getRPCServiceName() const override
    {
        return _service_name;
    }
           
private:
    std::string _service_name;
    std::shared_ptr<IRPCRequester> _rpc_requester;

};
}
}
}
