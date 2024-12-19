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

/// @brief This fixture uses the "real" execution policy and error handling
/// with the serial execution policy
struct ExecutionWithErrorPolicy : ::testing::Test {
    auto getSytemStateTransition()
    {
        return SystemStateTransition([](const FakeProxy& f) -> int32_t { return f.getPriority(); },
                                     SystemStateTransitionPrioSorting::decreasing,
                                     ThrowOnErrorPolicy{},
                                     SerialExecutionPolicy{});
    }

    std::vector<FakeProxy> _dummy_proxies{
        FakeProxy{1}, FakeProxy{1}, FakeProxy{5}, FakeProxy{5}, FakeProxy{2}, FakeProxy{2}};
    StateInfo _state_info = {"loaded"};
};

TEST_F(ExecutionWithErrorPolicy, StateTransition__execute__successful)
{
    auto state_transition = getSytemStateTransition();
    auto transition_function = [](FakeProxy&) -> ExecResult { return {true}; };

    state_transition.execute(_dummy_proxies, transition_function, _state_info);
}

TEST_F(ExecutionWithErrorPolicy, StateTransition__execute__throwsOnTransitionError)
{
    auto state_transition = getSytemStateTransition();
    int execution_count = 0;
    auto transition_function = [&](FakeProxy& d) {
        if (++execution_count == 3)
            return ExecResult{false, "Some Error"};

        else {
            d.transition();
        }
        return ExecResult{true};

    };

    ASSERT_THROW(state_transition.execute(_dummy_proxies, transition_function, _state_info),
                 std::runtime_error);
    // Participants with prio < 5 had transition function not called
    ASSERT_FALSE(_dummy_proxies.at(0)._called);
    ASSERT_FALSE(_dummy_proxies.at(1)._called);
    ASSERT_TRUE(_dummy_proxies.at(2)._called);
    ASSERT_TRUE(_dummy_proxies.at(3)._called);
    ASSERT_FALSE(_dummy_proxies.at(4)._called);
    ASSERT_FALSE(_dummy_proxies.at(5)._called);
}

TEST_F(ExecutionWithErrorPolicy, StateTransition__execute__throwOnTransitionException)
{
    auto state_transition = getSytemStateTransition();
    int execution_count = 0;
    auto transition_function = [&](FakeProxy& d) {
        if (++execution_count == 3)
            throw std::runtime_error("Some exception");
        else
            d.transition();
        return ExecResult{true};
    };

    ASSERT_THROW(state_transition.execute(_dummy_proxies, transition_function, _state_info),
                 std::runtime_error);
    // Participants with prio < 5 had transition function not called
    ASSERT_FALSE(_dummy_proxies.at(0)._called);
    ASSERT_FALSE(_dummy_proxies.at(1)._called);
    ASSERT_TRUE(_dummy_proxies.at(2)._called);
    ASSERT_TRUE(_dummy_proxies.at(3)._called);
    ASSERT_FALSE(_dummy_proxies.at(4)._called);
    ASSERT_FALSE(_dummy_proxies.at(5)._called);
}

/// @brief Test fixture that tests the behavior of ThrowOnErrorPolicy
struct TestThrowOnErrorPolicy : ::testing::Test {
    TransitionFunctionMetaData getDummyMetaData(bool res,
                                                const std::string& participant_name = "FakeProxy",
                                                const std::string& error_desc = "")
    {
        return TransitionFunctionMetaData{
            nullptr, participant_name, {"TransitionName"}, ExecResult{res, error_desc}};
    }
};

TEST_F(TestThrowOnErrorPolicy, ThrowOnErrorPolicy__operator__noThrow)
{
    ThrowOnErrorPolicy policy;
    std::vector<TransitionFunctionMetaData> transition_meta_data = {
        getDummyMetaData(true),
        getDummyMetaData(true),
        getDummyMetaData(true),
        getDummyMetaData(true)};
    ASSERT_NO_THROW(ASSERT_TRUE(policy(transition_meta_data)));
}

TEST_F(TestThrowOnErrorPolicy, ThrowOnErrorPolicy__operator__throwOnError)
{
    ThrowOnErrorPolicy policy;
    std::vector<TransitionFunctionMetaData> transition_meta_data = {
        getDummyMetaData(true),
        getDummyMetaData(true),
        getDummyMetaData(false, "Fake Proxy", "Some Error1"),
        getDummyMetaData(false, "Fake Proxy", "Some Error2")};
    try {
        policy(transition_meta_data);
    }
    catch (std::exception& e) {
        using namespace testing;
        ASSERT_THAT(e.what(), AllOf(HasSubstr("Some Error1"), HasSubstr("Some Error2")));
        return;
    }
    FAIL() << "Function expected to throw";
}
