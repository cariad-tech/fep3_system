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

#include "participant_statemachine_rpc_intf_def.h"


namespace fep3
{
namespace rpc
{
namespace arya
{
    /**
     * @brief Participant State
     *
     */
    enum ParticipantState
    {
        // [[deprecated]], undefined state is deprecated. only unreachable is used
        undefined,
        ///This state is used if the participant is shut down or
        /// the state cannot be obtained from the participant's state machine (no answer)
        unreachable,
        ///valid unloaded state
        unloaded,
        ///valid loaded state
        loaded,
        ///valid initialized state
        initialized,
        ///valid paused state
        paused,
        ///valid running state
        running
    };
}
namespace catelyn
{
/**
* @brief definition of the external service interface of the participant itself
* @see participant_statemachine.json file
*/
class IRPCParticipantStateMachine : public fep3::rpc::catelyn::IRPCParticipantStateMachineDef
{
protected:
    /**
     * @brief Destroy the IRPCParticipantInfo object
     *
     */
    virtual ~IRPCParticipantStateMachine() = default;

public:
    /**
     * @brief State for participants
     * @see ParticipantState
     */
    using State = fep3::rpc::arya::ParticipantState;
    /**
     * @brief Get the state of the participant
     *
     * @return see State
     */
    virtual State getState() const = 0;
    /**
     * @brief sends a load event to the participants state machine
     * @throw logical_error if the event is invalid
     *
     */
    virtual void load() = 0;
    /**
     * @brief sends an unload event to the participants state machine
     * @throw logical_error if the event is invalid
     *
     */
    virtual void unload() = 0;
    /**
     * @brief sends an initialize event to the participants state machine
     * @throw logical_error if the event is invalid
     *
     */
    virtual void initialize() = 0;
    /**
     * @brief sends a deinitialize event to the participants state machine
     * @throw logical_error if the event is invalid
     *
     */
    virtual void deinitialize() = 0;
    /**
     * @brief sends a start event to the participants state machine
     * @throw logical_error if the event is invalid
     *
     */
    virtual void start() = 0;
    /**
     * @brief sends a pause event to the participants state machine
     * @throw logical_error if the event is invalid
     *
     */
    virtual void pause() = 0;
    /**
     * @brief sends a stop event to the participants state machine
     * @throw logical_error if the event is invalid
     *
     */
    virtual void stop() = 0;
    /**
     * @brief sends a shutdown event to the participants state machine
     * @throw logical_error if the event is invalid
     *
     */
    virtual void shutdown() = 0;
};
}
using catelyn::IRPCParticipantStateMachine;
using arya::ParticipantState;
}
}
