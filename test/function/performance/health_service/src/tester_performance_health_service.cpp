/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <chrono>

// Possible loss of data during conversion from long long to int in boost process
#ifdef WIN32
    #pragma warning( push )
    #pragma warning(disable:4244)
#endif
#include <boost/process.hpp>
#ifdef WIN32
    #pragma warning( pop )
#endif
#include <boost/asio.hpp>

#include <gtest/gtest.h>

#include <fep_test_common.h>
#include <fep_system/fep_system.h>
#include <fep_system/rpc_services/health/health_service_rpc_intf.h>
#include <fep_system/mock_event_monitor.h>
#include "test_environment.h"

// value printers for gtest
void PrintTo(const std::chrono::nanoseconds& nanoseconds, std::ostream* os) 
{
    *os << nanoseconds.count() << "ns";
}
void PrintTo(const std::chrono::microseconds& microseconds, std::ostream* os) 
{
    *os << microseconds.count() << "us";
}
void PrintTo(const std::chrono::milliseconds& milliseconds, std::ostream* os) 
{
    *os << milliseconds.count() << "ms";
}
void PrintTo(const std::chrono::seconds& seconds, std::ostream* os) 
{
    *os << seconds.count() << "s";
}

extern TestEnvironment* g_test_environment;

using namespace std::literals::chrono_literals;

using namespace fep3;

class HealthServicePerformance : public ::testing::Test
{
public:
    HealthServicePerformance()
        : participant_processes_(g_test_environment->_number_of_participants)
        , participant_process_output_(g_test_environment->_number_of_participants)
    {}
    
    void SetUp() override
    {
        participant_names_.resize(g_test_environment->_number_of_participants);
        std::generate
            (participant_names_.begin()
            , participant_names_.end()
            , [current_index = uint32_t{1}]() mutable
                {
                    return std::string() + "participant_" + std::to_string(current_index++);
                }
            );
        
        const auto test_system_name = makePlatformDepName("test_system");
        for(uint32_t participant_index = 0; participant_index < g_test_environment->_number_of_participants; ++participant_index)
        {
            const auto participant_name = participant_names_[participant_index];

            participant_processes_[participant_index] = boost::process::child
            (boost::process::exe = PARTICIPANT_EXECUTABLE_NAME
                , boost::process::args = { "--name", participant_name, "--system_name", test_system_name }
                , (boost::process::std_err & boost::process::std_out) > participant_process_output_[participant_index], participant_process_ios_
                , participant_processes_group_
            );
            
        }

        participant_process_ios_thread_ = std::thread([&]() { participant_process_ios_.run(); });

        try
        {
            fep_system_ = discoverSystem
                (test_system_name
                , participant_names_
                , 10s
                );
        }
        catch(const std::exception& ex)
        {
            FAIL() << ex.what();
        }
        
        participant_proxies_ = fep_system_.getParticipants();
        ASSERT_EQ(participant_proxies_.size(), g_test_environment->_number_of_participants);
        
        for(size_t participant_index = 0; participant_index < g_test_environment->_number_of_participants; ++participant_index)
        {
            configuration_proxies_.push_back(participant_proxies_[participant_index].getRPCComponentProxy<rpc::IRPCConfiguration>());
        }
        
        fep_system_.load();
        
        for(size_t participant_index = 0; participant_index < g_test_environment->_number_of_participants; ++participant_index)
        {
            auto job_property_node = configuration_proxies_[participant_index]->getProperties("/job_registry/jobs/basic_job");
            ASSERT_TRUE(job_property_node);
            ASSERT_TRUE(job_property_node->setProperty
                ("cycle_sim_time"
                , std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(g_test_environment->_job_cycle_time).count())
                , "int32"
                ));
        }
        
