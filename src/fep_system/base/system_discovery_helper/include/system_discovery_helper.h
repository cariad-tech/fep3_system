/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2022 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */

#pragma once
#include "service_bus_intf.h"
#include <vector>
#include <string>
#include <optional>

namespace fep3
{
    using DiscoveredParticipants = std::multimap<std::string, std::string>;
    using DiscoveredParticipantsWithError = std::pair<std::string, std::optional<DiscoveredParticipants>>;

    [[nodiscard]] DiscoveredParticipantsWithError discoverSystemParticipants(fep3::IServiceBus::ISystemAccess&  system_access, std::chrono::milliseconds timeout);

    [[nodiscard]] DiscoveredParticipantsWithError discoverSystemParticipants(fep3::IServiceBus::ISystemAccess& system_access,
        std::chrono::milliseconds timeout,
        std::vector<std::string> participant_names,
        bool discovered_all_systems = false);

    [[nodiscard]] DiscoveredParticipantsWithError discoverSystemParticipants(fep3::IServiceBus::ISystemAccess& system_access,
        std::chrono::milliseconds timeout,
        const uint32_t participant_size);

    template<typename Predicate, typename  ...PredicateArgs>
    [[nodiscard]] DiscoveredParticipantsWithError discoverSystemParticipantsHelper(fep3::IServiceBus::ISystemAccess& system_access,
        std::chrono::milliseconds timeout,
        Predicate predicate, PredicateArgs... predicate_args);

    struct ParticipantAndSystemName
    {
        std::string _participant_name;
        std::string _system_name;
    };

    std::optional<ParticipantAndSystemName> get_partictipant_and_system_name(const std::string& part_system_names_combined);
  
}
