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


namespace fep3
{
namespace rpc
{
namespace experimental
{

/**
* @brief definition of the a rpc passthrough service which gives direct access to the rpc methods of another service.
*/
class IRPCPassthrough
{
protected:
    /**
     * @brief Destroy the IRPCPassthrough object
     *
     */
    virtual ~IRPCPassthrough() = default;

public:
    ///definition of the FEP rpc service iid of the rpc passthrough service
    FEP_RPC_IID("rpc_passthrough.experimental.rpc.fep3.iid", "rpc_passthrough");

    /**
    * @brief passthrough the json rpc request directly to the service
    * @param [in] request Request string from json rpc 
    * @param [out] response Result of rpc call as json rpc string
    * @retval true if everything works fine
    */
    virtual bool call(const std::string & request, std::string & response) = 0;

};
}
}
}
