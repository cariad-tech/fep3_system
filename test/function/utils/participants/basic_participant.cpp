/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <chrono>
#include <iostream>

#include "basic_participant.h"

using namespace fep3;
using namespace std::chrono_literals;

BasicDataJob::BasicDataJob()
    : cpp::DataJob("basic_job", 1s)
{}

int main(int argc, const char* argv[])
{
    try
    {
        auto participant = cpp::createParticipant<cpp::DataJobElement<BasicDataJob>>
            (argc
            , argv
            , "basic_participant"
            , "basic_system"
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
