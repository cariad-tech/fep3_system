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


#include <string.h>
#include <future>

#include <gtest/gtest.h>

#include <fep_test_common.h>
#include <fep_system/fep_system.h>
#include <fep3/fep3_errors.h>
#include <fep3/cpp/datajob.h>
#include "fep_system/rpc_services/health/health_service_rpc_intf.h"
#include <gtest_asserts.h>

using namespace fep3;

struct SystemHealthTest : public testing::Test
{
    SystemHealthTest()
        : _system_name(makePlatformDepName("test_system"))
    {
    }

    void SetUp() override
    {
        // health state tests require the tested participant to provide a health service component which is not enabled by default currently
        const std::string components_file_path(std::string(TEST_BUILD_DIR));

        using namespace std::chrono_literals;
        _participants = createTestParticipants({ _participant_name }, _system_name);
        _system = fep3::discoverSystem(_system_name, { _participant_name }, 10s);
    }

    fep3::System _system;
    const std::string _system_name;
    TestParticipants _participants{};
    const std::string _participant_name{"test_participant"};
};

class TestEventMonitor : public fep3::IEventMonitor
{
public:
    TestEventMonitor(std::future<void>& execute_future, std::string log_content)
        : _log_content(std::move(log_content))
    {
        execute_future = execute_promise.get_future();
    }

    void onLog(std::chrono::milliseconds,
        fep3::LoggerSeverity,
        const std::string&,
        const std::string&, //depends on the Category ...
        const std::string& message) override
    {
        if (message.find(_log_content) != std::string::npos)
        {
            execute_promise.set_value();
        }
    }

private:
    std::promise<void> execute_promise;
    const std::string _log_content;

};

class TestJob : public fep3::IJob
{
public:
    TestJob(std::future<void>& execute_future)
    {
        execute_future = execute_promise.get_future();
    }
    fep3::Result executeDataIn(arya::Timestamp) override
    {
        return {};
    }
    fep3::Result execute(arya::Timestamp) override
    {
        ++execution_count;
        if (execution_count == 5)
        {
            execute_promise.set_value();
        }
        return {};
    }

    fep3::Result executeDataOut(arya::Timestamp) override
    {
        return {};
    }

protected:
    uint32_t execution_count = 0;
    std::promise<void> execute_promise;
};

class TestJobWithError : public TestJob
{
public:
    TestJobWithError(std::future<void>& execute_future)
        : TestJob(execute_future)
    {
    }

    fep3::Result executeDataIn(arya::Timestamp) override
    {
        return getError(CREATE_ERROR_DESCRIPTION(fep3::ERR_DEVICE_IO, "error 1 executeDataIn"),
            CREATE_ERROR_DESCRIPTION(fep3::ERR_NOT_CONNECTED, "error 2 executeDataIn"));

    }
    fep3::Result execute(arya::Timestamp) override
    {
        return getError(CREATE_ERROR_DESCRIPTION(fep3::ERR_CANCELLED, "error 1 execute"),
            CREATE_ERROR_DESCRIPTION(fep3::ERR_BAD_DEVICE, "error 2 execute"));
    }

    fep3::Result executeDataOut(arya::Timestamp) override
    {
        ++execution_count;
        if (execution_count == 5)
        {
            execute_promise.set_value();
        }
        return getError(CREATE_ERROR_DESCRIPTION(fep3::ERR_EMPTY, "error 1 executeDataOut"),
            CREATE_ERROR_DESCRIPTION(fep3::ERR_FAILED, "error 2 executeDataOut"));
    }
private:
    fep3::Result getError(fep3::Result error1, fep3::Result error2)
    {
        if (execution_count == 3)
        {
            return error1;
        }
        else if (execution_count == 4)
        {
            return error2;
        }
        else
        {
            return {};
        }
    }
};

/**
 * @brief Test whether the system library returns the correct participant health state via rpc using getHealth.
 */
