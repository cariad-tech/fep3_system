/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <chrono>
#include <functional>
#include <memory>

namespace fep3 {
class BoostAsioTimer;
class DeadlineTimer {
public:
    DeadlineTimer();
    ~DeadlineTimer();

    void expireAt(std::chrono::milliseconds timeout, std::function<void()> callback);

    void cancel();

private:
    std::unique_ptr<BoostAsioTimer> _timer;
};

} // namespace fep3
