/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once
#include <atomic>
#include "fep_system_state_transition_helper.h"

struct FakeProxy {
    int32_t _prio{};
    std::string _name{};
    int32_t _called_index{};
    bool _called = false;
    int32_t getPriority() const
    {
        return _prio;
    }

    std::string getName() const
    {
        return _name;
    }

    fep3::ExecResult transition()
    {
        _called = true;
        _called_index = _counter++;
        return fep3::ExecResult{true};
    }
    static std::atomic<int32_t> _counter;
};
