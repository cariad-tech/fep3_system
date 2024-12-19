/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fep_state_transition_controller.h"
#include "test_state_transition_helper_common.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ParticipantState = fep3::rpc::ParticipantState;
using testing::Test;
using testing::UnorderedElementsAre;
using testing::Field;

struct StateTransitionController : Test {
    using ParticipantStates = std::map<std::string, ParticipantState>;
    StateTransitionController()
    {
        std::transform(fake_proxies.begin(),
                      fake_proxies.end(),
                       std::back_insert_iterator(fake_proxies_ref),
                       [](FakeProxy& proxy) { return std::ref(proxy); });
    }

    protected:
    fep3::TransitionController _transition_controller;

    std::vector<FakeProxy> fake_proxies = {
        {0, "Proxy1"}, {0, "Proxy2"}, {0, "Proxy3"}, {0, "Proxy4"}, {0, "Proxy5"}};

     std::vector<std::reference_wrapper<FakeProxy>> fake_proxies_ref;
};

TEST_F(StateTransitionController, getParticipantsInState__targetStateLoaded)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::unloaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::initialized},
                                {"Proxy4", ParticipantState::running},
                                {"Proxy5", ParticipantState::loaded}};

    auto participants_in_state = _transition_controller.getParticipantsInState(
        states, ParticipantState::loaded, fake_proxies_ref);

    ASSERT_THAT(participants_in_state,
                UnorderedElementsAre(Field(&FakeProxy::_name, "Proxy2"),
                                     Field(&FakeProxy::_name, "Proxy5")));
}

TEST_F(StateTransitionController, getParticipantsInState__noParticipantsInStateRunning)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::unloaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::unreachable},
                                {"Proxy5", ParticipantState::initialized}};

    auto participants_in_state = _transition_controller.getParticipantsInState(
        states, ParticipantState::running, fake_proxies_ref);

    ASSERT_TRUE(participants_in_state.empty());
}

TEST_F(StateTransitionController, getParticipantsInState_noParticipants)
{
    ParticipantStates states = {};

    auto participants_in_state = _transition_controller.getParticipantsInState(
        states, ParticipantState::loaded, fake_proxies_ref);

    ASSERT_TRUE(participants_in_state.empty());
}

TEST_F(StateTransitionController, homogeneousTargetStateAchieved__failed)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::unloaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::unreachable},
                                {"Proxy5", ParticipantState::initialized}};

    ASSERT_FALSE(_transition_controller.homogeneousTargetStateAchieved(
        states, ParticipantState::initialized));
}

TEST_F(StateTransitionController, homogeneousTargetStateAchieved__allProxiesHaveSameState_failed)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::initialized},
                                {"Proxy2", ParticipantState::initialized},
                                {"Proxy3", ParticipantState::initialized},
                                {"Proxy4", ParticipantState::initialized},
                                {"Proxy5", ParticipantState::initialized}};

    ASSERT_FALSE(
        _transition_controller.homogeneousTargetStateAchieved(states, ParticipantState::loaded));
}

TEST_F(StateTransitionController, homogeneousTargetStateAchieved__successful)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::loaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::loaded},
                                {"Proxy5", ParticipantState::loaded}};

    ASSERT_TRUE(
        _transition_controller.homogeneousTargetStateAchieved(states, ParticipantState::loaded));
}

TEST_F(StateTransitionController, getNextParticipantsState__fromUnloadedToRunning_loaded)
{
    ASSERT_EQ(_transition_controller.getNextParticipantsState(ParticipantState::unloaded,
                                                              ParticipantState::running),
              ParticipantState::loaded);
}

TEST_F(StateTransitionController, getNextParticipantsState__fromInitializedToRunning_running)
{
    ASSERT_EQ(_transition_controller.getNextParticipantsState(ParticipantState::initialized,
                                                              ParticipantState::running),
              ParticipantState::running);
}

TEST_F(StateTransitionController, getNextParticipantsState__fromRunningToUnloaded_initialized)
{
    ASSERT_EQ(_transition_controller.getNextParticipantsState(ParticipantState::running,
                                                              ParticipantState::unloaded),
              ParticipantState::initialized);
}

/// @brief The error handling will check if any participant is unreachable
/// nevertheless we should check if no crash occurs.
TEST_F(StateTransitionController, getNextParticipantsState__fromUnreachableToURunning_unreachable)
{
    ASSERT_EQ(_transition_controller.getNextParticipantsState(ParticipantState::unreachable,
                                                              ParticipantState::running),
              ParticipantState::unreachable);
}

/// @brief The error handling will check if any participant is unreachable
/// nevertheless we should check if no crash occurs.
TEST_F(StateTransitionController, getNextParticipantsState__fromRunningToUndefined_unreachable)
{
    ASSERT_EQ(_transition_controller.getNextParticipantsState(ParticipantState::running,
                                                              ParticipantState::undefined),
              ParticipantState::unreachable);
}

TEST_F(StateTransitionController,
       participantStateToTrigger__allParticipantsBelowOrEqualTargetStart__StartFromLowerState)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::unloaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::initialized},
                                {"Proxy5", ParticipantState::initialized}};

    ASSERT_EQ(_transition_controller.participantStateToTrigger(states, ParticipantState::running),
              ParticipantState::unloaded);
}

TEST_F(StateTransitionController,
       participantStateToTrigger__allParticipantsBelowOrEqualTargetStart__StartFromLowerState_2)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::unloaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::initialized},
                                {"Proxy5", ParticipantState::running}};

    ASSERT_EQ(_transition_controller.participantStateToTrigger(states, ParticipantState::running),
              ParticipantState::unloaded);
}

TEST_F(StateTransitionController,
       participantStateToTrigger__allParticipantsBelowOrEqualTargetStart__StartFromLowerState_3)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::loaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::loaded},
                                {"Proxy5", ParticipantState::loaded}};

    ASSERT_EQ(_transition_controller.participantStateToTrigger(states, ParticipantState::running),
              ParticipantState::loaded);
}

TEST_F(StateTransitionController,
       participantStateToTrigger__allParticipantsHigherOrEqualTargetStart__StartFromHigherState)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::loaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::initialized},
                                {"Proxy5", ParticipantState::initialized}};

    ASSERT_EQ(_transition_controller.participantStateToTrigger(states, ParticipantState::unloaded),
              ParticipantState::initialized);
}

TEST_F(StateTransitionController,
       participantStateToTrigger__allParticipantsHigherOrEqualTargetStart__StartFromHigherState_2)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::unloaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::initialized},
                                {"Proxy5", ParticipantState::running}};

    ASSERT_EQ(_transition_controller.participantStateToTrigger(states, ParticipantState::unloaded),
              ParticipantState::running);
}

TEST_F(StateTransitionController,
       participantStateToTrigger__allParticipantsHigherOrLowerToTargetStart__StartFromHigherState)
{
    ParticipantStates states = {{"Proxy1", ParticipantState::unloaded},
                                {"Proxy2", ParticipantState::loaded},
                                {"Proxy3", ParticipantState::loaded},
                                {"Proxy4", ParticipantState::initialized},
                                {"Proxy5", ParticipantState::running}};

    ASSERT_EQ(_transition_controller.participantStateToTrigger(states, ParticipantState::loaded),
              ParticipantState::running);
}