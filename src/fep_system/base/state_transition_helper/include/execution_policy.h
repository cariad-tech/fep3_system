
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once
#include <boost/asio.hpp>

#include "execution_timer.h"

#include <atomic>
#include <vector>

namespace fep3 {

struct SerialExecutionPolicy {

    SerialExecutionPolicy(ExecutionTimer timer = ExecutionTimer{}) : _timer(std::move(timer))
    {
    }


    SerialExecutionPolicy(SerialExecutionPolicy&&) = default;
    template <typename T>
    auto operator()(std::vector<T>& funcs) -> bool
    {
        _timer.start();
        for (auto& func: funcs) {
            auto ret = func();
            if (!ret)
                return ret;
        }
        _timer.stop();
        return true;
    }
    ExecutionTimer _timer;
};

struct ParallelExecutionPolicy {
    ParallelExecutionPolicy(uint8_t thread_count, ExecutionTimer timer = ExecutionTimer{})
        : _thread_count(thread_count), _timer(std::move(timer))
    {
    }

    template <typename T>
    auto operator()(std::vector<T>& funcs) -> bool
    {
        boost::asio::thread_pool pool(_thread_count);
        bool success = true;
        std::mutex flag_mutex;
        _timer.start();
        for (auto& func: funcs) {
            boost::asio::post(pool, [&]() {
                // our guarantee is we stop as soon as one function failed
                // but cannot stop already triggered functions
                if (success) {
                    bool result = func();
                    {
                        std::scoped_lock<std::mutex> lock(flag_mutex);
                        success = success && result;
                    }
                }
            });
        }
        pool.join();
        _timer.stop();
        return success;
    }

private:
    uint8_t _thread_count;
    ExecutionTimer _timer;
};

} // namespace fep3
