/**
 * @file
 * @copyright
 * @verbatim
Copyright 2023 CARIAD SE. 

This Source Code Form is subject to the terms of the Mozilla
Public License, v. 2.0. If a copy of the MPL was not distributed
with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
@endverbatim
 */


#pragma once

#include <vector>
#include <string>

#include <fep_system/healthiness_types.h>
#include <fep3/rpc_services/health/health_service_rpc_intf_def.h>

namespace fep3
{
namespace rpc
{
namespace catelyn
{
/**
* @brief definition of the external health service interface of the participant
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
     * @return Result indicating whether setting the health state succeeded or failed.
     * @retval ERR_NOERROR Health state has been reset successfully.
     */
    virtual a_util::result::Result resetHealth() = 0;

    /**
     * @brief Get the current health state of the participant.
     *
     * @return HealthState The current health state of the participant.
     */
    virtual std::vector<JobHealthiness> getHealth() const = 0;
};
} // namespace catelyn
using catelyn::IRPCHealthService;
} //namespace rpc
} // namespace fep3
