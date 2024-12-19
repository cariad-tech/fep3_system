/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
 #include "system_logger.h"

namespace fep3
{
    std::map<std::string, std::unique_ptr<GlobalLoggingSinkClientService::LoggingSinkClientServiceWrapper>> GlobalLoggingSinkClientService::_servers;
}
