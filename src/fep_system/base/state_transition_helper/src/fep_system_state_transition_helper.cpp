
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "fep_system_state_transition_helper.h"
#include <a_util/strings/strings_format.h>

#include <assert.h>

namespace fep3 {
std::function<bool(int32_t, int32_t)> getSortFunction(SystemStateTransitionPrioSorting sorting)
{
    switch (sorting) {
    case SystemStateTransitionPrioSorting::decreasing:
        return std::greater<int32_t>();
    case SystemStateTransitionPrioSorting::increasing:
        return std::less<int32_t>();
    case SystemStateTransitionPrioSorting::none:
        return nullptr;
    }
    // so vs compiler is happy and not give out a warning
    return nullptr;
}

bool TransitionFunctionMetaData::operator()()
{
    // add try catch for safety
    try {
        _exec_result = _state_transition_function();
    }
    catch (std::exception& e) {
        _exec_result._result = false;
        _exec_result._error_description =
            a_util::strings::format("Participant %s threw exception: %s, and could not be %s'",
                                    _participant_name.c_str(),
                                    e.what(),
                                    _state_info.state_transition.c_str());
    }
    return _exec_result._result;
}

bool TransitionFunctionMetaData::hasError() const
{
    return (!_exec_result._result || !_exec_result._error_description.empty());
}

} // namespace fep3
