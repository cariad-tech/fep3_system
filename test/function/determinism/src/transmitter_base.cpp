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

TransmitJobBase::TransmitJobBase(fep3::arya::JobConfiguration job_config) :
    DataJob("transmit", job_config),
    _logger(*this)
{
    registerPropertyVariable(_delay, "delay");
}

// The destructor is only declared to make the class abstract, but we still need an implementation.
TransmitJobBase::~TransmitJobBase()
{
}

fep3::Result TransmitJobBase::process(fep3::Timestamp time)
{
    if (_data_writer)
    {
        *_data_writer << _values[_selector];
    }

    auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    _logger.logToFile(std::to_string((time_ms - _start_time_ms).count()) + " "
        + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(time).count()) + " "
        + std::to_string(_values[_selector]));

    _selector = ++_selector % _values.size();

    if (static_cast<int32_t>(_delay))
    {
        // Deliberately delay process to simulate high load or bus delay
        _counter = ++_counter % 10;
        if (_counter == 0)
        {
            updatePropertyVariables();
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int32_t>(_delay)));
        }
    }

    return {};
}

fep3::Result TransmitJobBase::reset()
{
    if (!_logger.isReady())
    {
        _logger.openLogFile();
        _start_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    }
    return {};
}