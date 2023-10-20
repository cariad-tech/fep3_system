#include "system_discovery_helper.h"
#include <boost/range/adaptors.hpp>
#include <algorithm>
#include <sstream>
#include <boost/range/algorithm.hpp>
#include <a_util/strings.h>

namespace
{
    using namespace std::literals::chrono_literals;
    constexpr const std::chrono::milliseconds default_discover_poll_period = 1000ms;
    /*
    *  auto discovered_participant_names =  discovered_participants | boost::adaptors::map_values;
       auto tt = discovered_participant_names.
    */
    std::pair<bool, std::string> all_participants_discovered_by_count(fep3::DiscoveredParticipants& discovered_participants, uint32_t participant_size)
    {
        bool success = discovered_participants.size() >= participant_size;
        std::string error_message = "";
        if (!success)
        {
            using namespace std::literals::string_literals;
            error_message = "Expected to discover " + std::to_string(participant_size) + " participants, actually discovered " +
                std::to_string(discovered_participants.size());
        }

        return {success,  error_message};
    }

    std::pair<bool, std::string> get_participant_names_from(std::vector<std::string>& participant_and_system_names)
    {
        bool success = true;
        std::stringstream ss;
        std::transform(participant_and_system_names.begin(), participant_and_system_names.end(), participant_and_system_names.begin(),
            [&](const std::string& s)
            {
                const auto opt_part_and_sys_name = fep3::get_partictipant_and_system_name(s);
                if (opt_part_and_sys_name.has_value())
                {
                    const auto [participant_name, _] = opt_part_and_sys_name.value();
                    return participant_name;
                }
                else
                {
                    success = false;
                    ss << "Cannot extract participant and system name from " + s;
                    return std::string{};
                }
            });

        return { success , ss.str()};
    }

    std::pair<bool,std::string> all_participants_discovered_by_name(fep3::DiscoveredParticipants& discovered_participants,
        const std::vector<std::string>&  expected_participant_names,
        bool discovered_all_systems)
    {
        auto discovered_participant_range = discovered_participants | boost::adaptors::map_keys;
        // we have to copy because we cannot sort the range of a map
        std::vector<std::string> discovered_participant_names(discovered_participants.size());
        boost::copy(discovered_participant_range, discovered_participant_names.begin());

        if (discovered_all_systems)
        {
           const auto [success, error_message] = get_participant_names_from(discovered_participant_names);
           if (!success)
           {
               return { success, error_message };
           }
        }

        boost::range::sort(discovered_participant_names);

        bool success = boost::range::includes(discovered_participant_names, expected_participant_names);

        if (success)
        {
            return { success,  "" };
        }
        else
        {
            std::stringstream ss;
            ss << "Expected to discover participants: ";
            std::copy(expected_participant_names.begin(), expected_participant_names.end(), std::ostream_iterator<std::string>(ss, " "));
            ss << ",actually discovered participants: ";
            std::copy(discovered_participant_names.begin(), discovered_participant_names.end(), std::ostream_iterator<std::string>(ss, " "));

            return { success,  ss.str() };
        }
    }
}

namespace fep3
{
    template<typename Predicate, typename  ...PredicateArgs>
    DiscoveredParticipantsWithError discoverSystemParticipantsHelper(fep3::IServiceBus::ISystemAccess& system_access,
        std::chrono::milliseconds timeout,
        Predicate predicate, PredicateArgs... predicate_args)
{
    auto discover_count = timeout / default_discover_poll_period;
    auto last_discover_duration = timeout % default_discover_poll_period;
    auto discover_duration = default_discover_poll_period;
    while (discover_count >= 0)
    {
        if (discover_count == 0)
        {
            discover_duration = last_discover_duration;
        }

        auto discovered_participants = system_access.discover(discover_duration);
        auto [success, error] = predicate(discovered_participants, std::forward<PredicateArgs>(predicate_args)...);
        if ((success) || (discover_count == 0))
        {
            if (success)
            {
                return { "", discovered_participants };
            }
            else
            {
                return { error, {} };
            }
        }
        --discover_count;
    }
    assert(0  && "code should not reach this point");
    return {};
}

DiscoveredParticipantsWithError discoverSystemParticipants(fep3::IServiceBus::ISystemAccess& system_access, std::chrono::milliseconds timeout)
{
        return { "", system_access.discover(timeout) };
}

DiscoveredParticipantsWithError discoverSystemParticipants(fep3::IServiceBus::ISystemAccess& system_access,
    std::chrono::milliseconds timeout,
     std::vector<std::string> participant_names,
    bool discovered_all_systems)
{
     boost::range::sort(participant_names);
     return discoverSystemParticipantsHelper(system_access, timeout,
        all_participants_discovered_by_name,
         participant_names, discovered_all_systems);
}

DiscoveredParticipantsWithError discoverSystemParticipants(fep3::IServiceBus::ISystemAccess& system_access,
    std::chrono::milliseconds timeout,
    const uint32_t participant_size)
{
    return discoverSystemParticipantsHelper(system_access, timeout, all_participants_discovered_by_count, participant_size);
}

std::optional<ParticipantAndSystemName> get_partictipant_and_system_name(const std::string& part_system_names_combined)
{
    auto splitted_id = a_util::strings::split(part_system_names_combined, "@", true);
    if (splitted_id.size() > 1)
    {
        const std::string& system_name = splitted_id[1];
        const std::string& participant_name = splitted_id[0];

        return ParticipantAndSystemName{ participant_name , system_name };
    }
    else
    {
        return {};
    }
}

}
