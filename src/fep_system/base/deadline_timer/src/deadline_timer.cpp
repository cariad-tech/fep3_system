/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "deadline_timer.h"
#include "boost_asio_timer.h"

namespace fep3 {

DeadlineTimer::DeadlineTimer() : _timer(std::make_unique<BoostAsioTimer>())
{
}

DeadlineTimer::~DeadlineTimer()
{
    _timer.reset();
}

void DeadlineTimer::expireAt(std::chrono::milliseconds timeout, std::function<void()> callback)
{
    _timer->expireAt(timeout, std::move(callback));
}

void DeadlineTimer::cancel()
{
    _timer->cancel();
}

} // namespace fep3