        fep_system_.initialize();
    }
    
    void TearDown() override
    {
        fep_system_.deinitialize();
        fep_system_.unload();
        fep_system_.shutdown();
           
        bool participants_exit_in_time = participant_processes_group_.wait_for(std::chrono::seconds(20));
        EXPECT_TRUE(participants_exit_in_time);

        // cleanup with further information: terminate remaining participant processes and print their output
        for (size_t participant_index = 0; participant_index < g_test_environment->_number_of_participants; ++participant_index)
        {
            if (participant_processes_group_.has(participant_processes_[participant_index]))
            {
                participant_processes_[participant_index].terminate();
            }
        }

        participant_process_ios_thread_.join();

        if (!participants_exit_in_time) {

            for (size_t participant_index = 0; participant_index < g_test_environment->_number_of_participants; ++participant_index)
            {
                auto out = participant_process_output_[participant_index].get();
                std::cerr << out << std::endl;
            }
        }
    }
protected:
    // map participant name to index; this helps to avoid expensive string comparisons during test execution
    std::vector<std::string> participant_names_;
    std::vector<boost::process::child> participant_processes_;
    std::thread participant_process_ios_thread_;
    boost::asio::io_service participant_process_ios_;
    std::vector<std::future<std::string>> participant_process_output_;
    boost::process::group participant_processes_group_;
    std::vector<ParticipantProxy> participant_proxies_;
    std::vector<RPCComponent<rpc::IRPCConfiguration>> configuration_proxies_;
    System fep_system_;
};

TEST_F(HealthServicePerformance, PeriodicRequestsOfParticipantsHealthViaRPC)
{
    std::vector<RPCComponent<rpc::IRPCHealthService>> health_service_proxies;
    for(size_t participant_index = 0; participant_index < g_test_environment->_number_of_participants; ++participant_index)
    {
        health_service_proxies.push_back(participant_proxies_[participant_index].getRPCComponentProxy<rpc::IRPCHealthService>());
    }
    
    fep_system_.start();

    auto rpc_requests_count = decltype(g_test_environment->_test_duration / g_test_environment->_rpc_request_period){0};
    
    const auto begin_time = std::chrono::steady_clock::now();
    // let it run until enough rpc requests have been performed
    auto participant_index = size_t{0};
    while(rpc_requests_count < g_test_environment->_test_duration / g_test_environment->_rpc_request_period)
    {
        const std::vector<JobHealthiness> jobs_healthiness = health_service_proxies[participant_index]->getHealth();
        EXPECT_EQ(1, jobs_healthiness.size());
        rpc_requests_count++;
        
        participant_index++;
        if(participant_index == g_test_environment->_number_of_participants)
        {
            participant_index = 0;
        }
    }
    const auto end_time = std::chrono::steady_clock::now();
    const auto actual_duration = end_time - begin_time;
    EXPECT_GE(g_test_environment->_test_duration, actual_duration);
    
    if(g_test_environment->_verbose)
    {
        std::cout << "performed " << rpc_requests_count << " RPC requests "
            << "in " << std::chrono::duration_cast<std::chrono::nanoseconds>(actual_duration).count() << "ns" << std::endl;
    }

    fep_system_.stop();
}


