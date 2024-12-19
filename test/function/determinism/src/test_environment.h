/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <chrono>

struct TestEnvironment : public ::testing::Environment
{
    TestEnvironment
        (bool verbose
        , uint32_t number_of_samples
        , uint32_t step_size_in_ms
        , float time_factor
        , bool waiting_transmisison
        , bool delayed_reception
        )
        : _verbose(verbose)
        , _number_of_samples(number_of_samples)
        , _step_size(step_size_in_ms)
        , _waiting_transmisison(waiting_transmisison)
        , _time_factor(time_factor)
        , _delayed_reception(delayed_reception)
    {}
    bool _verbose{ false };
    uint32_t _number_of_samples{ 0 };
    std::chrono::milliseconds _step_size{ 0 };
    float _time_factor{ 0 };
    bool _waiting_transmisison{ false };
    bool _delayed_reception{ false };
};