TEST_F(SystemHealthTest, TestParticpantGetHealth)
{
    std::future<void> log_future;
    TestEventMonitor tem(log_future, "Received update event from ");
    _system.registerMonitoring(tem);

    using namespace std::chrono_literals;
    auto* job_registry = _participants[_participant_name]->_part.getComponent<fep3::IJobRegistry>();
    ASSERT_TRUE(job_registry);

    auto data_registry = _participants[_participant_name]->_part.getComponent<fep3::IDataRegistry>();
    data_registry->registerDataIn("test_signal", fep3::base::StreamTypePlain<int32_t>());

    std::future<void> execute_future;
    auto test_job = std::make_shared<TestJob>(execute_future);
    const std::chrono::milliseconds job_cycle_time{ 100ms };
    const std::string job_name = "test_job";
    job_registry->addJob("test_job", test_job, fep3::ClockTriggeredJobConfiguration(job_cycle_time));
    auto data_triggered_job = std::make_shared<fep3::cpp::DataJob>("test_data_triggered_job", fep3::DataTriggeredJobConfiguration({"test_signal"}));
    data_triggered_job->addDataIn("test_signal", fep3::base::StreamTypeString());
    job_registry->addJob("test_data_triggered_job", data_triggered_job, data_triggered_job->getJobInfo().getConfig());

    _system.setSystemState(fep3::System::AggregatedState::running);

    execute_future.wait_for(10s);
    const auto participant_proxy = _system.getParticipant(_participant_name);

    auto health_service_proxy = participant_proxy.getRPCComponentProxy<fep3::rpc::IRPCHealthService>();

    // integration test RPC Service
    ASSERT_EQ(health_service_proxy->getRPCDefaultName(), rpc::getRPCDefaultName<fep3::rpc::IRPCHealthServiceDef>());
    ASSERT_EQ(health_service_proxy->getRPCIID(), rpc::getRPCIID<fep3::rpc::IRPCHealthServiceDef>());
    std::vector<JobHealthiness> jobs_healthiness = health_service_proxy->getHealth();

    ASSERT_EQ(jobs_healthiness.size(), 2);

    auto it_clock_trig_job = std::find_if(jobs_healthiness.begin(), jobs_healthiness.end(), [](const auto& e)
        {
            return e.job_name == "test_job";
        });
    ASSERT_NE(it_clock_trig_job, jobs_healthiness.end());

    const JobHealthiness& job_healthiness = *it_clock_trig_job;
    ASSERT_EQ(std::get <fep3::JobHealthiness::ClockTriggeredJobInfo>(job_healthiness.job_info).cycle_time, job_cycle_time);
    ASSERT_EQ(job_healthiness.job_name, job_name);
    ASSERT_GT(job_healthiness.simulation_time.count(), 0);
    ASSERT_EQ(job_healthiness.execute_data_in_error.error_count, 0);
    ASSERT_EQ(job_healthiness.execute_error.error_count, 0);
    ASSERT_EQ(job_healthiness.execute_data_out_error.error_count, 0);

    auto it_data_trig_job = std::find_if(jobs_healthiness.begin(), jobs_healthiness.end(), [](const auto& e)
        {
            return e.job_name == "test_data_triggered_job";
        });
    ASSERT_NE(it_data_trig_job, jobs_healthiness.end());

    const JobHealthiness& job_healthinessdata_trig_job = *it_data_trig_job;
    ASSERT_EQ(std::get <fep3::JobHealthiness::DataTriggeredJobInfo>(job_healthinessdata_trig_job.job_info).trigger_signals[0], "test_signal");
    ASSERT_EQ(job_healthinessdata_trig_job.job_name, "test_data_triggered_job");

    ASSERT_EQ(job_healthinessdata_trig_job.execute_data_in_error.error_count, 0);
    ASSERT_EQ(job_healthinessdata_trig_job.execute_error.error_count, 0);
    ASSERT_EQ(job_healthinessdata_trig_job.execute_data_out_error.error_count, 0);

    // test system getHealth
    log_future.wait_for(10s);
    std::map<std::string, ParticipantHealth> participants_health = _system.getParticipantsHealth();
    ASSERT_EQ(participants_health.size(), 1);
    ASSERT_EQ(participants_health.count(_participant_name), 1);
    const ParticipantHealth& participant_health = participants_health[_participant_name];
    ASSERT_EQ(participant_health.running_state, ParticipantRunningState::online);
    const auto& jobs_healthiness_system = participant_health.jobs_healthiness;
    ASSERT_EQ(jobs_healthiness_system.size(), 2);


    it_clock_trig_job = std::find_if(jobs_healthiness.begin(), jobs_healthiness.end(), [](const auto& e)
        {
            return e.job_name == "test_job";
        });
    ASSERT_NE(it_clock_trig_job, jobs_healthiness.end());
    const JobHealthiness& job_healthiness_system = *it_clock_trig_job;

    ASSERT_EQ(std::get <fep3::JobHealthiness::ClockTriggeredJobInfo>(job_healthiness_system.job_info).cycle_time, job_cycle_time);
    ASSERT_EQ(job_healthiness_system.job_name, job_name);
    ASSERT_GT(job_healthiness_system.simulation_time.count(), 0);
    ASSERT_EQ(job_healthiness_system.execute_data_in_error.error_count, 0);
    ASSERT_EQ(job_healthiness_system.execute_error.error_count, 0);
    ASSERT_EQ(job_healthiness_system.execute_data_out_error.error_count, 0);

    _system.setSystemState(fep3::System::AggregatedState::unloaded);
    _system.unregisterMonitoring(tem);
}