TEST_F(HealthServicePerformance, HeartBeatEventsPerSecond)
{
    // Test verification: The heart beat interval must be greater than the job cycle time. 
    //                    Otherwise, we cannot expect a change of the health information 
    //                    (esp. the simulation time ) with every heartbeat. Even with 
    //                    heart beat interval being just a little greater than the job cycle time 
    //                    race conditions might occur, where the health information still doesn't 
    //                    change with each heart beat. Therefore we require:
    ASSERT_GE(g_test_environment->_heartbeat_interval, g_test_environment->_job_cycle_time / 2);
    
    std::vector<RPCComponent<rpc::IRPCHttpServer>> http_server_proxies;
    for(size_t participant_index = 0; participant_index < g_test_environment->_number_of_participants; ++participant_index)
    {
        http_server_proxies.push_back(participant_proxies_[participant_index].getRPCComponentProxy<rpc::IRPCHttpServer>());
        http_server_proxies.back()->setHeartbeatInterval(g_test_environment->_heartbeat_interval);
    }
    
    fep_system_.start();
    
    struct TestData
    {
        std::chrono::nanoseconds previous_simulation_time_{0ns};
        uint32_t health_change_counter_{0};
    };
    std::map<std::string, TestData> participants_test_data;

    // wait until the initial health has been cached for each participant 
    // (participants might take some time until they are running)
    bool initial_healths_cached = false;
    const auto initial_health_waiting_begin_time = std::chrono::steady_clock::now();
    std::map<std::string, ParticipantHealth> initial_participants_health;
    while(std::chrono::steady_clock::now() - initial_health_waiting_begin_time < 10s)
    {
        initial_participants_health = fep_system_.getParticipantsHealth();
        if(initial_participants_health.size() == g_test_environment->_number_of_participants)
        {
            auto initial_participant_health_iter = initial_participants_health.cbegin();
            for(; initial_participant_health_iter != initial_participants_health.cend()
                ; ++initial_participant_health_iter)
            {
                if(initial_participant_health_iter->second.jobs_healthiness.size() == 1)
                {
                    ASSERT_NE(std::find(participant_names_.cbegin(), participant_names_.cend(), initial_participant_health_iter->first), participant_names_.cend())
                        << "found health information for participant \"" << initial_participant_health_iter->first 
                        << " which shouldn't be participating in the test system";
                    participants_test_data[initial_participant_health_iter->first] 
                        = TestData{initial_participant_health_iter->second.jobs_healthiness.at(0).simulation_time, 0};
                }
                else
                {
                    break;
                }
            }
            // if for each participant the initial health for the job has been cached
            if(initial_participant_health_iter == initial_participants_health.cend())
            {
                initial_healths_cached = true;
                break;
            }
        }
        std::this_thread::sleep_for(g_test_environment->_heartbeat_interval);
    }
    ASSERT_TRUE(initial_healths_cached);

    const auto begin_time = std::chrono::steady_clock::now();

    while(std::chrono::steady_clock::now() - begin_time < g_test_environment->_test_duration)
    {
        const auto current_participant_healths = fep_system_.getParticipantsHealth();
        for(const auto& current_participant_health_iter : current_participant_healths)
        {
            auto test_data_iter = participants_test_data.find(current_participant_health_iter.first);
            ASSERT_NE(participants_test_data.cend(), test_data_iter)
                << "found health information for participant \"" << current_participant_health_iter.first 
                << " which shouldn't be participating in the test system";
            auto& participant_test_data = test_data_iter->second;
            // at least the simulation time must have changed
            if(participant_test_data.previous_simulation_time_
                != current_participant_health_iter.second.jobs_healthiness.at(0).simulation_time)
            {
                participant_test_data.health_change_counter_++;
            }
        }
        // the test implements oversampling of getParticipantHealth in order to prevent race conditions
        std::this_thread::sleep_for(g_test_environment->_heartbeat_interval / 2);
    }

    const auto end_time = std::chrono::steady_clock::now();
    const auto actual_duration = end_time - begin_time;
    EXPECT_EQ(g_test_environment->_number_of_participants, participants_test_data.size());
    // for each participant, test that the expected number of health changes were detected
    for(auto participant_test_data : participants_test_data)
    {
        EXPECT_LE(actual_duration / g_test_environment->_heartbeat_interval, participant_test_data.second.health_change_counter_)
            << "for participant \"" << participant_test_data.first << "\"";
    }
    
    if(g_test_environment->_verbose)
    {
        std::cout << "the following number of health changes has been recorded "
            << "in " << std::chrono::duration_cast<std::chrono::nanoseconds>(actual_duration).count() << "ns: " << std::endl;
        for(auto participant_test_data : participants_test_data)
        {
            std::cout << "\t participant index \"" << participant_test_data.first << "\": " << participant_test_data.second.health_change_counter_ << std::endl;
        }
    }

    fep_system_.stop();
}
