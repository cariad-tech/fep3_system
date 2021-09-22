/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2021 VW Group. All rights reserved.

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

#include <fep3/core/element_configurable.h>
#include <fep3/cpp/datajob.h>
#include <fep3/components/job_registry/job_configuration.h>
#include <fep3/fep3_participant_version.h>
#include <fep3/participant/element_factory_intf.h>

#include <a_util/filesystem.h>
#include "clock/local_system_clock_discrete.h"

#include <fstream>

namespace fep3
{
namespace test
{
using Configuration = base::arya::Configuration;
template<typename T>
using PropertyVariable = base::arya::PropertyVariable<T>;

/**
 * @brief This Element Type is a copy of cpp::DataJobElement. It adds a job configuration parameter to the constructor
 *        and a max runtime property for a custom discrete clock that stops after a given simulation time passed
 *
 * @tparam data_job_type Type of the DataJob to add to the Element
 */
template<typename data_job_type>
class DataJobElement : public core::arya::ElementConfigurable
{

public:
    /// CTOR
    DataJobElement(fep3::arya::JobConfiguration job_config)
        : core::arya::ElementConfigurable("fep3::cpp::DataJobElement",
            FEP3_PARTICIPANT_LIBRARY_VERSION_STR),
        _job(std::make_shared<data_job_type>(job_config)),
        _need_reset(true)
    {
        registerPropertyVariable(_max_runtime, "max_runtime");
    }

    fep3::Result load() override
    {
        auto config_service = getComponents()->getComponent<fep3::arya::IConfigurationService>();
        if (config_service && _job)
        {
            FEP3_RETURN_IF_FAILED(_job->initConfiguration(*config_service));
        }

        /*
        auto clock_service = getComponents()->getComponent<fep3::arya::IClockService>();
        auto service_bus = getComponents()->getComponent<fep3::arya::IServiceBus>();
        if (clock_service && service_bus)
        {
            _clock = std::make_shared<test::LocalSystemSimClockWithTimeout>(service_bus);
            FEP3_RETURN_IF_FAILED(clock_service->registerClock(_clock));
        }
        */
        return {};
    }

    void unload() override
    {
        if (_job)
        {
            _job->deinitConfiguration();
        }

        /*
        auto clock_service = getComponents()->getComponent<fep3::arya::IClockService>();
        if (clock_service && _clock)
        {
            clock_service->unregisterClock(_clock->getName());
            _clock.reset();
        }
        */
    }

    void stop() override
    {
        if (_job)
        {
            _need_reset = true;
        }
    }

    fep3::Result run() override
    {
        if (_job && _need_reset)
        {
            _need_reset = false;
            return _job->reset();
        }
        return {};
    }

    fep3::Result initialize() override
    {
        if (_job)
        {
            const auto components = getComponents();
            if (components)
            {
                FEP3_RETURN_IF_FAILED(cpp::arya::addToComponents({ _job }, *components));
            }
            else
            {
                RETURN_ERROR_DESCRIPTION(ERR_INVALID_ADDRESS, "components reference invalid");
            }
        }
        else
        {
            RETURN_ERROR_DESCRIPTION(ERR_INVALID_ADDRESS, "job was not initialized in element");
        }

        /*
        if (_clock)
        {
            updatePropertyVariables();
            _clock->setMaxRuntime(std::chrono::milliseconds(_max_runtime));
        }
        */

        return{};
    }

    void deinitialize() override
    {
        if (_job)
        {
            cpp::arya::removeFromComponents({ _job }, *getComponents());
        }
    }
private:
    /// data job
    std::shared_ptr<cpp::arya::DataJob> _job;
    //std::shared_ptr<LocalSystemSimClockWithTimeout> _clock;
    bool _need_reset;

    PropertyVariable<int32_t> _max_runtime;
};

/**
 * Element factory for the scenario::DataJobElement
 * @tparam element_type the element_type to create. Derive from fep3::core::ElementBase and requires a constructor with a JobConfiguration as parameter.
 */
template <typename element_type>
class DataJobElementFactory : public fep3::arya::IElementFactory
{
public:
    /**
     * CTOR for the Element factory which is able to create elements with a specified job configuration
     *
     * @returns Shared pointer to the created element.
     */
    DataJobElementFactory(fep3::arya::JobConfiguration job_config)
        : _job_config(job_config)
    {
    }
    /**
     * Creates the element
     *
     * @returns unique pointer to the created element.
     */
    std::unique_ptr<fep3::arya::IElement> createElement(const fep3::arya::IComponents& /*components*/) const override
    {
        return std::unique_ptr<fep3::arya::IElement>(new element_type(_job_config));
    }
private:
    fep3::arya::JobConfiguration _job_config;
};

/**
 * Logging class that writes the log verbatim into the file. (As opposed to the logging service that does some formatting and adds more information)
 */
class TestFileLogger
{
public:
    explicit TestFileLogger(Configuration& configuration) : _configuration(configuration)
    {
        _configuration.registerPropertyVariable(_log_file_variable, "logfile");
    }

    ~TestFileLogger()
    {
        _log_file.close();
    }

    bool isReady()
    {
        return _log_file.is_open();
    }

    void openLogFile()
    {
        _configuration.updatePropertyVariables();
        std::string logfile = static_cast<std::string>(_log_file_variable);
        a_util::filesystem::Path path{ logfile };

        _log_file.open(path.toString().c_str(), std::fstream::out);

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
    Configuration& _configuration;
    std::ofstream _log_file{};
    fep3::cpp::PropertyVariable<std::string> _log_file_variable{ "./logfile.txt" };
};

} // namespace test
} // namespace fep3