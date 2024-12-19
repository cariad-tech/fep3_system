/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fep_system_state_tree.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::ContainerEq;
using ParticipantState = fep3::rpc::ParticipantState;

TEST(FepSystemStateTree, getStateTransitionPath__unloadedTorunning)
{
    ASSERT_THAT(fep3::getStateTransitionPath(ParticipantState::unloaded, ParticipantState::running),
                ContainerEq(std::list{ParticipantState::unloaded,
                                      ParticipantState::loaded,
                                      ParticipantState::initialized,
                                      ParticipantState::running}));
}

TEST(FepSystemStateTree, getStateTransitionPath__unloadedTopaused)
{
    ASSERT_THAT(fep3::getStateTransitionPath(ParticipantState::unloaded, ParticipantState::paused),
                ContainerEq(std::list{ParticipantState::unloaded,
                                      ParticipantState::loaded,
                                      ParticipantState::initialized,
                                      ParticipantState::paused}));
}

TEST(FepSystemStateTree, getStateTransitionPath__runningTopaused)
{
    ASSERT_THAT(fep3::getStateTransitionPath(ParticipantState::running, ParticipantState::paused),
                ContainerEq(std::list{ParticipantState::running, ParticipantState::paused}));
}

TEST(FepSystemStateTree, getStateTransitionPath__pausedTorunning)
{
    ASSERT_THAT(fep3::getStateTransitionPath(ParticipantState::paused, ParticipantState::running),
                ContainerEq(std::list{ParticipantState::paused, ParticipantState::running}));
}

TEST(FepSystemStateTree, getStateTransitionPath__pausedToinitialized)
{
    ASSERT_THAT(
        fep3::getStateTransitionPath(ParticipantState::paused, ParticipantState::initialized),
        ContainerEq(std::list{ParticipantState::paused, ParticipantState::initialized}));
}

TEST(FepSystemStateTree, getStateTransitionPath__initializedTopaused)
{
    ASSERT_THAT(
        fep3::getStateTransitionPath(ParticipantState::initialized, ParticipantState::paused),
        ContainerEq(std::list{ParticipantState::initialized, ParticipantState::paused}));
}

TEST(FepSystemStateTree, getStateTransitionPath__runningToloaded)
{
    ASSERT_THAT(fep3::getStateTransitionPath(ParticipantState::running, ParticipantState::loaded),
                ContainerEq(std::list{ParticipantState::running,
                                      ParticipantState::initialized,
                                      ParticipantState::loaded}));
}

TEST(FepSystemStateTree, getStateTransitionPath__unreachableToloaded)
{
    ASSERT_TRUE(
        fep3::getStateTransitionPath(ParticipantState::unreachable, ParticipantState::loaded)
            .empty());
}

TEST(FepSystemStateTree, getStateTransitionPath__unreachableTorunning)
{
    ASSERT_TRUE(
        fep3::getStateTransitionPath(ParticipantState::unreachable, ParticipantState::running)
            .empty());
}

TEST(FepSystemStateTree, getStateTransitionPath__undefinedToloaded)
{
    ASSERT_TRUE(fep3::getStateTransitionPath(ParticipantState::undefined, ParticipantState::loaded)
                    .empty());
}

TEST(FepSystemStateTree, getStateTransitionPath__runningToundefined)
{
    ASSERT_TRUE(fep3::getStateTransitionPath(ParticipantState::running, ParticipantState::undefined)
                    .empty());
}

TEST(FepSystemStateTree, getStateTransitionPath__runningTounreachable)
{
    ASSERT_THAT(
        fep3::getStateTransitionPath(ParticipantState::running, ParticipantState::unreachable),
        ContainerEq(std::list{ParticipantState::running,
                              ParticipantState::initialized,
                              ParticipantState::loaded,
                              ParticipantState::unloaded,
                              ParticipantState::unreachable}));
}

TEST(FepSystemStateTree, getStateTransitionPath__loadedTounreachable)
{
    ASSERT_THAT(
        fep3::getStateTransitionPath(ParticipantState::loaded, ParticipantState::unreachable),
        ContainerEq(std::list{
            ParticipantState::loaded, ParticipantState::unloaded, ParticipantState::unreachable}));
}

TEST(FepSystemStateTree, getStateTransitionPath__unloadedTounreachable)
{
    ASSERT_THAT(
        fep3::getStateTransitionPath(ParticipantState::unloaded, ParticipantState::unreachable),
        ContainerEq(std::list{ParticipantState::unloaded, ParticipantState::unreachable}));
}

TEST(FepSystemStateTree, getStateTransitionPath__runningTorunning)
{
    ASSERT_THAT(fep3::getStateTransitionPath(ParticipantState::running, ParticipantState::running),
                ContainerEq(std::list{ParticipantState::running}));
}

TEST(FepSystemStateTree, getStateTransitionPath__unreachableTounreachable)
{
    ASSERT_THAT(
        fep3::getStateTransitionPath(ParticipantState::unreachable, ParticipantState::unreachable),
        ContainerEq(std::list{ParticipantState::unreachable}));
}

TEST(FepSystemStateTree, getStateTransitionPath__undefinedToundefined)
{
    ASSERT_THAT(
        fep3::getStateTransitionPath(ParticipantState::undefined, ParticipantState::undefined),
        ContainerEq(std::list{ParticipantState::undefined}));
}
