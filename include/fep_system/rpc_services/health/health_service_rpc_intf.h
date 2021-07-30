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

#include <vector>
#include <string>

#include "health_service_rpc_intf_def.h"
#include <a_util/result.h>
#include "../../base/health/health_types.h"

namespace fep3
{
namespace experimental
{

/**
* @brief definition of the external health service interface of the participan
* @see health.json file
*/
class IRPCHealthService : public IRPCHealthServiceDef
{
protected:
    /**
     * @brief Destroy the IRPCHealthService object
     *
     */
    virtual ~IRPCHealthService() = default;
public:
    /**
     * @brief Reset the health state of this participant to state ok and log the provided message.
     * Resetting the health state to ok indicates all errors have been resolved and the participant
     * works correctly.
     * An error health state should only be reset to health state ok from external using rpc
     * @param message log message indicating why the health state has been set to ok.
     * @return Result indicating whether setting the health state succeeded or failed.
     * @retval ERR_NOERROR Health state has been set successfully.
     */
    virtual a_util::result::Result resetHealth(const std::string& message) = 0;

    /**
     * @brief Get the current health state of the participant.
     *
     * @return HealthState The current health state of the participant.
     */
    virtual HealthState getHealth() const = 0;
};
//using arya::IRPCHealthService;
}
}
