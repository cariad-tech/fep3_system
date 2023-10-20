/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2022 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */

#include <chrono>

#include <gtest/gtest.h>

#include <clipp.h>

#include "test_environment.h"

TestEnvironment* g_test_environment{nullptr};

int main(int argc, char* argv[])
{
    bool show_help{ false };
    bool verbose{ false };
    uint32_t number_of_samples{ 10 };
    uint32_t step_size_in_ms{ 1000 };
    float time_factor{ 1.0f };
    bool waiting_transmission{ true };
    bool delayed_reception{ false };

    auto cli = 
        (clipp::option("-?", "-h", "--help")
            .set(show_help)
            .doc("show help")
        , clipp::option("-v", "--verbose")
            .set(verbose)
            .doc("provide additional test progess details on stdout (default: not set)")
        );
    const auto number_of_samples_option = (clipp::option("-n", "--number-of-samples") & clipp::value("number", number_of_samples))
        % ("the number of samples to be transmitted by each transmitter (default: " + std::to_string(number_of_samples) + ")");
    const auto step_size_option = (clipp::option("-s", "--step-size") & clipp::value("time", step_size_in_ms))
        % ("the discrete's clock step size [ms] (default: " + std::to_string(step_size_in_ms) + ")");
    const auto time_factor_option = (clipp::option("-f", "--time-factor") & clipp::value("factor", time_factor))
        % ("the discrete's clock time factor (default: " + std::to_string(time_factor) + ")");
    const auto waiting_transmission_option = clipp::option("-p", "--waiting-transmission")
        .set(waiting_transmission)
        .doc
        (std::string()
            + "whether to make the transmitter participants' jobs wait by step-size in real time (set) or not (not set)"
            + "(default: " + (waiting_transmission ? "set" : "not set") + ")"
        );
    const auto delayed_reception_option = clipp::option("-d", "--delayed-reception")
        .set(delayed_reception)
        .doc
            (std::string() 
            + "whether to delay the receiver participant's job by step-size (set) or not (not set)"
            + "(default: " + (delayed_reception ? "set" : "not set") + ")"
            );
    cli.push_back(number_of_samples_option);
    cli.push_back(step_size_option);
    cli.push_back(time_factor_option);
    cli.push_back(waiting_transmission_option);
    cli.push_back(delayed_reception_option);

    // pass the command line arguments to google test first in order to support controlling it via 
    // google-test-specific options (e. g. --gtest_repeat)
    ::testing::InitGoogleTest(&argc, argv);

    if(!clipp::parse(argc, argv, cli))
    {
        std::cout << make_man_page(cli, argv[0]);
        return -1;
    }
    else if(show_help)
    {
        std::cout << make_man_page(cli, argv[0]);
        return 0;
    }
    else
    {
        g_test_environment = new TestEnvironment
            (verbose
            , number_of_samples
            , step_size_in_ms
            , time_factor
            , waiting_transmission
            , delayed_reception
            );
        if(g_test_environment->_verbose)
        {
            std::cout << "Testing with verbose output." << std::endl;
            std::cout << "\t" << number_of_samples_option.doc() << ": " << number_of_samples << std::endl;
            std::cout << "\t" << step_size_option.doc() << ": " << step_size_in_ms << std::endl;
            std::cout << "\t" << time_factor_option.doc() << ": " << time_factor << std::endl;
            std::cout << "\t" << waiting_transmission_option.doc() << ": " << waiting_transmission << std::endl;
            std::cout << "\t" << delayed_reception_option.doc() << ": " << (delayed_reception ? "set" : "not set") << std::endl;
        }

        testing::AddGlobalTestEnvironment(g_test_environment);

        return RUN_ALL_TESTS();
    }
}