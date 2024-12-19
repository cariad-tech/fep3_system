/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#include "transmitter_base.h"

#include <fep3/cpp/element_base.h>
#include <fep3/cpp/participant.h>

#include <iostream>

using namespace fep3;

class TransmitJobTwo : public TransmitJobBase
{
public:
    TransmitJobTwo(const fep3::IComponents& components)
        : TransmitJobBase(components, "transmit_job_two_logger")
    {
        _data_writer = addDataOut("data_two", fep3::base::StreamTypePlain<uint32_t>());
    }
};

int main(int argc, char* argv[])
{
    try
    {
        auto participant = core::createParticipant<test::DataJobElementFactoryWithComponents<TransmitJobTwo>>
            (argc
            , argv
            , FEP3_PARTICIPANT_LIBRARY_VERSION_STR
            );
        return participant.exec();
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what();
        return -1;
    }
    catch (...)
    {
        std::cerr << "exception caught";
        return -1;
    }
}
