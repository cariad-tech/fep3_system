/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "execution_policy.h"
#include "fep_system_state_transition.h"
#include "fep_system_state_transition_error_handling.h"
#include "test_state_transition_helper_common.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace fep3;

struct DummyErrorHandling {
    bool operator()(std::vector<TransitionFunctionMetaData>& data)
    {
        return std::find_if(data.begin(),
                            data.end(),
                            [](const TransitionFunctionMetaData& meta_data) -> bool {
                                return meta_data.hasError();
                            }) == data.end();
    }
};

std::atomic<int32_t> FakeProxy::_counter = 0;
/// @brief This fixture focuses on the correct execution of proxies according to their prio
struct ExecutionAccordingToPrio : ::testing::Test {
    auto getSytemStateTransition(SystemStateTransitionPrioSorting sorting)
    {
        return SystemStateTransition([](const FakeProxy& f) -> int32_t { return f.getPriority(); },
                                     sorting,
                                     _dummy_error_handling,
                                     SerialExecutionPolicy{});
    }

    void SetUp() override
    {
        FakeProxy::_counter = 0;
    }

private:
    DummyErrorHandling _dummy_error_handling;
};

TEST_F(ExecutionAccordingToPrio,
       StateTransition__execute__decreasingSortingHigherPriorityTransitionFirst)
{
    std::vector<FakeProxy> ss{
        FakeProxy{1}, FakeProxy{1}, FakeProxy{5}, FakeProxy{5}, FakeProxy{2}, FakeProxy{2}};

    auto state_transition = getSytemStateTransition(SystemStateTransitionPrioSorting::decreasing);
    state_transition.execute(ss, [](FakeProxy& f) { return f.transition(); }, {"loaded"});

    ASSERT_LE(ss.at(2)._called_index, 1)
        << "Highest Prio should be called_index first on decreasing sorting";
    ASSERT_LE(ss.at(3)._called_index, 1)
        << "Highest Prio should be called_index first on decreasing sorting";
    ASSERT_LE(ss.at(4)._called_index, 3)
        << "Middle Prio should be called_index second on decreasing sorting";
    ASSERT_LE(ss.at(5)._called_index, 3)
        << "Middle Prio should be called_index second on decreasing sorting";
    ASSERT_LE(ss.at(0)._called_index, 5)
        << "Lowest Prio should be called_index last on decreasing sorting";
    ASSERT_LE(ss.at(1)._called_index, 5)
        << "Lowest Prio should be called_index last on decreasing sorting";
}

TEST_F(ExecutionAccordingToPrio,
       StateTransition__execute__increasingSortingLowerPriorityTransitionFirst)
{
    std::vector<FakeProxy> ss{FakeProxy{5}, FakeProxy{1}, FakeProxy{2}};

    auto state_transition = getSytemStateTransition(SystemStateTransitionPrioSorting::increasing);
    state_transition.execute(ss, [](FakeProxy& f) { return f.transition(); }, {"loaded"});

    ASSERT_EQ(ss.at(0)._called_index, 2)
        << "Highest Prio should be called_index last on increasing sorting";
    ASSERT_EQ(ss.at(1)._called_index, 0)
        << "Lowest Prio should be called_index first on increasing sorting";
    ASSERT_EQ(ss.at(2)._called_index, 1)
        << "Middle Prio should be called_index second on decreasing sorting";
}

TEST_F(ExecutionAccordingToPrio, StateTransition__execute__noSorting)
{
    std::vector<FakeProxy> ss{FakeProxy{5}, FakeProxy{1}, FakeProxy{2}};

    auto state_transition = getSytemStateTransition(SystemStateTransitionPrioSorting::none);
    state_transition.execute(ss, [](FakeProxy& f) { return f.transition(); }, {"loaded"});

    ASSERT_TRUE(ss.at(0)._called);
    ASSERT_TRUE(ss.at(1)._called);
    ASSERT_TRUE(ss.at(2)._called);
}

TEST_F(ExecutionAccordingToPrio, StateTransition__execute__stopsIfErrorPolicyReturnsError)
{
    int execution_count = 0;
    auto transition_function = [&](FakeProxy& f){
        if (++execution_count == 3)
            return ExecResult{false};
        else {
            f.transition();
            return ExecResult{true};
        }
    };

    std::vector<FakeProxy> ss{FakeProxy{5, "Part1"}, FakeProxy{1, "Part2"}, FakeProxy{2, "Part3"}};
    auto state_transition = getSytemStateTransition(SystemStateTransitionPrioSorting::decreasing);
    ASSERT_FALSE(state_transition.execute(ss, transition_function, {"loaded"}));
    // Part2 transition not called
    ASSERT_FALSE(ss.at(1)._called)
        << "no further transition function calls should be made after failure";
    ASSERT_TRUE(ss.at(0)._called);
    ASSERT_TRUE(ss.at(2)._called);
}
