/**
 * @file
 * @copyright
 * @verbatim
Copyright 2023 CARIAD SE. 

This Source Code Form is subject to the terms of the Mozilla
Public License, v. 2.0. If a copy of the MPL was not distributed
with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
@endverbatim
 */


#pragma once
#include <string>
#include "base/logging/logging_types.h"
#include "logging_types_legacy.h"
#include "rpc_services/participant_statemachine/participant_statemachine_rpc_intf.h"

namespace fep3
{
/// Virtual class to implement FEP System Event Monitor, used with
/// @ref fep3::System::registerMonitoring
class IEventMonitor
{
public:
    /// DTOR
    virtual ~IEventMonitor() = default;

    /**
     * @brief Callback on log
     *
     * You will only retrieve log messages above the given fep::System::setSystemSeverityLevel
     *
     * @param[in] log_time time of the log
     * @param[in] severity_level severity level of the log
     * @param[in] system_name_or_participant_name name of the logging participant or system
     * @param[in] logger_name name of the logging logger (usually the participant, component or element name and category, "system_logger" for system logs)
     * @param[in] message detailed message
     */
    virtual void onLog(std::chrono::milliseconds log_time,
        LoggerSeverity severity_level,
        const std::string& system_name_or_participant_name,
        const std::string& logger_name, //depends on the Category ...
        const std::string& message) = 0;
};


/**
 * this is a namespace for porting convenience
 * this will be removed in further releases
 */
namespace legacy
{
/// Virtual class to implement FEP System Event Monitor, used with
/// @ref fep3::System::registerMonitoring
class EventMonitor : public IEventMonitor
{
public:
    /**
     * legacy callback by state change. has no effect and is not supported anymore.
     *
     */
    virtual void onStateChanged(const std::string&,
        fep3::rpc::IRPCParticipantStateMachine::State)
    {
        //this is not supported in fep3 beta
        return;
    }

    /**
     * legacy callback by name change. has no effect and is not supported anymore.
     *
     */
    virtual void onNameChanged(const std::string&, const std::string&)
    {
        //this is not supported anymore in fep3
        return;
    }
    /**
     * @brief legacy callback on log at system, participant, component or element category.
     * This function will be removed in further releases. category was removed.
     *
     * You will only retrieve log messages above the given fep::System::setSystemSeverityLevel
     *
     * @param[in] log_time time of the log
     * @param[in] category category of the log
     * @param[in] severity_level severity level of the log
     * @param[in] participant_name participant name (is empty on system category)
     * @param[in] logger_name (usually the system, participant, component or element name)
     * @param[in] message detailed message
     */
    virtual void onLog(std::chrono::milliseconds log_time,
        Category category,
        LoggerSeverity severity_level,
        const std::string& participant_name,
        const std::string& logger_name, //depends on the Category ...
        const std::string& message) = 0;

    /**
     * @copydoc fep3::IEventMonitor::onLog
     */
    void onLog(std::chrono::milliseconds log_time,
        LoggerSeverity severity_level,
        const std::string& system_name_or_participant_name,
        const std::string& logger_name, //depends on the Category ...
        const std::string& message) override
    {
        legacy::Category category = legacy::Category::CATEGORY_NONE;
        if (system_name_or_participant_name.empty())
        {
            category = legacy::Category::CATEGORY_SYSTEM;
        }
        else
        {
            if (logger_name.find(".component") != std::string::npos)
            {
                category = legacy::Category::CATEGORY_COMPONENT;
            }
            else if (logger_name.find(".element") != std::string::npos)
            {
                category = legacy::Category::CATEGORY_ELEMENT;
            }
            else
            {
                category = legacy::Category::CATEGORY_PARTICIPANT;
            }
        }
        onLog(log_time,
              category,
              severity_level,
              system_name_or_participant_name,
              logger_name,
              message);
    }
};
}
}
