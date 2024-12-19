/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


// Possible loss of data during conversion from long long to int in boost process
#pragma warning(disable:4244)

#include <gtest/gtest.h>
#include <boost/process.hpp>

#include <fep_system/fep_system.h>
#include <fep3/components/clock/clock_service_intf.h>
#include <fep_system/event_monitor_intf.h>
#include <fep3/fep3_filesystem.h>

#include <array>
#include <fstream>

#include <fep_test_common.h>
#include "test_environment.h"

extern TestEnvironment* g_test_environment;

using namespace ::testing;
using namespace fep3;
using namespace std::chrono;
using namespace std::chrono_literals;
namespace bp = boost::process;

struct EventMonitorLogger : public fep3::IEventMonitor
{
    EventMonitorLogger(std::future<void>& test_finished_future)
    {
        test_finished_future = _test_finished_promise.get_future();
    }
    void onLog(std::chrono::milliseconds log_time,
        LoggerSeverity,
        const std::string& participant_name,
        const std::string& logger_name,
        const std::string& message) override
    {
        std::lock_guard<std::mutex> lock_guard(_mutex);
        std::cout << log_time.count() << "ms: " <<
            participant_name << ": " <<
            logger_name << ": " <<
            message << std::endl;
        if (message.find("Participant finished work") != std::string::npos)
        {
            _participants_finished++;
            if (_participants_finished > 2)
            {
                _test_finished_promise.set_value();
            }
        }
    }

private:
    std::mutex _mutex;
    uint32_t _participants_finished{ 0 };
    std::promise<void> _test_finished_promise;
};

