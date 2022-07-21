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


// Possible loss of data during conversion from long long to int in boost process
#pragma warning(disable:4244)

#include <gtest/gtest.h>
#include <boost/process.hpp>
#include <a_util/filesystem.h>
#include <a_util/strings.h>

#include <fep_system/fep_system.h>
#include "event_listener/event_listener.h"
#include <fep3/components/clock/clock_service_intf.h>

#include <array>
#include <fstream>

using namespace ::testing;
using namespace fep3;
namespace bp = boost::process;

TEST(Determinism, RegularOnTime)
{
    std::cout << "Working directory: " << boost::filesystem::current_path() << std::endl; // For debugging
    bp::child c1(bp::search_path("transmitter_one", { boost::filesystem::current_path() }), "--cycletime", "1", bp::std_out > stdout, bp::std_err > stderr);
    bp::child c2(bp::search_path("transmitter_two", { boost::filesystem::current_path() }), "--cycletime", "2", bp::std_out > stdout, bp::std_err > stderr);
    bp::child c3(bp::search_path("receiver", { boost::filesystem::current_path() }), "--cycletime", "1", bp::std_out > stdout, bp::std_err > stderr);

    auto my_sys = fep3::discoverSystem("test_system");
    my_sys.load();

    auto participant_transmitter_one = my_sys.getParticipant("test_transmitter_one");
    auto participant_transmitter_two = my_sys.getParticipant("test_transmitter_two");
    auto participant_receiver = my_sys.getParticipant("test_receiver");

    auto properties_transmitter_one = participant_transmitter_one.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>()->getProperties("/");
    auto properties_transmitter_two = participant_transmitter_two.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>()->getProperties("/");
    auto properties_receiver = participant_receiver.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>()->getProperties("/");

    std::vector<std::string> properties = properties_transmitter_one->getPropertyNames();

    properties_transmitter_one->setProperty("job_transmit/logfile", "sender_one_no_delay.txt", "string");
    properties_transmitter_two->setProperty("job_transmit/logfile", "sender_two_no_delay.txt", "string");
    properties_receiver->setProperty("job_receive/logfile", "receiver_no_delay.txt", "string");

    //experimental::EventListener event_listener;
    //event_listener.initRPCService("test_system");
    //event_listener.registerParticipant("test_receiver");

    my_sys.configureTiming3AFAP("test_receiver", std::to_string(FEP3_CLOCK_SIM_TIME_STEP_SIZE_MIN_VALUE));
    //properties_receiver->setProperty("clock/main_clock", "local_system_simtime_with_timeout", "string");
    //properties_receiver->setProperty("element/max_runtime", "10000", "int");

    my_sys.initialize();

    // DDS Transmission Component does not check if at least one sender is available for an input signal
    // before entering the initialized state
    std::this_thread::sleep_for(std::chrono::seconds(2));
    my_sys.start();

    //event_listener.waitForNotification();
    std::this_thread::sleep_for(std::chrono::seconds(2)); // work around until event listener works

    my_sys.stop();
    my_sys.deinitialize();
    my_sys.unload();
    my_sys.shutdown();

    std::vector<int> values_signal_one = { 8, 3, 6 };
    std::vector<int> values_signal_two = { 5, 2, 10 };


/*  // TODO: re-enable when the delay problem is resolved, see FEPSDK-3089
    std::array<std::array<int, 2>, 6> expected_values =
    { {
        {values_signal_one[0], values_signal_two[0]},
        {values_signal_one[1], 0},
        {values_signal_one[2], values_signal_two[1]},
        {values_signal_one[0], 0},
        {values_signal_one[1], values_signal_two[2]},
        {values_signal_one[2], 0}
    } };
*/
    std::ofstream expected_values_file{"expected_values.txt", std::fstream::out };
    EXPECT_FALSE(expected_values_file.fail());

    std::vector<std::string> text_lines;
    ASSERT_EQ(a_util::filesystem::readTextLines(a_util::filesystem::Path("receiver_no_delay.txt"), text_lines), a_util::filesystem::Error::OK);

    int signal_one_selector = 0, signal_two_selector = 0;
    for (std::string line : text_lines)
    {
        std::vector<std::string> data = a_util::strings::split(line, " ");
        // in case we receive an empty line or non complete line
        if (data.size() < 2)
        {
            continue;
        }

        if (a_util::strings::toInt32(data[1]) == 0) // No signals can be received in the first simulation time step
        {
            expected_values_file << "0 0 0 0 0" << std::endl;
            EXPECT_FALSE(expected_values_file.fail());
            expected_values_file.flush();
            continue;
        }
        if (a_util::strings::toInt32(data[1]) >= 300) // This is to shorten the test. Should be removed when event listener is used.
        {
            break;
        }
        /**
        * data[0]: Real time
        * data[1]: Simulation time
        * data[2]: Input signal one
        * data[3]: Input signal two
        * data[4]: Difference of both signals
        */
        int32_t expected_value_signal_one = values_signal_one[signal_one_selector];
        int32_t expected_value_signal_two = values_signal_two[signal_two_selector];
        if (a_util::strings::toInt32(data[2]) == expected_value_signal_one)
        {
            signal_one_selector = (signal_one_selector + 1) % 3;
        }
        else
        {
            EXPECT_EQ(a_util::strings::toInt32(data[2]), 0); // because of delay
        }

        if (a_util::strings::toInt32(data[3]) == expected_value_signal_two)
        {
            signal_two_selector = (signal_two_selector + 1) % 3;
        }
        else
        {
            EXPECT_EQ(a_util::strings::toInt32(data[3]), 0);
        }

        expected_values_file <<
            data[0] << " " << data[1] << " " <<
            expected_value_signal_one << " " <<
            expected_value_signal_two << " " <<
            (expected_value_signal_one - expected_value_signal_two) << std::endl;
        EXPECT_FALSE(expected_values_file.fail());
        expected_values_file.flush();
    }

    expected_values_file.close();

    EXPECT_TRUE(c1.wait_for(std::chrono::seconds(5)));
    EXPECT_TRUE(c2.wait_for(std::chrono::seconds(5)));
    EXPECT_TRUE(c3.wait_for(std::chrono::seconds(5)));
}