TEST_F(SystemHealthTest, TestParticpantGetHealthErrorJob)
{
    std::future<void> log_future;
    TestEventMonitor tem(log_future, "Received update event from ");

    //add the job and run the system
    using namespace std::chrono_literals;
    auto* job_registry = _participants[_participant_name]->_part.getComponent<fep3::IJobRegistry>();
    ASSERT_TRUE(job_registry);

    std::future<void> execute_future;
    auto test_job = std::make_shared<TestJobWithError>(execute_future);
    const std::chrono::milliseconds job_cycle_time{ 100ms };
    const std::string job_name = "test_job";
    job_registry->addJob("test_job", test_job, fep3::ClockTriggeredJobConfiguration(job_cycle_time));

    _system.setSystemState(fep3::System::AggregatedState::running);

    execute_future.wait_for(10s);

    // test system getHealth
    _system.registerMonitoring(tem);
    log_future.wait_for(10s);

    // test the errors
    std::map<std::string, ParticipantHealth> participants_health = _system.getParticipantsHealth();
    ASSERT_EQ(participants_health.size(), 1);
    ASSERT_EQ(participants_health.count(_participant_name), 1);
    const ParticipantHealth& participant_health = participants_health[_participant_name];
    ASSERT_EQ(participant_health.running_state, ParticipantRunningState::online);
    const auto& jobs_healthiness_system = participant_health.jobs_healthiness;
    ASSERT_EQ(jobs_healthiness_system.size(), 1);

    const JobHealthiness& job_healthiness_system = jobs_healthiness_system[0];
    ASSERT_EQ(std::get <fep3::JobHealthiness::ClockTriggeredJobInfo>(job_healthiness_system.job_info).cycle_time, job_cycle_time);
    ASSERT_EQ(job_healthiness_system.job_name, job_name);
    ASSERT_GT(job_healthiness_system.simulation_time.count(), 0);

    ASSERT_EQ(job_healthiness_system.execute_data_in_error.error_count, 2);
    ASSERT_GT(job_healthiness_system.execute_data_in_error.simulation_time.count(), 0);
    ASSERT_FEP3_RESULT(job_healthiness_system.execute_data_in_error.last_error.error_code, fep3::ERR_NOT_CONNECTED);
    ASSERT_NE(std::string(job_healthiness_system.execute_data_in_error.last_error.error_description).find("error 2 executeDataIn"),
        std::string::npos);

    ASSERT_EQ(job_healthiness_system.execute_error.error_count, 2);
    ASSERT_GT(job_healthiness_system.execute_error.simulation_time.count(), 0);
    ASSERT_FEP3_RESULT(job_healthiness_system.execute_error.last_error.error_code, fep3::ERR_BAD_DEVICE);
    ASSERT_NE(std::string(job_healthiness_system.execute_error.last_error.error_description).find("error 2 execute"),
        std::string::npos);

    ASSERT_EQ(job_healthiness_system.execute_data_out_error.error_count, 2);
    ASSERT_GT(job_healthiness_system.execute_data_out_error.simulation_time.count(), 0);
    ASSERT_FEP3_RESULT(job_healthiness_system.execute_data_out_error.last_error.error_code, fep3::ERR_FAILED);
    ASSERT_NE(std::string(job_healthiness_system.execute_data_out_error.last_error.error_description).find("error 2 executeDataOut"),
        std::string::npos);

    _system.setSystemState(fep3::System::AggregatedState::unloaded);
    _system.unregisterMonitoring(tem);
}

TEST_F(SystemHealthTest, TestParticpantGetHealthOnDeactivatedListener)
{
    auto [error, status] = _system.getHealthListenerRunningStatus();
    ASSERT_TRUE(status);
    ASSERT_FEP3_NOERROR(error);
    _system.setHealthListenerRunningStatus(false);
    auto [error_not_running, status_not_running] = _system.getHealthListenerRunningStatus();
    ASSERT_FALSE(status_not_running);
    ASSERT_FEP3_NOERROR(error_not_running);

    ASSERT_THROW(_system.getParticipantsHealth(), std::runtime_error);
}

TEST_F(SystemHealthTest, TestParticpantGetHealthOnDeactivatedListenerFromParticipantt)
{
    using namespace std::chrono_literals;
    // wait for a heartbeat
    std::future<void> log_future;
    TestEventMonitor tem(log_future, "Received update event from ");
    _system.registerMonitoring(tem);
    log_future.wait_for(10s);
    _system.unregisterMonitoring(tem);
    // shutdown the participant
    _participants[_participant_name]->_part_executor.shutdown();

    // set a very small timeout
    _system.setLivelinessTimeout(10ns);
    // wait much more than the timeout
    std::this_thread::sleep_for(1s);

    std::map<std::string, ParticipantHealth> participants_health = _system.getParticipantsHealth();
    ASSERT_EQ(participants_health.size(), 1);
    ASSERT_EQ(participants_health.count(_participant_name), 1);
    const ParticipantHealth& participant_health = participants_health[_participant_name];
    ASSERT_EQ(participant_health.running_state, ParticipantRunningState::offline);
}

TEST_F(SystemHealthTest, TestParticpantGetSetLivelinessTimeout)
{
    using namespace std::chrono_literals;
    ASSERT_EQ(_system.getLivelinessTimeout(), 20s);
    _system.setLivelinessTimeout(1s);
    ASSERT_EQ(_system.getLivelinessTimeout(), 1s);
}
