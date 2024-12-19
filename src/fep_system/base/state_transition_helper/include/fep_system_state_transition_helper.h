
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <functional>
#include <string>

namespace fep3 {
constexpr auto noPrio = [](const auto&) constexpr
{
    return 1;
};

enum class SystemStateTransitionPrioSorting
{
    decreasing,
    increasing,
    none
};

struct StateInfo {
    std::string state_transition;
};

 struct ExecResult {
    bool _result{false};
    std::string _error_description{};
};

struct TransitionFunctionMetaData {
    std::function<ExecResult()> _state_transition_function;
    const std::string _participant_name;
    const StateInfo _state_info;
    ExecResult _exec_result;
    bool operator()();
    bool hasError() const;
};

std::function<bool(int32_t, int32_t)> getSortFunction(SystemStateTransitionPrioSorting sorting);
} // namespace fep3
