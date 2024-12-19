/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <boost/current_function.hpp>

#include <string>
#include <stdexcept>

/**
* @brief Helper macro to log a message to a system logger
* @param[in] given_logger logger pointer
* @param[in] given_sev Level
* @param[in] log_message the message that will be forwarded
*/
#define FEP3_SYSTEM_LOG(given_logger, given_sev, log_message) \
do \
{ \
    if(given_logger) { \
        given_logger->log(\
            given_sev, \
            std::string() + log_message + "; logged in " \
            + std::string(BOOST_CURRENT_FUNCTION) + " - " \
            + std::string(__FILE__) + " - line: " \
            + std::to_string(__LINE__)); \
    } \
} while (false)

/**
* @brief Helper macro to log to a system logger a message and throw a runtime_error
* @param[in] given_logger logger pointer
* @param[in] given_sev Level
* @param[in] log_message the message that will be forwarded
*/
#define FEP3_SYSTEM_LOG_AND_THROW(given_logger, given_sev, log_message) \
do \
{ \
    FEP3_SYSTEM_LOG(given_logger, given_sev, log_message); \
    throw std::runtime_error(log_message); \
} while (false)
