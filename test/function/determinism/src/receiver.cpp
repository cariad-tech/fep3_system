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


#include <fep3/cpp/datajob.h>
#include <fep3/cpp/participant.h>
#include "determinism_helper.hpp"

#include <3rdparty/clara/Clara-1.1.2/single_include/clara.hpp>

using namespace fep3;
using namespace cpp;

class ReceiveJob : public DataJob
{
public:
    ReceiveJob(fep3::arya::JobConfiguration job_config) : DataJob("receive", job_config), _logger(*this)
    {
        _reader_data_one = addDataIn("data_one", fep3::base::StreamTypePlain<int32_t>(), 10);
        _reader_data_two = addDataIn("data_two", fep3::base::StreamTypePlain<int32_t>(), 10);
        registerPropertyVariable(_max_sim_time, "max_sim_time");
    }

    fep3::Result process(fep3::Timestamp time) override
    {
        if (!_logger.isReady())
        {
            _logger.openLogFile();
            _start_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        }

        _value_data_one = 0;
        _value_data_two = 0;

        *_reader_data_one >> _value_data_one;
        *_reader_data_two >> _value_data_two;

        _result = (_value_data_one - _value_data_two);

        auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        _logger.logToFile(std::to_string((time_ms - _start_time_ms).count()) + " "
            + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(time).count()) + " "
            + std::to_string(_value_data_one) + " "
            + std::to_string(_value_data_two) + " "
            + std::to_string(_result));

        if (time >= std::chrono::milliseconds(static_cast<int32_t>(_max_sim_time)))
        {
            //getNotification().notify();
        }

        return {};
    }

private:
    DataReader* _reader_data_one;
    DataReader* _reader_data_two;
    int32_t _value_data_one{};
    int32_t _value_data_two{};
    int32_t _result{};
    test::TestFileLogger _logger;
    std::chrono::milliseconds _start_time_ms;

    fep3::cpp::PropertyVariable<int32_t> _max_sim_time{ 0 };
};

int main(int argc, char* argv[])
{
    // We cannot use the createParticipant parser because we need the command line arguments already for the parameters of createParticipant.
    // So we have to parse the arguments ourselves beforehand.
    uint32_t cycle_time{FEP3_CLOCK_SIM_TIME_STEP_SIZE_DEFAULT_VALUE};
    uint32_t offset{};
    clara::Parser parser;
    parser |= clara::Opt(cycle_time, "milliseconds")
        ["-c"]["--cycletime"]
        ("Set the cycle time of the data job");
    parser |= clara::Opt(offset, "milliseconds")
        ["-o"]["--offset"]
        ("Set the cycle time offset of the data job");

    parser.parse(clara::Args(argc, argv));

    auto elem_factory = std::make_shared<fep3::test::DataJobElementFactory<fep3::test::DataJobElement<ReceiveJob>>>(
        JobConfiguration(std::chrono::milliseconds(cycle_time), std::chrono::milliseconds(offset)));
    auto participant = fep3::core::createParticipant("test_receiver",
        FEP3_PARTICIPANT_LIBRARY_VERSION_STR,
        "test_system",
        elem_factory);

    participant.exec();
}