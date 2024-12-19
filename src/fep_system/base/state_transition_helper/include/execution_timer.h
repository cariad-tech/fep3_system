
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "deadline_timer.h"

#include <boost/asio.hpp>

#include <atomic>
#include <vector>

namespace fep3 {

struct ExecutionTimer {
    ExecutionTimer() : ExecutionTimer(std::chrono::milliseconds(0), nullptr)
    {
    }

    ExecutionTimer(std::chrono::milliseconds timeout, std::function<void()> callable)
        : _timeout(std::move(timeout)), _callable(std::move(callable)), _deadline_timer(nullptr)
    {
    }

    void start()
    {
        if (_callable) {
            _deadline_timer = std::make_unique<DeadlineTimer>();
            _deadline_timer->expireAt(_timeout, _callable);
        }
    }
    void stop()
    {
        if (_deadline_timer)
            _deadline_timer->cancel();
    }

    ~ExecutionTimer()
    {
        stop();
    }
    ExecutionTimer(ExecutionTimer&&) = default;

private:
    std::chrono::milliseconds _timeout;
    std::function<void()> _callable;
    std::unique_ptr<DeadlineTimer> _deadline_timer;
};
} // namespace fep3
