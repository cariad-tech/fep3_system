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

#include <vector>
#include <string>
#include <chrono>
#include "http_server_rpc_intf_def.h"

namespace fep3
{
namespace rpc
{
namespace catelyn
{

/**
* @brief definition of the external http server interface of the participant
* @see http_server.json file
*/
class IRPCHttpServer : public catelyn::IRPCHttpServerDef
{
protected:
  /**
   * @brief Destroy the IRPCParticipantInfo object
   *
   */
  virtual ~IRPCHttpServer() = default;

public:

  /**
   * @brief Set the heartbeat interval of the participant service bus
   * @return Heartbeat interval in milliseconds
   * @throw runtime_error 
   * @throw If the RPC call fails it will throw a runtime_error
   */
  virtual std::chrono::milliseconds getHeartbeatInterval() const = 0;

  /**
   * @brief Set the heartbeat interval of the participant service bus
   * @param [in] interval_ms in milliseconds
   * @throw If the RPC call fails it will throw a runtime_error
   */
  virtual void setHeartbeatInterval(std::chrono::milliseconds interval_ms) = 0;
};

}
using catelyn::IRPCHttpServer;
}
}
