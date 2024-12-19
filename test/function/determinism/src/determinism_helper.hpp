/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#pragma once

#include <fep3/core/element_configurable.h>
#include <fep3/cpp/datajob.h>
#include <fep3/cpp/element_base.h>
#include <fep3/components/job_registry/job_configuration.h>
#include <fep3/fep3_participant_version.h>
#include <fep3/participant/element_factory_intf.h>

#include <fstream>

namespace fep3
{
namespace test
{

template<typename data_job_type>
class DataJobElementFactoryWithComponents : public base::IElementFactory
{
public:
    std::unique_ptr<base::IElement> createElement(const fep3::IComponents& components) const override
    {
        return std::make_unique<cpp::DataJobElement<data_job_type>>(std::make_shared<data_job_type>(components));
    }
};

using Configuration = base::Configuration;
template<typename T>
using PropertyVariable = base::arya::PropertyVariable<T>;

/**
 * Logging class that writes the log verbatim into the file. (As opposed to the logging service that does some formatting and adds more information)
 */
class TestFileLogger
{
public:
    ~TestFileLogger()
    {
        _log_file.close();
    }

    bool isReady()
    {
        return _log_file.is_open();
    }

    void openLogFile(const std::string& log_file_path)
    {
        _log_file.open(log_file_path, std::fstream::out);
        if (_log_file.fail())
        {
            throw std::runtime_error(std::string("Unable to open log file"));
        }
    }

    void logToFile(std::string message)
    {
        if (static_cast<bool>(_log_file))
        {
            _log_file << message << std::endl;
            if (_log_file.fail())
            {
                throw std::runtime_error(std::string("Failed to write to log file"));
            }

            _log_file.flush();
        }
    }

private:
    std::ofstream _log_file{};
};

} // namespace test
} // namespace fep3
