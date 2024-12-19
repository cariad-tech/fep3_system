/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "execution_policy.h"

#include <boost/thread/latch.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace fep3;

struct ExecutionPolicy : ::testing::Test {
    struct AlwaysTrueCallable {
        bool operator()()
        {
            _called = true;
            return true;
        }
        bool _called = false;
    };

    struct FailsOnThirdCallable {
        bool operator()()
        {
            _called = true;
            return !(++_call_count == 3);
        }
        bool _called = false;
        static std::atomic<int> _call_count;
    };

    void SetUp()
    {
        ExecutionPolicy::FailsOnThirdCallable::_call_count = 0;
    }

protected:
    std::vector<AlwaysTrueCallable> _all_true_function_calls{5};
    std::vector<FailsOnThirdCallable> _fails_on_third_function_call{5};
    std::vector<FailsOnThirdCallable> _multi_thread_fails_on_third_function_call{30};
};

std::atomic<int> ExecutionPolicy::FailsOnThirdCallable::_call_count = 0;
TEST_F(ExecutionPolicy, SerialExecutionPolicy__call__successful)
{
    using namespace testing;
    SerialExecutionPolicy policy;
    ASSERT_TRUE(policy(_all_true_function_calls));
    ASSERT_THAT(_all_true_function_calls, Each(Field(&AlwaysTrueCallable::_called, true)));
}

TEST_F(ExecutionPolicy, SerialExecutionPolicy__operator__stopsOnError)
{
    using namespace testing;
    SerialExecutionPolicy policy;
    ASSERT_FALSE(policy(_fails_on_third_function_call));
    ASSERT_THAT(_fails_on_third_function_call,
                ElementsAre(Field(&FailsOnThirdCallable::_called, true),
                            Field(&FailsOnThirdCallable::_called, true),
                            Field(&FailsOnThirdCallable::_called, true),
                            Field(&FailsOnThirdCallable::_called, false),
                            Field(&FailsOnThirdCallable::_called, false)));
}

TEST_F(ExecutionPolicy, ParallelExecutionPolicy__operator__successful)
{
    using namespace testing;
    ParallelExecutionPolicy policy(4);
    ASSERT_TRUE(policy(_all_true_function_calls));
    ASSERT_THAT(_all_true_function_calls, Each(Field(&AlwaysTrueCallable::_called, true)));
}

TEST_F(ExecutionPolicy, ParallelExecutionPolicy__operator__stopsOnError)
{
    using namespace testing;
    ParallelExecutionPolicy policy(4);
    ASSERT_FALSE(policy(_multi_thread_fails_on_third_function_call));
    // Parallel Policy cannot make no guarantees on what is called
}

struct Callable {
    MOCK_METHOD(void, call_operator, ());
    void operator()()
    {
        call_operator();
    }
};

struct ExecutionPolicyTimeout : ::testing::Test {
    struct BlockingCallable {
        BlockingCallable(boost::latch& latch) : _latch(latch)
        {
        }
        bool operator()()
        {
            _latch.wait();
            _called = true;
            return true;
        }
        bool _called = false;
        boost::latch& _latch;
    };

    boost::latch _latch{1};
    std::vector<BlockingCallable> _blocking_callable = {BlockingCallable(_latch)};
};

TEST_F(ExecutionPolicyTimeout, parallelPolicy_timeout_callbackCalled)
{
    using namespace testing;
    using namespace std::chrono_literals;
    Callable callable;
    ParallelExecutionPolicy policy(4, ExecutionTimer{400ms, [&]() {
                                                         callable();
                                                         ASSERT_FALSE(
                                                             _blocking_callable.front()._called);
                                                         _latch.count_down();
                                                     }});

    EXPECT_CALL(callable, call_operator()).Times(1);

    ASSERT_TRUE(policy(_blocking_callable));
}

TEST_F(ExecutionPolicyTimeout, parallelPolicy_noTimeout_callbackNotCalled)
{
    using namespace testing;
    using namespace std::chrono_literals;
    Callable callable;
    ParallelExecutionPolicy policy(4, ExecutionTimer{1000000000000000000ms, [&]() { callable(); }});

    EXPECT_CALL(callable, call_operator()).Times(0);
    _latch.count_down();
    ASSERT_TRUE(policy(_blocking_callable));
}

TEST_F(ExecutionPolicyTimeout, serialPolicy_timeout_callbackCalled)
{
    using namespace testing;
    using namespace std::chrono_literals;
    Callable callable;
    SerialExecutionPolicy policy(ExecutionTimer{400ms, [&]() {
                                                    callable();
                                                    ASSERT_FALSE(
                                                        _blocking_callable.front()._called);
                                                    _latch.count_down();
                                                }});

    EXPECT_CALL(callable, call_operator()).Times(1);

    ASSERT_TRUE(policy(_blocking_callable));
}

TEST_F(ExecutionPolicyTimeout, serialPolicy_noTimeout_callbackNotCalled)
{
    using namespace testing;
    using namespace std::chrono_literals;
    Callable callable;
    SerialExecutionPolicy policy(ExecutionTimer{400ms, [&]() { callable(); }});

    EXPECT_CALL(callable, call_operator()).Times(0);
    _latch.count_down();
    ASSERT_TRUE(policy(_blocking_callable));
}
