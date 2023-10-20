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

#include <fep3/cpp/element_base.h>
#include <fep3/cpp/participant.h>
#include <fep3/components/logging/logging_service_intf.h>

#include <iostream>

using namespace fep3;

class TransmitJobOne 
    : public TransmitJobBase
{
public:
    TransmitJobOne(const fep3::IComponents& components) 
        : TransmitJobBase(components, "transmit_job_one_logger")
    {
        _data_writer = addDataOut("data_one", fep3::base::StreamTypePlain<int32_t>());
    }
private:
    fep3::cpp::PropertyVariable<bool> _verbose{ false };
};

int main(int argc, char* argv[])
{
    try
    {
        auto participant = core::createParticipant<test::DataJobElementFactoryWithComponents<TransmitJobOne>>
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
