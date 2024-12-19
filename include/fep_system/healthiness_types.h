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

#include <chrono>
#include <vector>
#include <string>
#include <variant>
#include "fep_system_export.h"

namespace fep3
{
    /**
    * @brief Healthiness structure of a job.
    */
    struct JobHealthiness
    {
        /**
        * @brief Holds information about the clock triggered job.
        */
        struct ClockTriggeredJobInfo
        {
            /// job cycle time
            const std::chrono::nanoseconds cycle_time;
        };
        /**
         * @brief Holds information about the data triggered job.
        */
        struct DataTriggeredJobInfo
        {
            /// job trigger signals
            const std::vector<std::string> trigger_signals;
        };
        ///  name of the job
        const std::string job_name;
        /// job relevant information
        const std::variant<ClockTriggeredJobInfo, DataTriggeredJobInfo>  job_info;
        /// last simulation time that JobHealthiness was updated.
        std::chrono::nanoseconds simulation_time = std::chrono::nanoseconds(0);

        /**
         * @brief Holds information about the last result of job execution.
         */
        struct ExecuteResult
        {
            /// error code
            int error_code = 0;
            /// description of the error
            std::string error_description{""};
            /// line of code that the error occurred
            std::int32_t line = 0;
            /// file name that the error occurred
            std::string file{""};
            /// function name that the error occurred
            std::string function{""};
        };

        /**
         * @brief Holds information about the last error of job execution.
         */
        struct ExecuteError
        {
            /// number of times the job returned an non zero error code<summary>
            uint64_t error_count = 0;
            /// last simulation time that error was returned.
            std::chrono::nanoseconds simulation_time = std::chrono::nanoseconds(0);
            /// last non zero error from job execution<summary>
            ExecuteResult last_error;
        };
        /// holds last execution error of a job's executeDataIn
        ExecuteError execute_data_in_error = {};
        /// holds last execution error of a job's execute
        ExecuteError execute_error = {};
        /// holds last execution error of a job's executeDataOut
        ExecuteError execute_data_out_error = {};
    };

    /**
    * @brief Enum for describing the participants running state.
    */
    enum class ParticipantRunningState
    {
        /// participant has not sent a notify message inside a defined time interval
        offline,
        /// participant has sent a notify message inside a defined time interval
        online
    };

    /// describes the healthiness of all jobs running in a participant.
    using JobsHealthiness = std::vector<JobHealthiness>;

    /**
    * @brief Describes an update of the participant's health.
    */
    struct ParticipantHealthUpdate
    {
        /// System time in which the participant's health was updated
        std::chrono::time_point<std::chrono::steady_clock> system_time;
        /// the participant's healthiness
        JobsHealthiness jobs_healthiness;
    };

    /**
    * @brief Describes an the participant's health.
    */
    struct ParticipantHealth
    {
        /// the participants running state.
        ParticipantRunningState running_state;
        /// the participant's healthiness
        JobsHealthiness jobs_healthiness;
    };

    /**
    * @brief Converts participant running state string to enum value and throws domain error exception if unsuccessful.
    * 
    * @param[in] state_string the state as string.
    * @return ParticipantRunningState resolved enum value.
    */
    ParticipantRunningState FEP3_SYSTEM_EXPORT participantRunningStateFromString(const std::string& state_string);

    /**
    * @brief Converts participant running state enum value to string and returns "NOT RESOLVABLE" for unknown value.
    * 
    * @param[in] state the state as enum value.
    * @return std::string value in string.
    */
    std::string FEP3_SYSTEM_EXPORT participantRunningStateToString(const ParticipantRunningState state);
} //namespace fep3