TEST(Determinism, TransmissionReception)
{
    const auto test_signal_connection_timeout = 2s;

    // * in case of delayed reception set, the transmitters and the receiver are configured
    //     for alternating job execution:
    //     t=0: execution of transmitter
    //     t=1: execution of receiver
    //     t=2: execution of transmitter
    //     t=3: execution of receiver
    //     ...
    // * in case of delayed reception not set, the transmitters and the receiver are configured
    //     for simultaneous job execution:
    //     t=0: execution of transmitter and receiver
    //     t=1: no execution
    //     t=2: execution of transmitter and receiver
    //     t=3: no execution
    //     ...
    const nanoseconds cycle_time = g_test_environment->_step_size * 2;
    const nanoseconds transmitter_wait_time = g_test_environment->_waiting_transmisison
        ? cycle_time : 0ns;
    const nanoseconds receiver_delay_time = g_test_environment->_delayed_reception
        ? g_test_environment->_step_size : 0ns;

    const auto transmitter_one_log_file_path = "transmitter_one_transmitted_samples.txt";
    const auto transmitter_two_log_file_path = "transmitter_two_transmitted_samples.txt";
    const auto receiver_one_log_file_path = "receiver_received_samples_data_one.txt";
    const auto receiver_two_log_file_path = "receiver_received_samples_data_two.txt";

    if(g_test_environment->_verbose)
    {
        std::cout << "current working directory: " << boost::filesystem::current_path() << std::endl;
        std::cout << "cycle time (transmitters and receivers): " << cycle_time.count() << "ns" << std::endl;
        std::cout << "wait time (transmitter): " << transmitter_wait_time.count() << "ns" << std::endl;
        std::cout << "delay time (receiver): " << receiver_delay_time.count() << "ns" << std::endl;
    }

    bp::group participant_processes_group;
    const auto test_system_name = makePlatformDepName("test_system");
    bp::child transmitter_one_process
    (bp::search_path("transmitter_one", { boost::filesystem::current_path() })
        , bp::args = {"--name", "test_transmitter_one", "--system_name", test_system_name}
        , bp::std_out > stdout
        , bp::std_err > stderr
        , participant_processes_group
        );
    bp::child transmitter_two_process
    (bp::search_path("transmitter_two", { boost::filesystem::current_path() })
        , bp::args = {"--name", "test_transmitter_two", "--system_name", test_system_name}
        , bp::std_out > stdout
        , bp::std_err > stderr
        , participant_processes_group
        );
    bp::child receiver_process
    (bp::search_path("receiver", { boost::filesystem::current_path() })
        , bp::args = {"--name", "test_receiver", "--system_name", test_system_name}
        , bp::std_out > stdout
        , bp::std_err > stderr
        , participant_processes_group
        );

    auto my_sys = fep3::discoverSystem
        (test_system_name
        , { "test_transmitter_one", "test_transmitter_two", "test_receiver"}, 5000ms);

    my_sys.load();

    auto participant_transmitter_one = my_sys.getParticipant("test_transmitter_one");
    auto participant_transmitter_two = my_sys.getParticipant("test_transmitter_two");
    auto participant_receiver = my_sys.getParticipant("test_receiver");

    auto properties_transmitter_one = participant_transmitter_one.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>()->getProperties("/");
    auto properties_transmitter_two = participant_transmitter_two.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>()->getProperties("/");
    auto properties_receiver = participant_receiver.getRPCComponentProxy<fep3::rpc::IRPCConfiguration>()->getProperties("/");

    std::vector<std::string> properties = properties_transmitter_one->getPropertyNames();

    ASSERT_TRUE(properties_transmitter_one->setProperty("job_transmit/verbose", g_test_environment->_verbose ? "true" : "false", "bool"));
    ASSERT_TRUE(properties_transmitter_one->setProperty("job_transmit/log_file_path", transmitter_one_log_file_path, "string"));
    ASSERT_TRUE(properties_transmitter_one->setProperty("job_transmit/number_of_transmissions", std::to_string(g_test_environment->_number_of_samples), "uint32"));
    ASSERT_TRUE(properties_transmitter_one->setProperty("job_transmit/wait_real_time_ns", std::to_string(transmitter_wait_time.count()), "uint64"));
    ASSERT_TRUE(properties_transmitter_one->setProperty("logging/default_sinks", "rpc", "string"));

    ASSERT_TRUE(properties_transmitter_two->setProperty("job_transmit/verbose", g_test_environment->_verbose ? "true" : "false", "bool"));
    ASSERT_TRUE(properties_transmitter_two->setProperty("job_transmit/log_file_path", transmitter_two_log_file_path, "string"));
    ASSERT_TRUE(properties_transmitter_two->setProperty("job_transmit/number_of_transmissions", std::to_string(g_test_environment->_number_of_samples), "uint32"));
    ASSERT_TRUE(properties_transmitter_two->setProperty("job_transmit/wait_real_time_ns", std::to_string(transmitter_wait_time.count()), "uint64"));
    ASSERT_TRUE(properties_transmitter_two->setProperty("logging/default_sinks", "rpc", "string"));

    ASSERT_TRUE(properties_receiver->setProperty("job_receive/verbose", g_test_environment->_verbose ? "true" : "false", "bool"));
    ASSERT_TRUE(properties_receiver->setProperty("job_receive/log_file_path_data_one", receiver_one_log_file_path, "string"));
    ASSERT_TRUE(properties_receiver->setProperty("job_receive/log_file_path_data_two", receiver_two_log_file_path, "string"));
    ASSERT_TRUE(properties_receiver->setProperty("logging/default_sinks", "rpc", "string"));
    // we want to receive exactly the same number as transmissions
    ASSERT_TRUE(properties_receiver->setProperty
        ("job_receive/number_of_receptions"
        // Note: In case of non-delayed reception, the reception takes place in the next cycle, so there won't be any samples in the very first reception 
        //       but there will be samples to be received in the (_number_of_samples + 1)th cycle.
        , std::to_string(g_test_environment->_number_of_samples + (g_test_environment->_delayed_reception ? 0 : 1))
        , "uint32"));

    // note: Depending on the initialization order, the receiver(s) might wait for the transmitters to register their signals 
    //       (due to usage of "FEP3_RTI_DDS_SIMBUS_MUST_BE_READY_SIGNALS"), so we set the number of thread_count to the total number of participants
    //       which are initialized in parallel which is 2 (transmitters) + 1 (timing master) = 3
    my_sys.setInitAndStartPolicy(fep3::System::InitStartExecutionPolicy::parallel, 3);
    my_sys.configureTiming3DiscreteSteps
        ("test_transmitter_two"
        , std::to_string(duration_cast<nanoseconds>(g_test_environment->_step_size).count())
        , std::to_string(g_test_environment->_time_factor)
        );
    ASSERT_TRUE(properties_transmitter_one->setProperty("job_registry/jobs/transmit/cycle_sim_time", std::to_string(cycle_time.count()), "int32"));
    ASSERT_TRUE(properties_transmitter_two->setProperty("job_registry/jobs/transmit/cycle_sim_time", std::to_string(cycle_time.count()), "int32"));
    ASSERT_TRUE(properties_receiver->setProperty("job_registry/jobs/receive/cycle_sim_time", std::to_string(cycle_time.count()), "int32"));
    ASSERT_TRUE(properties_receiver->setProperty("job_registry/jobs/receive/delay_sim_time", std::to_string(receiver_delay_time.count()), "int32"));
    ASSERT_TRUE(properties_receiver->setProperty
        (FEP3_RTI_DDS_SIMBUS_DATAWRITER_READY_TIMEOUT
        , std::to_string(duration_cast<nanoseconds>(test_signal_connection_timeout).count())
        , "int64"
        ));
    ASSERT_TRUE(properties_receiver->setProperty
        (FEP3_RTI_DDS_SIMBUS_MUST_BE_READY_SIGNALS
        , "*" // wait for all signals
        , "array-string"
        ));

    // Due to the wait for signal feature we use to ensure data transmitters and receivers are coupled
    // before the system starts we need the receiver participant to be initialized last and
    // set the init prioroties accordingly.
    participant_transmitter_one.setInitPriority(2);
    participant_transmitter_two.setInitPriority(2);
    participant_receiver.setInitPriority(1);

    // Due to FEPSDK-3089 we want to ensure the timing master is started last and therefore set the
    // start priorities accordingly.
    participant_transmitter_one.setStartPriority(2);
    participant_transmitter_two.setStartPriority(1);
    participant_receiver.setStartPriority(2);

    std::future<void> test_finished_future;
    EventMonitorLogger event_monitor_logger(test_finished_future);
    my_sys.registerMonitoring(event_monitor_logger);

    my_sys.initialize();

    my_sys.start();

    // As transmission starts at t = 0, the test has to be running at least for the time, the number of discrete timing steps require to 
    // perform transmission *and* reception, plus an extra step size in order to make sure, the last job execution is actually performed
    // plus the optional transmitter waiting time.
    // As puffer the minimum time to wait for is doubled as we will stop waiting as soon as all participants finished.
    test_finished_future.wait_for(((cycle_time* g_test_environment->_number_of_samples)
        + g_test_environment->_step_size
        + g_test_environment->_number_of_samples * transmitter_wait_time)
        * 2);

    my_sys.stop();
    my_sys.deinitialize();
    my_sys.unload();
    my_sys.shutdown();

    EXPECT_TRUE(participant_processes_group.wait_for(5s));

    // Note: We explicitly don't want to load the full file content into memory (e. g. loading it to an std::string),
    //       because this would cause memory issues with large number of samples (big files). We rather use the std::equal
    //       function on the stream buffers, which makes sure to load only portions of the files into memory.

    // compare transmitted samples and received samples of data_one signal
    std::ifstream transmitter_one_transmitted_samples_stream(transmitter_one_log_file_path);
    ASSERT_TRUE(transmitter_one_transmitted_samples_stream.good());
    std::ifstream receiver_received_samples_data_one_stream(receiver_one_log_file_path);
    ASSERT_TRUE(receiver_received_samples_data_one_stream.good());
    ASSERT_EQ
        (g_test_environment->_number_of_samples
        , std::count(std::istreambuf_iterator<char>(transmitter_one_transmitted_samples_stream), std::istreambuf_iterator<char>(), '\n')
        );
    ASSERT_EQ
        (g_test_environment->_number_of_samples
        , std::count(std::istreambuf_iterator<char>(receiver_received_samples_data_one_stream), std::istreambuf_iterator<char>(), '\n')
        );
    transmitter_one_transmitted_samples_stream.seekg(0, std::ifstream::beg);
    receiver_received_samples_data_one_stream.seekg(0, std::ifstream::beg);
    EXPECT_TRUE(std::equal
        (std::istreambuf_iterator<char>(transmitter_one_transmitted_samples_stream.rdbuf())
        , std::istreambuf_iterator<char>()
        , std::istreambuf_iterator<char>(receiver_received_samples_data_one_stream.rdbuf())
        ));

    // compare transmitted samples and received samples of data_two signal
    std::ifstream transmitter_two_transmitted_samples_stream(transmitter_two_log_file_path);
    ASSERT_TRUE(transmitter_two_transmitted_samples_stream.good());
    std::ifstream receiver_received_samples_data_two_stream(receiver_two_log_file_path);
    ASSERT_TRUE(receiver_received_samples_data_two_stream.good());
    ASSERT_EQ
        (g_test_environment->_number_of_samples
        , std::count(std::istreambuf_iterator<char>(transmitter_two_transmitted_samples_stream), std::istreambuf_iterator<char>(), '\n')
        );
    ASSERT_EQ
        (g_test_environment->_number_of_samples
        , std::count(std::istreambuf_iterator<char>(receiver_received_samples_data_two_stream), std::istreambuf_iterator<char>(), '\n')
        );
    transmitter_two_transmitted_samples_stream.seekg(0, std::ifstream::beg);
    receiver_received_samples_data_two_stream.seekg(0, std::ifstream::beg);
    EXPECT_TRUE(std::equal
        (std::istreambuf_iterator<char>(transmitter_two_transmitted_samples_stream.rdbuf())
        , std::istreambuf_iterator<char>()
        , std::istreambuf_iterator<char>(receiver_received_samples_data_two_stream.rdbuf())
        ));
}
