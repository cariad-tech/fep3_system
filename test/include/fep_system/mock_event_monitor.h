/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#pragma once

#include <gmock/gmock.h>

#include <fep_system/event_monitor_intf.h>

namespace fep3
{
namespace mock
{

struct EventMonitor : public fep3::IEventMonitor
{
    MOCK_METHOD(void, onLog, (std::chrono::milliseconds, LoggerSeverity, const std::string&, const std::string&, const std::string&), (override));
};

}
}
