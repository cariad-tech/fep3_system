/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <chrono>

#include <gtest/gtest.h>

#include <clipp.h>

#include "test_environment.h"

TestEnvironment* g_test_environment{nullptr};

int main(int argc, char* argv[])
{
    bool show_help{false};
    bool verbose{false};
    uint32_t number_of_participants{3};
    uint32_t job_cycle_time_in_ms{1000};
    uint32_t test_duration_in_ms{6000};
    uint32_t rpc_request_period_in_ms{500};
    uint32_t heartbeat_interval_in_ms{2000};
    
    auto cli = 
        (clipp::option("-?", "-h", "--help")
            .set(show_help)
            .doc("show help")
        , clipp::option("-v", "--verbose")
            .set(verbose)
            .doc("provide additional test progess details on stdout")
        );
    const auto number_of_participants_option = (clipp::option("-n", "--number-of-participants") & clipp::value("number", number_of_participants))
        % (std::string() + "number of participants to participate in the test (default: " + std::to_string(number_of_participants) + ")");
    const auto test_duration_option = (clipp::option("-d", "--test-duration") & clipp::value("duration", test_duration_in_ms))
        % ("the duration of the test to run [ms] (default: " + std::to_string(test_duration_in_ms) + ")");
    const auto job_cycle_time_option = (clipp::option("-c", "--cycle-time") & clipp::value("time", job_cycle_time_in_ms))
        % ("the job cycle time of the participants [ms] (default: " + std::to_string(job_cycle_time_in_ms) + ")");
    const auto rpc_request_period_option = (clipp::option("-r", "--rpc-request-period") & clipp::value("period", rpc_request_period_in_ms))
        % ("the period of RPC request [ms] (default: " + std::to_string(rpc_request_period_in_ms) + ")");
    const auto heartbeat_interval_option = (clipp::option("-i", "--heartbeat-interval") & clipp::value("interval", heartbeat_interval_in_ms))
        % ("the heartbeat interval [ms] (default: " + std::to_string(heartbeat_interval_in_ms) + ")");
    cli.push_back(number_of_participants_option);
    cli.push_back(test_duration_option);
    cli.push_back(job_cycle_time_option);
    cli.push_back(rpc_request_period_option);
    cli.push_back(heartbeat_interval_option);

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
            , number_of_participants
            , job_cycle_time_in_ms
            , test_duration_in_ms
            , rpc_request_period_in_ms
            , heartbeat_interval_in_ms
            );
        if(g_test_environment->_verbose)
        {
            std::cout << "Testing with verbose output." << std::endl;
            std::cout << "\t" << number_of_participants_option.doc() << ": " << number_of_participants << std::endl;
            std::cout << "\t" << job_cycle_time_option.doc() << ": " << job_cycle_time_in_ms << std::endl;
            std::cout << "\t" << test_duration_option.doc() << ": " << test_duration_in_ms << std::endl;
            std::cout << "\t" << rpc_request_period_option.doc() << ": " << rpc_request_period_in_ms << std::endl;
            std::cout << "\t" << heartbeat_interval_option.doc() << ": " << heartbeat_interval_in_ms << std::endl;
        }

        testing::AddGlobalTestEnvironment(g_test_environment);

        return RUN_ALL_TESTS();
    }
}