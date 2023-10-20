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

#include "transmitter_base.h"

TransmitJobBase::TransmitJobBase(const fep3::IComponents& components, const std::string& logger_name) :
    DataJob("transmit", std::chrono::seconds(1))
{
    if(!initLogger(components, logger_name))
    {
        throw std::runtime_error("failed to initialize logger");
    }
    registerPropertyVariable(_verbose, "verbose");
    registerPropertyVariable(_log_file_path_property_variable, "log_file_path");
    registerPropertyVariable(_number_of_transmissions_property_variable, "number_of_transmissions");
    registerPropertyVariable(_wait_real_time_ns_property_variable, "wait_real_time_ns");
}

// The destructor is only declared to make the class abstract, but we still need an implementation.
TransmitJobBase::~TransmitJobBase()
{
}

fep3::Result TransmitJobBase::process(fep3::Timestamp time)
{
    if(_number_of_transmissions_done < _number_of_transmissions_property_variable)
    {
        if (_verbose)
        {
            FEP3_LOG_INFO("Waiting to transmit data for " + _wait_real_time_ns_property_variable.toString() + "ns");
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(_wait_real_time_ns_property_variable));

        if(_verbose)
        {
            FEP3_LOG_INFO("Writing sample @ " + std::to_string(time.count()) + "ns: counter = " + std::to_string(_sample_counter) + " value = " + std::to_string(_current_data_value));
        }

        if (_data_writer)
        {
            *_data_writer << _current_data_value;
        }

        _data_logger.logToFile
            (std::to_string(_sample_counter)
            + " @ " + std::to_string(time.count()) + "ns: "
            + std::to_string(_current_data_value)
            );

        // modify the data
        if(_current_data_value < std::numeric_limits<uint32_t>::max())
        {
            _current_data_value += 3;
        }
        else
        {
            _current_data_value = 0;
        }
        _sample_counter++;
        _number_of_transmissions_done++;
    }
    else
    {
        if(_verbose)
        {
            FEP3_LOG_INFO("No further transmission performed because number of transmissions has been reached.");
        }
        if (!_finished)
        {
            _finished = true;
            // notify tester about finish
            FEP3_LOG_INFO("Participant finished work");
        }
    }

    return {};
}

fep3::Result TransmitJobBase::reset()
{
    _current_data_value = 0;
    _sample_counter = 1;

    updatePropertyVariables();
    if (!_data_logger.isReady())
    {
        _data_logger.openLogFile(_log_file_path_property_variable);
    }
    return {};
}
