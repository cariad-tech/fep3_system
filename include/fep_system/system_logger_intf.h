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
#include "base/logging/logging_types.h"
#include <string>
#include <chrono>


namespace fep3
{
    /**
     * @brief The system logger is an internal interface used within the ParticipantProxy
     * It is the connection to the IEventMonitor set within the fep3::System class.
     * Each logging message within the ParticipantProxy which is part of a fep3::System will be forwarded to this
     * interface.
     *
     */
    class ISystemLogger
    {
        protected:
            /**
             * @brief default DTOR
             *
             */
            virtual ~ISystemLogger() = default;
        public:
            /**
             * @brief logs a log message to the system logger.
             * time value is taken from now
             *
             * @param[in] severity logger severity
             * @param[in] message the message that will be forwarded
             */
            virtual void log(
                LoggerSeverity severity,
                const std::string& message) const = 0;

            /**
             * @brief logs a proxy error message.
             * time value is taken from now
             *
             * @param[in] severity logger severity
             * @param[in] participant_name the name of the participant
             * @param[in] component the component creating the message
             * @param[in] message the message that will be forwarded
             */
            virtual void logProxyError(
                LoggerSeverity severity,
                const std::string& participant_name,
                const std::string& component,
                const std::string& message) const = 0;

    };

    /**
     * @brief The participant logger is an internal interface used to forward the logs coming
     * from remote rpc participants.
     *
     */
    class IRemoteLogForwarder {
    protected:
        /**
         * @brief default DTOR
         *
         */
        virtual ~IRemoteLogForwarder() = default;

    public:
        /**
         * @brief logs a log message to the system logger.
         *
         * @param[in] timestamp time value
         * @param[in] level Level
         * @param[in] participant_name The name of the participant
         * @param[in] logger_name the name of the logger
         * @param[in] message the message that will be forwarded
         */
        virtual void log(const std::chrono::milliseconds& timestamp,
                         LoggerSeverity level,
                         const std::string& participant_name,
                         const std::string& logger_name, // depends on the Category ...
                         const std::string& message) const = 0;

        /**
         * @brief gets a valid url for registering the system instance to the participants logging
         * service.
         *
         * @return returns a valid url for registering the system instance to the participants
         * logging service.
         */
        virtual std::string getUrl() const = 0;
    };
}
