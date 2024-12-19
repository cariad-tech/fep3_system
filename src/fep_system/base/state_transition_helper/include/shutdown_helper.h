
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "system_logger_helper.h"
#include <fep3/fep3_result_decl.h>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <fep_system/system_logger_intf.h>
#include <functional>
#include <type_traits>
#include <vector>
namespace fep3 {

template <typename ProxyType>
std::string getShutDownInfo(std::vector<ProxyType>& participants,
                            std::vector<std::reference_wrapper<ProxyType>>& participant_refs)
{
    std::string what_was_shut_down;
    if (participant_refs.size() == participants.size()) {
        what_was_shut_down = "System ";
    }
    else {
        what_was_shut_down.append("Participant(s): ");
        for (const auto& participant_ref: participant_refs) {
            what_was_shut_down.append(participant_ref.get().getName());
            what_was_shut_down.append(",");
        }
        what_was_shut_down.pop_back();
    }

    return what_was_shut_down;
}

void logShutDownInfo(const std::vector<Result>& errors,
                     const std::string& shutdown_participant_or_system,
                     std::shared_ptr<ISystemLogger> system_logger)
{
    if (!errors.empty()) {
        std::string error_message;
        FEP3_SYSTEM_LOG(system_logger, LoggerSeverity::fatal, "Shutdown failed");
        for (const auto& error: errors) {
            error_message.append(error.getDescription());
        }
        throw std::runtime_error("Shutdown failed: " + error_message);
    }
    else {
        FEP3_SYSTEM_LOG(system_logger,
                        LoggerSeverity::info,
                        shutdown_participant_or_system + " shutdown successful");
    }
}

template <typename ProxyType, typename Invocable>
void shutdownParticipants(std::vector<ProxyType>& participants,
                          std::vector<std::reference_wrapper<ProxyType>>& participant_refs,
                          Invocable& shutdown_function,
                          std::shared_ptr<ISystemLogger> system_logger)
{
    std::vector<Result> errors;
    std::vector<std::string> participants_shut_down;
    std::string what_was_shut_down = getShutDownInfo(participants, participant_refs);

    for (auto& participant_ref: participant_refs) {
        auto res = std::invoke(shutdown_function, participant_ref);
        const std::string participant_name = participant_ref.get().getName();
        if (!res) {
            errors.push_back(res);
            FEP3_SYSTEM_LOG(system_logger,
                            LoggerSeverity::error,
                            "Participant " + participant_name +
                                " was not shut down successfully, and will not be removed from "
                                "system, error :" +
                                res.getDescription());
        }
        else {
            FEP3_SYSTEM_LOG(system_logger,
                            LoggerSeverity::info,
                            "Participant " + participant_name +
                                " was shut down successfully and will be removed from system");
            participants_shut_down.push_back(participant_name);
        }
    }

    auto participant_shut_down = [&participants_shut_down](const std::string& participant_name) {
        return std::find(participants_shut_down.begin(),
                         participants_shut_down.end(),
                         participant_name) != participants_shut_down.end();
    };

    boost::range::remove_erase_if(participants,
                                  [&participant_shut_down](const auto& participant_proxy) -> bool {
                                      return participant_shut_down(participant_proxy.getName());
                                  });

    logShutDownInfo(errors, what_was_shut_down, system_logger);
}

} // namespace fep3
