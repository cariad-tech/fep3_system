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

#include <fep3/cpp/participant.h>

#include <3rdparty/clara/Clara-1.1.2/single_include/clara.hpp>

class TransmitJobOne : public TransmitJobBase
{
public:
    TransmitJobOne(fep3::arya::JobConfiguration job_config) : TransmitJobBase(job_config)
    {
        _data_writer = addDataOut("data_one", fep3::base::StreamTypePlain<int32_t>());
        _values = { 8, 3, 6 };
    }
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

    auto elem_factory = std::make_shared<fep3::test::DataJobElementFactory<fep3::test::DataJobElement<TransmitJobOne>>>(
        fep3::JobConfiguration(std::chrono::milliseconds(cycle_time), std::chrono::milliseconds(offset)));
    auto participant = fep3::core::createParticipant("test_transmitter_one",
        FEP3_PARTICIPANT_LIBRARY_VERSION_STR,
        "test_system",
        elem_factory);

    participant.exec();

    return{};
}