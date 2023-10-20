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


 /**
 * Test Case:   TestSystemLibrary
 * Test ID:     1.0
 * Test Title:  FEP System Library base test
 * Description: Test if controlling a test system is possible
 * Strategy:    Invoke Controller and issue commands
 * Passed If:   no errors occur
 * Ticket:      -
 * Requirement: -
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fep_system/fep_system.h>
#include <string.h>
#include <functional>
#include <fep_test_common.h>
#include <a_util/logging.h>
#include <a_util/process.h>
#include <a_util/system.h>
#include <boost/range/irange.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/algorithm/cxx11/is_permutation.hpp>

#include <fep3/components/configuration/configuration_service_intf.h>
#include <fep3/components/clock/clock_service_intf.h>
#include <fep3/components/clock_sync/clock_sync_service_intf.h>
#include <fep3/components/scheduler/scheduler_service_intf.h>
#include <fep3/base/properties/propertynode_helper.h>
#include "component_file_path_from_env_var.h"

void addingTestParticipants(fep3::System& sys)
{
    sys.add("part1");
    sys.add("part2");
    auto part1 = sys.getParticipant("part1");
    part1.setInitPriority(1);
    part1.setStartPriority(11);
    part1.setAdditionalInfo("part1", "part1_value");
    part1.setAdditionalInfo("part1_2", "part1_value_2");

    auto part2 = sys.getParticipant("part2");
    part2.setInitPriority(2);
    part2.setStartPriority(22);
    part2.setAdditionalInfo("part2", "part2_value");
}

bool hasTestParticipants(fep3::System& sys)
{
    try
    {
        auto part1 = sys.getParticipant("part1");
        if (!(part1.getInitPriority() == 1))
        {
            return false;
        }
        if (!(part1.getStartPriority() == 11))
        {
            return false;
        }
        if (!(part1.getAdditionalInfo("part1", "") == "part1_value"))
        {
            return false;
        }
        if (!(part1.getAdditionalInfo("part1_2", "") == "part1_value_2"))
        {
            return false;
        }

        auto part2 = sys.getParticipant("part2");
        if (!(part2.getInitPriority() == 2))
        {
            return false;
        }
        if (!(part2.getStartPriority() == 22))
        {
            return false;
        }
        if (!(part2.getAdditionalInfo("part2", "") == "part2_value"))
        {
            return false;
        }
        return true;
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
}


/*
This class basically blocks the first participant_block_count-1 elements that calls stateIn() and unblocks the
participant_block_count -th element that calls stateOut() or if the timeout expired. The flag _unblockedInTime tells if the
unblocking happened because of the timeout or because the stateOut was called.
*/
struct StateTransitionControl
{
    MOCK_METHOD(void, StateInMock, ());
    MOCK_METHOD(void, StateOutMock, ());

    StateTransitionControl(int participant_block_count)
        : _participant_block_count(participant_block_count)
    {}

    void stateIn(const std::string& part_name)
    {
        StateInMock();
        std::unique_lock<std::mutex> lock(_mutex);
        _state_in_calls.push_back(part_name);
        // all elements except the last in the state transition wait
        if ((_state_in_calls.size() % _participant_block_count) != 0)
        {
            _unblockedInTime = _cv.wait_for
            (lock
                , _timeout
                , [this]()
                {
                    return _notified;
                }
            );
        }
        // last element in the state transition continues
    }

    void stateOut(const std::string& part_name)
    {
        // first coming here unlocks the others wating in stateIn()
        std::unique_lock<std::mutex> lock(_mutex);
        _state_out_calls.push_back(part_name);
        if ((_state_out_calls.size() % _participant_block_count) == 1)
        {
            // simulate a delay in state transition of the last element
            std::this_thread::sleep_for(_state_transition_delay);

            _notified = true;
            _cv.notify_all();
        }
        StateOutMock();
    }

    void reset()
    {
        _state_in_calls.clear();
        _state_out_calls.clear();
        _notified = false;
        _unblockedInTime = false;
    }

    bool wasUnblockedInTime()
    {
        return _unblockedInTime;
    }

    std::vector<std::string> getStateInCallers() const
    {
        return _state_in_calls;
    }

    std::vector<std::string> getStateOutCallers() const
    {
        return _state_out_calls;
    }

    void set_participant_block_count(int participant_block_count)
    {
        _participant_block_count = participant_block_count;
    }

private:
    std::mutex _mutex;
    std::condition_variable _cv;
    bool _notified{ false };
    const std::chrono::seconds _timeout{ 10 };
    const std::chrono::seconds _state_transition_delay{ 1 };
    bool _unblockedInTime{ false };
    int _participant_block_count;
    std::vector<std::string> _state_in_calls;
    std::vector<std::string> _state_out_calls;
};

struct BlockingElement : public fep3::core::ElementBase
{
    BlockingElement(StateTransitionControl& state_transition_control, std::string part_name)
        : fep3::core::ElementBase(makePlatformDepName("Testelement"), "3.0")
        , _state_transition_control(state_transition_control)
        , _part_name(part_name)
    {
    }

    fep3::Result initialize() override
    {
        return  callStateTransition();
    }

    fep3::Result run() override
    {
        return  callStateTransition();
    }
private:

    fep3::Result callStateTransition()
    {
        _state_transition_control.stateIn(_part_name);
        _state_transition_control.stateOut(_part_name);
        return  {};
    }

    StateTransitionControl& _state_transition_control;
    const std::string _part_name;
};

/*
Factory class that injects StateTransitionControl into the BlockingElement object
*/
class BlockingElementFactory : public fep3::base::IElementFactory
{
public:
    /**
     * CTOR for the Element factory which is able to create elements with a specified
     *
     * @returns Shared pointer to the created element.
     */
    BlockingElementFactory(StateTransitionControl& tc, std::string part_name)
        : _tc(tc)
        , _part_name(part_name)
    {
    }
    /**
     * Creates the element
     *
     * @returns unique pointer to the created element.
     */
    std::unique_ptr<fep3::base::IElement> createElement(const fep3::arya::IComponents& /*components*/) const override
    {
        return std::unique_ptr<fep3::base::IElement>(new BlockingElement(_tc, _part_name));
    }
private:
    StateTransitionControl& _tc;
    const std::string _part_name;
};


inline TestParticipants createTestBlockingParticipants(
    const std::vector<std::string>& participant_names,
    const std::string& system_name,
    StateTransitionControl& tc)
{
    using namespace fep3::core;
    TestParticipants test_parts;
    std::for_each
    (participant_names.begin()
        , participant_names.end()
        , [&](const std::string & name)
        {
            auto part = fep3::base::createParticipant(name, "1.0", system_name, std::make_shared<BlockingElementFactory>(tc, name));
            auto part_exec = std::make_unique<PartStruct>(std::move(part));
            part_exec->_part_executor.exec();
            test_parts[name].reset(part_exec.release());
        }
    );

    return std::move(test_parts);
}

struct NotInitializableableElement : public fep3::core::ElementBase
{
    NotInitializableableElement(std::string part_name)
        : fep3::core::ElementBase(makePlatformDepName("Testelement"), "3.0")
        , _part_name(part_name)
    {
    }

    fep3::Result initialize() override
    {
        RETURN_ERROR_DESCRIPTION(fep3::ERR_FAILED, "'initialize' transition failed");
    }

    const std::string _part_name;
};

class NotInitializableElementFactory : public fep3::base::IElementFactory
{
public:
    NotInitializableElementFactory(std::string part_name)
        : _part_name(part_name)
    {
    }

    std::unique_ptr<fep3::base::IElement> createElement(const fep3::arya::IComponents& /*components*/) const override
    {
        return std::unique_ptr<fep3::base::IElement>(new NotInitializableableElement(_part_name));
    }
private:
    const std::string _part_name;
};

inline TestParticipants createNotInitializableParticipants(
    const std::vector<std::string>& participant_names,
    const std::string& system_name)
{
    using namespace fep3::core;
    TestParticipants test_parts;
    std::for_each
    (participant_names.begin()
        , participant_names.end()
        , [&](const std::string& name)
        {
            auto part = fep3::base::createParticipant(name, "1.0", system_name, std::make_shared<NotInitializableElementFactory>(name));
            auto part_exec = std::make_unique<PartStruct>(std::move(part));
            part_exec->_part_executor.exec();
            test_parts[name].reset(part_exec.release());
        }
    );

    return test_parts;
}

 /**
 * @brief Test teh CTORs of the fep::system
 * @req_id
 */

TEST(SystemLibrary, SystemCtors)
{
    using namespace fep3;
    {
        preloadServiceBusPlugin();
        //default CTOR will have a system_name which is empty
        System test_system_default;
        addingTestParticipants(test_system_default);
        ASSERT_EQ(test_system_default.getSystemName(), "");
        ASSERT_TRUE(hasTestParticipants(test_system_default));
    }

    {
        //spec CTOR will have a system_name which is empty
        System test_system("system_name");
        ASSERT_EQ(test_system.getSystemName(), "system_name");
        ASSERT_FALSE(hasTestParticipants(test_system));
    }

    {
        //move CTOR will have a system_name which is empty
        System test_system("system_name");
        addingTestParticipants(test_system);
        System moved_sys(std::move(test_system));
        ASSERT_EQ(moved_sys.getSystemName(), "system_name");
        ASSERT_TRUE(hasTestParticipants(moved_sys));
    }

    {
        System test_system("system_name");
        addingTestParticipants(test_system);
        //copy CTOR will have a system_name which is empty
        System copied_sys(test_system);
        ASSERT_EQ(copied_sys.getSystemName(), "system_name");
        ASSERT_EQ(test_system.getSystemName(), "system_name");
        ASSERT_TRUE(hasTestParticipants(test_system));
        ASSERT_TRUE(hasTestParticipants(copied_sys));
    }

    {
        System test_system("orig");
        addingTestParticipants(test_system);
        //copy CTOR will have a system_name which is empty
        System copied_assigned_sys = test_system;
        ASSERT_EQ(test_system.getSystemName(), "orig");
        ASSERT_EQ(copied_assigned_sys.getSystemName(), "orig");
ASSERT_TRUE(hasTestParticipants(test_system));
ASSERT_TRUE(hasTestParticipants(copied_assigned_sys));
    }

    {
    //moved CTOR will have a system_name which is empty
    System test_system("orig");
    addingTestParticipants(test_system);
    System moved_assigned_sys = std::move(test_system);
    ASSERT_EQ(moved_assigned_sys.getSystemName(), "orig");
    ASSERT_TRUE(hasTestParticipants(moved_assigned_sys));
    }
}

TEST(SystemLibrary, ComponentsFileFromEnvVariable)
{
    // set the env variable
    const std::string system_components_file_path_environment_variable = "FEP3_SYSTEM_COMPONENTS_FILE_PATH";
    a_util::process::setEnvVar(system_components_file_path_environment_variable, component_file_path_from_env_var);

    //RUN THE TEST, ServiceBusWrapper called in system constructor
    fep3::System test_system("orig");

    //cleanup
    a_util::process::setEnvVar(system_components_file_path_environment_variable, "");
}

class TestEventMonitor : public fep3::IEventMonitor
{
public:
    TestEventMonitor() : _logger_name_filter("")
    {
    }
    TestEventMonitor(const std::string& logger_name_filter) : _logger_name_filter(logger_name_filter)
    {

    }
    void onLog(std::chrono::milliseconds,
        fep3::LoggerSeverity severity_level,
        const std::string& participant_name,
        const std::string& logger_name, //depends on the Category ...
        const std::string& message) override
    {
        if (!_logger_name_filter.empty())
        {
            if (_logger_name_filter != logger_name)
            {
                return;
            }
        }
        std::unique_lock<std::mutex> lk(_cv_m);
        _severity_level = severity_level;
        _participant_name = participant_name;
        _logger_name = logger_name;
        _message = message;
        _messages.push_back(message);
        _done = true;
    }

    bool waitForDone(timestamp_t timeout_ms = 4000)
    {
        if (_done)
        {
            _done = false;
            return true;
        }
        int i = 0;
        auto single_wait = timeout_ms / 10;
        while (!_done && i < 10)
        {
            a_util::system::sleepMilliseconds(static_cast<uint32_t>(single_wait));
            ++i;
        }
        if (_done)
        {
            _done = false;
            return true;
        }
        return false;
    }

    bool waitFor(const std::string& message, timestamp_t timeout_ms = 4000)
    {
        auto begin_time = a_util::system::getCurrentMilliseconds();
        {
            auto single_wait = timeout_ms / 10;
            while (timeout_ms > (a_util::system::getCurrentMilliseconds() - begin_time))
            {
                std::string current_message;
                {
                    std::unique_lock<std::mutex> lk(_cv_m);
                    current_message = _message;
                }
                if (current_message.find(message) != std::string::npos)
                {
                    return true;
                }
                a_util::system::sleepMilliseconds(static_cast<uint32_t>(single_wait));
            }
        }
        std::string current_message_final;
        {
            std::unique_lock<std::mutex> lk(_cv_m);
            current_message_final = _message;
        }
        return (current_message_final.find(message) != std::string::npos);
    }

    bool checkForLogMessage(const std::string& message_to_search_for)
    {
        for (const auto& message : _messages)
        {
            if (message.find(message_to_search_for) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    fep3::Category _category;
    fep3::LoggerSeverity _severity_level;
    std::string _participant_name;
    std::string _message;
    std::vector<std::string> _messages;
    std::string _logger_name;
    std::vector<fep3::rpc::ParticipantState> _states;
    std::string _new_name;
    std::string _logger_name_filter;
private:
    std::mutex _cv_m;
    std::atomic_bool _done{ false };
};

/**
* @brief It's tested that a system containing standalone participants can not be started
* @req_id FEPSDK-2129
*/
/* readd ... when paticiapnts without a statemachine can be added
AEV_TEST(SystemLibrary, SystemWithStandaloneParticipantCanNotBeStarted, "")
{
    const std::string sys_name = makePlatformDepName("system_under_test");
    const auto participant_names = std::vector<std::string>{ "participant1", "participant2" };
    const auto test_parts = createTestParticipants(participant_names, sys_name);

    ASSERT_EQ(
        modules.begin()->second->GetPropertyTree()->SetPropertyValue(FEP_STM_STANDALONE_PATH, true),
        a_util::result::Result());

    fep::System my_system = fep::System("my_system");
    EXPECT_NO_THROW(my_system.add(participant_names));

    /// start fails because standalone module is part of fep system
    EXPECT_THROW(my_system.start(), std::runtime_error);
}*/

struct SystemLibraryWithTestSystem: public ::testing::Test {
    SystemLibraryWithTestSystem()
        : my_sys(sys_name)
    {
    }

    void SetUp() override {
        my_sys.add(part_name_1);
        my_sys.add(part_name_2);
    }

    const std::string sys_name = makePlatformDepName("system_under_test");
    const std::string part_name_1 = "participant1";
    const std::string part_name_2 = "participant2";
    const std::vector<std::string> participant_names{ part_name_1, part_name_2 };
    const TestParticipants test_parts = createTestParticipants(participant_names, sys_name);
    fep3::System my_sys;
};

TEST_F(SystemLibraryWithTestSystem, TestConfigureSystemOK)
{
        my_sys.add("Participant2");
        my_sys.remove("Participant2");
        my_sys.getParticipant(part_name_1).setInitPriority(1);
        auto p1 = my_sys.getParticipant(part_name_1);
        ASSERT_EQ(part_name_1, p1.getName());
        my_sys.clear();

        auto ps = my_sys.getParticipants();
        ASSERT_TRUE(ps.empty());
        ASSERT_EQ(my_sys.getSystemName(), sys_name);
}

TEST(SystemLibrary, TestConfigureSystemNOK)
{
    const std::string sys_name = makePlatformDepName("system_under_test");
    {
        fep3::System my_sys(sys_name);

        bool caught = false;
        try
        {
            my_sys.getParticipant("does_not_exist");
        }
        catch (std::runtime_error e)
        {
            auto msg = e.what();
            ASSERT_TRUE(a_util::strings::isEqual(msg, "No Participant with the name does_not_exist found"));
            caught = true;
        }
        ASSERT_TRUE(caught);

        ASSERT_TRUE(fep3::rpc::arya::IRPCParticipantStateMachine::State::undefined == my_sys.getSystemState()._state);

    }
}

TEST_F(SystemLibraryWithTestSystem, TestControlSystemOK)
{
    using namespace std::literals::chrono_literals;
    my_sys = fep3::discoverSystem(sys_name, participant_names, 4000ms);

    my_sys.setInitAndStartPolicy(fep3::System::InitStartExecutionPolicy::sequential, 4);
    // start system
    my_sys.load();
    my_sys.initialize();
    my_sys.start();
    auto p1 = my_sys.getParticipant(part_name_1);

    //check if inti priority are taking in concern
    p1.setInitPriority(23);
    ASSERT_EQ(p1.getInitPriority(), 23);
    auto check_p1 = my_sys.getParticipant(part_name_1);
    ASSERT_EQ(check_p1.getInitPriority(), 23);

    //check additional info
    check_p1.setAdditionalInfo("my_value", "this is the information i want");
    ASSERT_EQ(check_p1.getAdditionalInfo("my_value", ""), "this is the information i want");
    ASSERT_EQ(check_p1.getAdditionalInfo("my_value_does_not_exist_using_default", "does not exist"), "does not exist");


    auto state1 = p1.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState();
    auto p2 = my_sys.getParticipant(part_name_2);
    auto state2 = p2.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState();
    ASSERT_EQ(state1, fep3::rpc::ParticipantState::running);
    ASSERT_EQ(state2, fep3::rpc::ParticipantState::running);

    // stop system
    my_sys.stop();
    state1 = p1.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState();
    state2 = p2.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState();
    ASSERT_EQ(state1, fep3::rpc::ParticipantState::initialized);
    ASSERT_EQ(state2, fep3::rpc::ParticipantState::initialized);

    // shutdown event should be react
    ASSERT_ANY_THROW(
        my_sys.shutdown();
    );

    state1 = p1.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState();
    state2 = p2.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState();
    ASSERT_EQ(state1, fep3::rpc::ParticipantState::initialized);
    ASSERT_EQ(state2, fep3::rpc::ParticipantState::initialized);

    // shutdown system
    my_sys.deinitialize();
    my_sys.unload();
    my_sys.shutdown();
    state1 = p1.getRPCComponentProxyByIID<fep3::rpc::IRPCParticipantStateMachine>()->getState();
    state2 = p2.getRPCComponentProxyByIID<fep3::rpc::IRPCParticipantStateMachine>()->getState();
    ASSERT_EQ(state1, fep3::rpc::ParticipantState::unreachable);
    ASSERT_EQ(state2, fep3::rpc::ParticipantState::unreachable);
}

using State = fep3::rpc::ParticipantState;

namespace {
    void transitionToState(fep3::RPCComponent<fep3::rpc::IRPCParticipantStateMachine>& state_machine, State target_state)
    {
        try {
            const auto state = state_machine->getState();
            switch (state) {
            case State::paused:
                FAIL() << "State::paused reached";
            case State::undefined:
                FAIL() << "State::undefined reached";
            case State::unreachable:
                FAIL() << "State::unreachable reached";
            case State::unloaded:
                if (state == target_state)
                    return;
                else {

                    state_machine->load();
                    transitionToState(state_machine, target_state);
                    return;
                }
            case State::loaded:
                if (state == target_state)
                    return;
                else if (state < target_state)
                {
                    state_machine->initialize();
                    transitionToState(state_machine, target_state);
                    return;
                }
                else {
                    state_machine->unload();
                    transitionToState(state_machine, target_state);
                    return;
                }
            case State::initialized:
                if (state == target_state)
                    return;
                else if (state < target_state)
                {
                    state_machine->start();
                    transitionToState(state_machine, target_state);
                    return;
                }
                else {
                    state_machine->deinitialize();
                    transitionToState(state_machine, target_state);
                    return;
                }
            case State::running:
                if (state == target_state)
                    return;
                else {
                    state_machine->stop();
                    transitionToState(state_machine, target_state);
                    return;
                }
            }
        }
        catch (const std::exception& exception)
        {
            FAIL() << exception.what();
        }
    }
}

TEST_F(SystemLibraryWithTestSystem, TestStateTransitionHeterogenous)
{
        auto participant1 = my_sys.getParticipant(part_name_1);
        auto state_machine1 = participant1.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>();
        auto participant2 = my_sys.getParticipant(part_name_2);
        auto state_machine2 = participant2.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>();

        auto check_heterogeneous_transition = [&](State participant_1_state, State participant_2_state, State system_target_state)
        {
            transitionToState(state_machine1, participant_1_state);
            transitionToState(state_machine2, participant_2_state);
            ASSERT_EQ(state_machine1->getState(), participant_1_state);
            ASSERT_EQ(state_machine2->getState(), participant_2_state);

            fep3::SystemState system_state;
            fep3::ParticipantStates participant_states;
            try {
                my_sys.setSystemState(system_target_state);
                system_state = my_sys.getSystemState();
                participant_states = my_sys.getParticipantStates();
            }
            catch (const std::exception& exception) {
                FAIL() << exception.what();
            }

            ASSERT_EQ(state_machine1->getState(), system_target_state);
            ASSERT_EQ(state_machine2->getState(), system_target_state);
            ASSERT_TRUE(system_state._homogeneous);
            ASSERT_EQ(participant_states[part_name_1], system_target_state);
            ASSERT_EQ(participant_states[part_name_2], system_target_state);
        };

        check_heterogeneous_transition(State::unloaded, State::loaded, State::unloaded);
        check_heterogeneous_transition(State::unloaded, State::loaded, State::loaded);
        check_heterogeneous_transition(State::unloaded, State::loaded, State::initialized);
        check_heterogeneous_transition(State::unloaded, State::loaded, State::running);
        check_heterogeneous_transition(State::unloaded, State::initialized, State::unloaded);
        check_heterogeneous_transition(State::unloaded, State::initialized, State::loaded);
        check_heterogeneous_transition(State::unloaded, State::initialized, State::initialized);
        check_heterogeneous_transition(State::unloaded, State::initialized, State::running);
        check_heterogeneous_transition(State::unloaded, State::running, State::unloaded);
        check_heterogeneous_transition(State::unloaded, State::running, State::loaded);
        check_heterogeneous_transition(State::unloaded, State::running, State::initialized);
        check_heterogeneous_transition(State::unloaded, State::running, State::running);
        check_heterogeneous_transition(State::loaded, State::initialized, State::unloaded);
        check_heterogeneous_transition(State::loaded, State::initialized, State::loaded);
        check_heterogeneous_transition(State::loaded, State::initialized, State::initialized);
        check_heterogeneous_transition(State::loaded, State::initialized, State::running);
        check_heterogeneous_transition(State::loaded, State::running, State::unloaded);
        check_heterogeneous_transition(State::loaded, State::running, State::loaded);
        check_heterogeneous_transition(State::loaded, State::running, State::initialized);
        check_heterogeneous_transition(State::loaded, State::running, State::running);
        check_heterogeneous_transition(State::initialized, State::running, State::unloaded);
        check_heterogeneous_transition(State::initialized, State::running, State::loaded);
        check_heterogeneous_transition(State::initialized, State::running, State::initialized);
        check_heterogeneous_transition(State::initialized, State::running, State::running);
}

TEST(SystemLibrary, TestControlSystemNOK)
{
    const std::string sys_name = makePlatformDepName("system_under_test");

    {
        fep3::System my_sys(sys_name);
        // test with element that doesn't exist
        TestEventMonitor tem;
        my_sys.registerMonitoring(tem);
        my_sys.add("does_not_exist");
        bool caught = false;
        try
        {
            my_sys.setSystemState({ fep3::SystemAggregatedState::running }, std::chrono::milliseconds(500));
        }
        catch (std::runtime_error e)
        {
            std::string msg = e.what();
            ASSERT_STREQ("Participant does_not_exist is unreachable", msg.c_str());
            caught = true;
        }
        ASSERT_TRUE(caught);
        caught = false;
        ASSERT_TRUE(tem.waitForDone());

        // test with empty system
        my_sys.clear();
        try
        {
            my_sys.setSystemState(fep3::SystemAggregatedState::running, std::chrono::milliseconds(500));
        }
        catch (std::runtime_error e)
        {
            caught = true;
        }
        // exception is thrown ... because the current state change is not possible and invalid
        ASSERT_TRUE(caught);
        // but logging message
        ASSERT_TRUE(tem.waitForDone());
        ASSERT_TRUE(tem._message.find("No participant has a statemachine") != std::string::npos);
        ASSERT_TRUE(tem._severity_level == fep3::LoggerSeverity::error);
        my_sys.unregisterMonitoring(tem);
    }
}

struct NotInitializableTestSystem : public ::testing::Test {
    NotInitializableTestSystem()
        : my_sys(sys_name)
    {
    }

    void SetUp() override {
        my_sys.add(part_name_1);
        my_sys.add(part_name_2);
        test_parts = createNotInitializableParticipants(participant_names, sys_name);
    }

    const std::string sys_name = makePlatformDepName("system_under_test");
    const std::string part_name_1 = "participant1";
    const std::string part_name_2 = "participant2";
    const std::vector<std::string> participant_names{ part_name_1, part_name_2 };
    TestParticipants test_parts;
    fep3::System my_sys;
};

TEST_F(NotInitializableTestSystem, TestStartSystemNOK)
{
    TestEventMonitor tem;
    my_sys.registerMonitoring(tem);
    auto p1 = my_sys.getParticipant(part_name_1);
    auto p2 = my_sys.getParticipant(part_name_2);

    // Trying to start a system of participants which fail to initialize shall result in a system of state loaded
    {
        EXPECT_THROW(my_sys.setSystemState({ fep3::SystemAggregatedState::running }, std::chrono::milliseconds(500)),
            std::runtime_error);

        ASSERT_TRUE(tem.waitForDone());

        ASSERT_TRUE(tem.checkForLogMessage("System loaded successfully"));
        ASSERT_TRUE(tem.checkForLogMessage("Participant participant1 threw exception"));
        ASSERT_TRUE(tem.checkForLogMessage("Participant participant2 threw exception"));
        ASSERT_TRUE(tem.checkForLogMessage("could not be initialized successfully and remains in state 'loaded'"));
        ASSERT_TRUE(tem.checkForLogMessage("System could not be initialized in a homogeneous way"));
        ASSERT_TRUE(tem.checkForLogMessage("No participant has a statemachine"));

        ASSERT_EQ(p1.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::loaded);
        ASSERT_EQ(p2.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::loaded);
    }

    // The system shall be unloaded successfully
    {
        EXPECT_NO_THROW(my_sys.setSystemState({ fep3::SystemAggregatedState::unloaded }, std::chrono::milliseconds(500)));

        ASSERT_TRUE(tem.waitForDone());

        ASSERT_TRUE(tem.checkForLogMessage("System unloaded successfully"));

        ASSERT_EQ(p1.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::unloaded);
        ASSERT_EQ(p2.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::unloaded);
    }

    my_sys.unregisterMonitoring(tem);
}

TEST_F(NotInitializableTestSystem, TestStartSystemPartiallyOK)
{
        TestEventMonitor tem;
        my_sys.registerMonitoring(tem);
        
        // test with valid participants within the system which shall reach the corresponding state
        const std::string part_name_3 = "participant3";
        const auto test_parts_valid = createTestParticipants({ part_name_3 }, sys_name);
        my_sys.add(part_name_3);
        auto p1 = my_sys.getParticipant(part_name_1);
        auto p2 = my_sys.getParticipant(part_name_2);
        auto p3 = my_sys.getParticipant(part_name_3);

        // Trying to start a system of participants which fail to initialize and a valid participant
        // shall result in a system of heterogeneous state loaded/running
        {
            EXPECT_NO_THROW(my_sys.setSystemState({ fep3::SystemAggregatedState::running }, std::chrono::milliseconds(500)));

            ASSERT_TRUE(tem.waitForDone());

            ASSERT_TRUE(tem.checkForLogMessage("System loaded successfully"));
            ASSERT_TRUE(tem.checkForLogMessage("Participant participant1 threw exception"));
            ASSERT_TRUE(tem.checkForLogMessage("Participant participant2 threw exception"));
            ASSERT_TRUE(tem.checkForLogMessage("could not be initialized successfully and remains in state 'loaded'"));
            ASSERT_TRUE(tem.checkForLogMessage("System could not be initialized in a homogeneous way"));

            ASSERT_EQ(p1.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::loaded);
            ASSERT_EQ(p2.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::loaded);
            ASSERT_EQ(p3.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::running);
        }

        // The system shall be unloaded successfully
        {
            EXPECT_NO_THROW(my_sys.setSystemState({ fep3::SystemAggregatedState::unloaded }, std::chrono::milliseconds(500)));

            ASSERT_TRUE(tem.waitForDone());

            ASSERT_TRUE(tem.checkForLogMessage("System unloaded successfully"));

            ASSERT_EQ(p1.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::unloaded);
            ASSERT_EQ(p2.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::unloaded);
            ASSERT_EQ(p3.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::rpc::ParticipantState::unloaded);
        }

        my_sys.unregisterMonitoring(tem);
}

TEST(SystemLibrary, TestMonitorSystemOK)
{
    const std::string sys_name = makePlatformDepName("system_under_test");
    const std::string part_name_1 = "participant1";

    const auto participant_names = std::vector<std::string>{ part_name_1 };

    // testing state monitor and log function
    {
        using namespace std::literals::chrono_literals;
        auto test_parts = createTestParticipants(participant_names, sys_name);
        TestEventMonitor tem;
        fep3::System my_sys = fep3::discoverSystem(sys_name, participant_names, 4000ms);
        my_sys.setInitAndStartPolicy(fep3::System::InitStartExecutionPolicy::sequential, 4);
        my_sys.registerMonitoring(tem);

        my_sys.load();

        my_sys.setSystemState(fep3::System::AggregatedState::running);

        EXPECT_TRUE(tem.waitFor("Successfully start", 5000));

        //this test is only possible if we have the logging system reactivatd
        ASSERT_TRUE(tem._participant_name.find(part_name_1) != std::string::npos);
        ASSERT_TRUE(tem._message.find("Successfully start") != std::string::npos);

        my_sys.setSystemState(fep3::System::AggregatedState::initialized);

        EXPECT_TRUE(tem.waitFor("Successfully stop", 5000));
        ASSERT_TRUE(tem._message.find("Successfully stop") != std::string::npos);

        // unregister monitoring is important!
        my_sys.unregisterMonitoring(tem);

    }
}

TEST_F(SystemLibraryWithTestSystem, TestGetAClock)
{
    // testing state monitor and log function
    {
        using namespace std::literals::chrono_literals;
        using namespace std::chrono;
        TestEventMonitor tem;
        my_sys.registerMonitoring(tem);

        my_sys.load();

        ASSERT_NO_THROW(
            my_sys.configureTiming3ClockSyncOnlyInterpolation(part_name_1, "");
        );

        auto masters = my_sys.getCurrentTimingMasters();
        EXPECT_EQ(masters, std::vector<std::string>{ part_name_1 });
        
        // configure participants to log only information relevant for the test
        {
            auto logging_service_part2 = my_sys.getParticipant(part_name_2).getRPCComponentProxyByIID<fep3::rpc::IRPCLoggingService>();
            ASSERT_TRUE(logging_service_part2);
            ASSERT_TRUE(logging_service_part2->setLoggerFilter("Testelement.element", { fep3::LoggerSeverity::off, {} }));
            ASSERT_TRUE(logging_service_part2->setLoggerFilter("participant", { fep3::LoggerSeverity::off, {} }));
            ASSERT_TRUE(logging_service_part2->setLoggerFilter("slave_clock.clock_sync_service.component", { fep3::LoggerSeverity::debug, {"console", "rpc"} }));

            auto logging_service_part1 = my_sys.getParticipant(part_name_1).getRPCComponentProxyByIID<fep3::rpc::IRPCLoggingService>();
            ASSERT_TRUE(logging_service_part1);
            ASSERT_TRUE(logging_service_part1->setLoggerFilter("Testelement.element", { fep3::LoggerSeverity::off, {} }));
            ASSERT_TRUE(logging_service_part1->setLoggerFilter("participant", { fep3::LoggerSeverity::off, {} }));
        }

        ASSERT_NO_THROW(
            my_sys.setSystemState(fep3::System::AggregatedState::loaded);
            my_sys.setSystemState(fep3::System::AggregatedState::initialized);
        );

        const auto participant_start_delay = 1s;
        // we wait for the second participant to start to see whether the time sync works
        {
            ASSERT_NO_THROW(
                my_sys.getParticipant(part_name_1).getRPCComponentProxyByIID<fep3::rpc::IRPCParticipantStateMachine>()->start();
                std::this_thread::sleep_for(participant_start_delay);
                my_sys.getParticipant(part_name_2).getRPCComponentProxyByIID<fep3::rpc::IRPCParticipantStateMachine>()->start();
            );
        }

        // we wait for the synchronization of the timing client until we check the rpc time calls
        ASSERT_TRUE(tem.waitFor("Retrieved master time"));

        auto part1 = my_sys.getParticipant(part_name_1).getRPCComponentProxyByIID<fep3::rpc::IRPCClockService>();
        ASSERT_TRUE(part1);
        auto time_part1 = part1->getTime("");
        auto part2 = my_sys.getParticipant(part_name_2).getRPCComponentProxyByIID<fep3::rpc::IRPCClockService>();
        ASSERT_TRUE(part2);
        auto time_part2 = part2->getTime("");

        auto diff = time_part2 - time_part1;
        EXPECT_LT(diff, fep3::Duration(participant_start_delay).count())
            << "Difference of timing client time and timing master time exceeded '1s': " << duration_cast<milliseconds>(nanoseconds(diff)).count();

        ASSERT_NO_THROW(
            my_sys.setSystemState(fep3::System::AggregatedState::unloaded);
        );

        // unregister monitoring is important!
        my_sys.unregisterMonitoring(tem);

    }
}

TEST_F(SystemLibraryWithTestSystem, getTimingPropertiesRealTime)
{
    using namespace std::literals::chrono_literals;
    my_sys = fep3::discoverSystem(sys_name, participant_names, 4000ms);

    my_sys.load();

    auto props_part = my_sys.getParticipant(part_name_1).getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>()->getProperties("/");

    // equivalent of configureTiming3NoMaster
    const std::string string_type = fep3::base::PropertyType<std::string>::getTypeName();

    props_part->setProperty(FEP3_CLOCK_SERVICE_MAIN_CLOCK, FEP3_CLOCK_LOCAL_SYSTEM_REAL_TIME, string_type);
    props_part->setProperty(FEP3_SCHEDULER_SERVICE_SCHEDULER, FEP3_SCHEDULER_CLOCK_BASED, string_type);

    auto timing_properties = my_sys.getTimingProperties();
    ASSERT_EQ(timing_properties.size(), 2u);

    auto participant_iterator = timing_properties.begin();
    EXPECT_EQ(participant_iterator->first, "participant1");

    const fep3::arya::IProperties& properties = *participant_iterator->second;
    auto property_names = properties.getPropertyNames();
    std::vector<std::string> expected_property_names = { FEP3_MAIN_CLOCK_PROPERTY, FEP3_SCHEDULER_PROPERTY };
    std::sort(property_names.begin(), property_names.end());
    std::sort(expected_property_names.begin(), expected_property_names.end());
    EXPECT_EQ(property_names, expected_property_names);

    EXPECT_EQ(properties.getProperty(FEP3_MAIN_CLOCK_PROPERTY), FEP3_CLOCK_LOCAL_SYSTEM_REAL_TIME);
    EXPECT_EQ(properties.getPropertyType(FEP3_MAIN_CLOCK_PROPERTY), string_type);

    EXPECT_EQ(properties.getProperty(FEP3_SCHEDULER_PROPERTY), FEP3_SCHEDULER_CLOCK_BASED);
    EXPECT_EQ(properties.getPropertyType(FEP3_SCHEDULER_PROPERTY), string_type);
}

/**
 * Test new members of convenience API (get/setParticipantProperty, get/setParticipantState)
 *
 * @req_id          FEPSDK-3266, FEPSDK-3349
 * @testData        none
 * @testType        functional
 * @precondition    none
 * @postcondition   none
 * @expectedResult  no deviations
 */TEST_F(SystemLibraryWithTestSystem, convenienceAPI)
{
    using namespace std::literals::chrono_literals;
    my_sys = fep3::discoverSystem(sys_name, participant_names, 4000ms);

    my_sys.load();

    auto part = my_sys.getParticipant(part_name_1);
    auto props_part = part.getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>()->getProperties("/");

    // equivalent of configureTiming3NoMaster
    const std::string string_type = fep3::base::PropertyType<std::string>::getTypeName();

    // set property to a defined value
    props_part->setProperty(FEP3_CLOCK_SERVICE_MAIN_CLOCK, FEP3_CLOCK_LOCAL_SYSTEM_REAL_TIME, string_type);

    // change property via convenience API and check result
    my_sys.setParticipantProperty(part_name_1, FEP3_CLOCK_SERVICE_MAIN_CLOCK, FEP3_CLOCK_LOCAL_SYSTEM_SIM_TIME);
    EXPECT_EQ(my_sys.getParticipantProperty(part_name_1, FEP3_CLOCK_SERVICE_MAIN_CLOCK), FEP3_CLOCK_LOCAL_SYSTEM_SIM_TIME);

    // check participant for state change via convenience API
    EXPECT_EQ(my_sys.getParticipantState(part_name_1), fep3::SystemAggregatedState::loaded);
    my_sys.unload();
    EXPECT_EQ(my_sys.getParticipantState(part_name_1), fep3::SystemAggregatedState::unloaded);
    my_sys.setParticipantState(part_name_1, fep3::SystemAggregatedState::unreachable);
    EXPECT_EQ(part.getRPCComponentProxy<fep3::rpc::IRPCParticipantStateMachine>()->getState(), fep3::SystemAggregatedState::unreachable);
    EXPECT_EQ(my_sys.getParticipants().size(), 1);
 }

TEST(SystemLibrary, getTimingPropertiesAFAP)
{
    const std::string sys_name = makePlatformDepName("system_under_test");
    const std::string master_part_name = "timing_master";
    const std::string slave_part_name = "timing_slave";

    const auto participant_names = std::vector<std::string>{ master_part_name, slave_part_name };

    auto test_parts = createTestParticipants(participant_names, sys_name);
    using namespace std::literals::chrono_literals;
    fep3::System my_sys = fep3::discoverSystem(sys_name, participant_names, 4000ms);
    my_sys.load();

    auto props_master = my_sys.getParticipant(master_part_name).getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>()->getProperties("/");
    auto props_slave = my_sys.getParticipant(slave_part_name).getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>()->getProperties("/");

    const std::string string_type = fep3::base::PropertyType<std::string>::getTypeName();
    const std::string double_type = fep3::base::PropertyType<double>::getTypeName();
    const std::string int32_type = fep3::base::PropertyType<int32_t>::getTypeName();
    const std::string int64_type = fep3::base::PropertyType<int64_t>::getTypeName();

    // equivalent of configureTiming3AFAP("timing_master", "50")
    props_master->setProperty(FEP3_CLOCKSYNC_SERVICE_CONFIG_TIMING_MASTER, master_part_name, string_type);
    props_master->setProperty(FEP3_SCHEDULER_SERVICE_SCHEDULER, FEP3_SCHEDULER_CLOCK_BASED, string_type);
    props_master->setProperty(FEP3_CLOCK_SERVICE_MAIN_CLOCK, FEP3_CLOCK_LOCAL_SYSTEM_SIM_TIME, string_type);
    props_master->setProperty(FEP3_CLOCK_SERVICE_CLOCK_SIM_TIME_TIME_FACTOR, "0.0", double_type);
    props_master->setProperty(FEP3_CLOCK_SERVICE_CLOCK_SIM_TIME_STEP_SIZE, "50000000", int64_type);

    props_slave->setProperty(FEP3_CLOCKSYNC_SERVICE_CONFIG_TIMING_MASTER, master_part_name, string_type);
    props_slave->setProperty(FEP3_SCHEDULER_SERVICE_SCHEDULER, FEP3_SCHEDULER_CLOCK_BASED, string_type);
    props_slave->setProperty(FEP3_CLOCK_SERVICE_MAIN_CLOCK, FEP3_CLOCK_SLAVE_MASTER_ONDEMAND_DISCRETE, string_type);

    auto timing_properties = my_sys.getTimingProperties();
    ASSERT_EQ(timing_properties.size(), 2u);

    auto master_iterator = timing_properties.begin();
    EXPECT_EQ(master_iterator->first, master_part_name);

    const fep3::arya::IProperties& master_properties = *master_iterator->second;
    auto master_property_names = master_properties.getPropertyNames();
    std::vector<std::string> expected_master_property_names = { FEP3_TIMING_MASTER_PROPERTY, FEP3_SCHEDULER_PROPERTY,
        FEP3_MAIN_CLOCK_PROPERTY, FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY, FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY,
        FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY, FEP3_TIME_UPDATE_TIMEOUT_PROPERTY };

    std::sort(master_property_names.begin(), master_property_names.end());
    std::sort(expected_master_property_names.begin(), expected_master_property_names.end());
    EXPECT_EQ(master_property_names, expected_master_property_names);

    EXPECT_EQ(master_properties.getProperty(FEP3_TIMING_MASTER_PROPERTY), master_part_name);
    EXPECT_EQ(master_properties.getPropertyType(FEP3_TIMING_MASTER_PROPERTY), string_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_SCHEDULER_PROPERTY), FEP3_SCHEDULER_CLOCK_BASED);
    EXPECT_EQ(master_properties.getPropertyType(FEP3_SCHEDULER_PROPERTY), string_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_MAIN_CLOCK_PROPERTY), FEP3_CLOCK_LOCAL_SYSTEM_SIM_TIME);
    EXPECT_EQ(master_properties.getPropertyType(FEP3_MAIN_CLOCK_PROPERTY), string_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY), "0.0");
    EXPECT_EQ(master_properties.getPropertyType(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY), double_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY), "50000000");
    EXPECT_EQ(master_properties.getPropertyType(FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY), int64_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY), std::to_string(FEP3_SLAVE_SYNC_CYCLE_TIME_DEFAULT_VALUE));
    EXPECT_EQ(master_properties.getPropertyType(FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY), int64_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_TIME_UPDATE_TIMEOUT_PROPERTY), std::to_string(FEP3_TIME_UPDATE_TIMEOUT_DEFAULT_VALUE));
    EXPECT_EQ(master_properties.getPropertyType(FEP3_TIME_UPDATE_TIMEOUT_PROPERTY), int64_type);

    auto slave_iterator = std::next(timing_properties.begin());
    EXPECT_EQ(slave_iterator->first, slave_part_name);

    const fep3::arya::IProperties& slave_properties = *slave_iterator->second;
    auto slave_property_names = slave_properties.getPropertyNames();
    std::vector<std::string> expected_slave_property_names = { FEP3_TIMING_MASTER_PROPERTY, FEP3_SCHEDULER_PROPERTY,
        FEP3_MAIN_CLOCK_PROPERTY, FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY, FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY,
        FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY, FEP3_TIME_UPDATE_TIMEOUT_PROPERTY };

    std::sort(slave_property_names.begin(), slave_property_names.end());
    std::sort(expected_slave_property_names.begin(), expected_slave_property_names.end());
    EXPECT_EQ(slave_property_names, expected_slave_property_names);

    EXPECT_EQ(slave_properties.getProperty(FEP3_TIMING_MASTER_PROPERTY), master_part_name);
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_TIMING_MASTER_PROPERTY), string_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_SCHEDULER_PROPERTY), FEP3_SCHEDULER_CLOCK_BASED);
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_SCHEDULER_PROPERTY), string_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_MAIN_CLOCK_PROPERTY), FEP3_CLOCK_SLAVE_MASTER_ONDEMAND_DISCRETE);
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_MAIN_CLOCK_PROPERTY), string_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY), std::to_string(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_DEFAULT_VALUE));
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY), double_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY), std::to_string(FEP3_CLOCK_SIM_TIME_STEP_SIZE_DEFAULT_VALUE));
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY), int64_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY), std::to_string(FEP3_SLAVE_SYNC_CYCLE_TIME_DEFAULT_VALUE));
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY), int64_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_TIME_UPDATE_TIMEOUT_PROPERTY), std::to_string(FEP3_TIME_UPDATE_TIMEOUT_DEFAULT_VALUE));
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_TIME_UPDATE_TIMEOUT_PROPERTY), int64_type);
}

TEST(SystemLibrary, getTimingPropertiesDiscrete)
{
    const std::string sys_name = makePlatformDepName("tested_system");
    const std::string master_part_name = "timing_master";
    const std::string slave_part_name = "timing_slave";

    const auto participant_names = std::vector<std::string>{ master_part_name, slave_part_name };

    auto test_parts = createTestParticipants(participant_names, sys_name);
    using namespace std::literals::chrono_literals;
    fep3::System my_sys = fep3::discoverSystem(sys_name, participant_names, 4000ms);
    my_sys.load();

    auto props_master = my_sys.getParticipant(master_part_name).getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>()->getProperties("/");
    auto props_slave = my_sys.getParticipant(slave_part_name).getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>()->getProperties("/");

    const std::string string_type = fep3::base::PropertyType<std::string>::getTypeName();
    const std::string double_type = fep3::base::PropertyType<double>::getTypeName();
    const std::string int32_type = fep3::base::PropertyType<int32_t>::getTypeName();
    const std::string int64_type = fep3::base::PropertyType<int64_t>::getTypeName();

    // equivalent of configureTiming3ClockSyncOnlyDiscrete(master_part_name, "20")
    props_master->setProperty(FEP3_CLOCKSYNC_SERVICE_CONFIG_TIMING_MASTER, master_part_name, string_type);
    props_master->setProperty(FEP3_SCHEDULER_SERVICE_SCHEDULER, FEP3_SCHEDULER_CLOCK_BASED, string_type);
    props_master->setProperty(FEP3_CLOCK_SERVICE_MAIN_CLOCK, FEP3_CLOCK_LOCAL_SYSTEM_REAL_TIME, string_type);

    props_slave->setProperty(FEP3_CLOCKSYNC_SERVICE_CONFIG_TIMING_MASTER, master_part_name, string_type);
    props_slave->setProperty(FEP3_SCHEDULER_SERVICE_SCHEDULER, FEP3_SCHEDULER_CLOCK_BASED, string_type);
    props_slave->setProperty(FEP3_CLOCK_SERVICE_MAIN_CLOCK, FEP3_CLOCK_SLAVE_MASTER_ONDEMAND_DISCRETE, string_type);
    props_slave->setProperty(FEP3_CLOCKSYNC_SERVICE_CONFIG_SLAVE_SYNC_CYCLE_TIME,"20000000" , int64_type);

    auto timing_properties = my_sys.getTimingProperties();
    ASSERT_EQ(timing_properties.size(), 2u);

    auto master_iterator = timing_properties.begin();
    EXPECT_EQ(master_iterator->first, master_part_name);

    const fep3::arya::IProperties& master_properties = *master_iterator->second;
    auto master_property_names = master_properties.getPropertyNames();
    std::vector<std::string> expected_master_property_names = { FEP3_TIMING_MASTER_PROPERTY, FEP3_SCHEDULER_PROPERTY,
        FEP3_MAIN_CLOCK_PROPERTY, FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY, FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY,
        FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY, FEP3_TIME_UPDATE_TIMEOUT_PROPERTY };

    std::sort(master_property_names.begin(), master_property_names.end());
    std::sort(expected_master_property_names.begin(), expected_master_property_names.end());
    EXPECT_EQ(master_property_names, expected_master_property_names);

    EXPECT_EQ(master_properties.getProperty(FEP3_TIMING_MASTER_PROPERTY), master_part_name);
    EXPECT_EQ(master_properties.getPropertyType(FEP3_TIMING_MASTER_PROPERTY), string_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_SCHEDULER_PROPERTY), FEP3_SCHEDULER_CLOCK_BASED);
    EXPECT_EQ(master_properties.getPropertyType(FEP3_SCHEDULER_PROPERTY), string_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_MAIN_CLOCK_PROPERTY), FEP3_CLOCK_LOCAL_SYSTEM_REAL_TIME);
    EXPECT_EQ(master_properties.getPropertyType(FEP3_MAIN_CLOCK_PROPERTY), string_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY), std::to_string(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_DEFAULT_VALUE));
    EXPECT_EQ(master_properties.getPropertyType(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY), double_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY), std::to_string(FEP3_CLOCK_SIM_TIME_STEP_SIZE_DEFAULT_VALUE));
    EXPECT_EQ(master_properties.getPropertyType(FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY), int64_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY), std::to_string(FEP3_SLAVE_SYNC_CYCLE_TIME_DEFAULT_VALUE));
    EXPECT_EQ(master_properties.getPropertyType(FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY), int64_type);

    EXPECT_EQ(master_properties.getProperty(FEP3_TIME_UPDATE_TIMEOUT_PROPERTY), std::to_string(FEP3_TIME_UPDATE_TIMEOUT_DEFAULT_VALUE));
    EXPECT_EQ(master_properties.getPropertyType(FEP3_TIME_UPDATE_TIMEOUT_PROPERTY), int64_type);

    auto slave_iterator = std::next(timing_properties.begin());
    EXPECT_EQ(slave_iterator->first, slave_part_name);

    const fep3::arya::IProperties& slave_properties = *slave_iterator->second;
    auto slave_property_names = slave_properties.getPropertyNames();
    std::vector<std::string> expected_slave_property_names = { FEP3_TIMING_MASTER_PROPERTY, FEP3_SCHEDULER_PROPERTY,
        FEP3_MAIN_CLOCK_PROPERTY, FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY, FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY,
        FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY, FEP3_TIME_UPDATE_TIMEOUT_PROPERTY };

    std::sort(slave_property_names.begin(), slave_property_names.end());
    std::sort(expected_slave_property_names.begin(), expected_slave_property_names.end());
    EXPECT_EQ(slave_property_names, expected_slave_property_names);

    EXPECT_EQ(slave_properties.getProperty(FEP3_TIMING_MASTER_PROPERTY), master_part_name);
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_TIMING_MASTER_PROPERTY), string_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_SCHEDULER_PROPERTY), FEP3_SCHEDULER_CLOCK_BASED);
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_SCHEDULER_PROPERTY), string_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_MAIN_CLOCK_PROPERTY), FEP3_CLOCK_SLAVE_MASTER_ONDEMAND_DISCRETE);
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_MAIN_CLOCK_PROPERTY), string_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY), std::to_string(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_DEFAULT_VALUE));
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_PROPERTY), double_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY), std::to_string(FEP3_CLOCK_SIM_TIME_STEP_SIZE_DEFAULT_VALUE));
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_CLOCK_SIM_TIME_STEP_SIZE_PROPERTY), int64_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY), "20000000");
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_SLAVE_SYNC_CYCLE_TIME_PROPERTY), int64_type);

    EXPECT_EQ(slave_properties.getProperty(FEP3_TIME_UPDATE_TIMEOUT_PROPERTY), std::to_string(FEP3_TIME_UPDATE_TIMEOUT_DEFAULT_VALUE));
    EXPECT_EQ(slave_properties.getPropertyType(FEP3_TIME_UPDATE_TIMEOUT_PROPERTY), int64_type);
}

class TestTransitionPolicy
    : public ::testing::Test
{

protected:
    using string_vec_it = std::vector<std::string>::const_iterator;

    TestTransitionPolicy()
        // cast to int beacause Times expects an int
        : _participant_count(static_cast<int>(_participant_names.size()))
        , _tc(_participant_count)
        , _my_sys(_sys_name)
    {}

    void SetUp()
    {
        _testParts = createTestBlockingParticipants(_participant_names, _sys_name, _tc);
        using namespace std::literals::chrono_literals;
        _my_sys = fep3::discoverSystem(_sys_name, _participant_names, 4000ms);
    }

    void performCheck(const boost::iterator_range <string_vec_it>& expected_participant_names, const boost::iterator_range <string_vec_it>& actual_participant_names)
    {
        ASSERT_EQ(expected_participant_names.size(), actual_participant_names.size()) << "Not all expected participants did the state transition";
        ASSERT_TRUE(boost::algorithm::is_permutation(expected_participant_names, actual_participant_names.begin())) << "Not all expected participants did the state transition";
    }

    void checkPrioritisedStateTransition(const boost::iterator_range <string_vec_it>& expected_participant_names, size_t index_start, size_t index_end)
    {
        performCheck(expected_participant_names, boost::adaptors::slice(_tc.getStateInCallers(), index_start, index_end));
        performCheck(expected_participant_names, boost::adaptors::slice(_tc.getStateOutCallers(), index_start, index_end));
    }

    const std::string _sys_name = makePlatformDepName("system_under_test");
    const std::vector<std::string> _participant_names{ "participant1", "participant2", "participant3", "participant4" };
    const int _participant_count;
    TestParticipants _testParts;
    ::testing::StrictMock<StateTransitionControl> _tc;
    fep3::System _my_sys;
};

TEST_F(TestTransitionPolicy, testSetGet)
{
    // default is parallel
    auto policy_pair = _my_sys.getInitAndStartPolicy();
    ASSERT_EQ(policy_pair.first, fep3::System::InitStartExecutionPolicy::parallel) << "Default execution policy is not parallel";
    ASSERT_EQ(policy_pair.second, 4) << "Default execution policy pool size is not 4";

    // change policy and pool size
    ASSERT_NO_THROW(_my_sys.setInitAndStartPolicy(fep3::System::InitStartExecutionPolicy::sequential, 15));
    policy_pair = _my_sys.getInitAndStartPolicy();
    ASSERT_EQ(policy_pair.first, fep3::System::InitStartExecutionPolicy::sequential) << "Default execution policy not changed to sequential";
    ASSERT_EQ(policy_pair.second, 15) << "Execution policy pool size was not changed to 15";

    ASSERT_THROW(_my_sys.setInitAndStartPolicy(fep3::System::InitStartExecutionPolicy::sequential, 0), std::runtime_error)
        << "Setting zero pool size did not throw";
}

TEST_F(TestTransitionPolicy, testSequentialInit)
{
    {
        ::testing::InSequence sequence;
        _my_sys.setInitAndStartPolicy(fep3::System::InitStartExecutionPolicy::sequential, 4);

        // this will fail if the StateIn is called by the next element before the previous calls StateOut
        for (int i = 0; i < _participant_count; ++i)
        {
            EXPECT_CALL(_tc, StateInMock()).Times(1);
            EXPECT_CALL(_tc, StateOutMock()).Times(1);
        }

        _my_sys.load();
        _my_sys.initialize();
        EXPECT_TRUE(_tc.wasUnblockedInTime()) << "Not both elements were initialized in parallel";

        _tc.reset();
        // this will fail if the StateIn is called by the next element before the previous calls StateOut
        for (int i = 0; i < _participant_count; ++i)
        {
            EXPECT_CALL(_tc, StateInMock()).Times(1);
            EXPECT_CALL(_tc, StateOutMock()).Times(1);
        }

        _my_sys.start();
        EXPECT_TRUE(_tc.wasUnblockedInTime()) << "Not both elements were started in parallel";
    }

    // shutdown system
    _my_sys.stop();
    _my_sys.deinitialize();
    _my_sys.unload();
    _my_sys.shutdown();
}

TEST_F(TestTransitionPolicy, testParallelInit)
{
    {
        ::testing::InSequence sequence;
        // this will fail if the StateOut is called by one element before all have called StateIn
        EXPECT_CALL(_tc, StateInMock()).Times(_participant_count);
        EXPECT_CALL(_tc, StateOutMock()).Times(_participant_count);

        _my_sys.load();
        _my_sys.initialize();
        EXPECT_TRUE(_tc.wasUnblockedInTime()) << "Not all elements were initialized in parallel";


        _tc.reset();
        // this will fail if the StateOut is called by one element before all have called StateIn
        EXPECT_CALL(_tc, StateInMock()).Times(_participant_count);
        EXPECT_CALL(_tc, StateOutMock()).Times(_participant_count);
        _my_sys.start();
        EXPECT_TRUE(_tc.wasUnblockedInTime()) << "Not all elements were started in parallel";
    }

    // shutdown system
    _my_sys.stop();
    _my_sys.deinitialize();
    _my_sys.unload();
    _my_sys.shutdown();
}

TEST_F(TestTransitionPolicy, testParallelInitDiferrentPrio)
{
    const int low_prio_part_count = 2;
    const int high_prio_part_count = static_cast<int>(_participant_names.size()) - low_prio_part_count;
    auto begin_low_prio = _participant_names.cbegin();
    // get the two ranges of participant names with different priorities
    boost::iterator_range<decltype(begin_low_prio)> _low_prio_participants = boost::adaptors::slice(_participant_names, 0, low_prio_part_count);
    boost::iterator_range<decltype(begin_low_prio)> _high_prio_participants = boost::adaptors::slice(_participant_names, low_prio_part_count, _participant_names.size());

    // after the hi prio transition we have to block the next low prio
    _tc.set_participant_block_count(high_prio_part_count);

    // set the prios for the participants
    const int32_t low_prio = 1;
    const int32_t high_prio = 10;
    auto set_part_prio = [&](const std::string part_name, int32_t prio)
    {
        auto part = _my_sys.getParticipant(part_name);
        part.setInitPriority(prio);
        part.setStartPriority(prio);
    };

    boost::range::for_each(_low_prio_participants, std::bind(set_part_prio, std::placeholders::_1, low_prio));
    boost::range::for_each(_high_prio_participants, std::bind(set_part_prio, std::placeholders::_1, high_prio));

    ::testing::InSequence sequence;
    // the high prio participants are initialized first
    EXPECT_CALL(_tc, StateInMock()).Times(high_prio_part_count);
    EXPECT_CALL(_tc, StateOutMock()).Times(high_prio_part_count);
    // the high prio participants are initialized second
    EXPECT_CALL(_tc, StateInMock()).Times(low_prio_part_count);
    EXPECT_CALL(_tc, StateOutMock()).Times(low_prio_part_count);

    _my_sys.load();
    _my_sys.initialize();
    // Make check after initialized is complete
    EXPECT_TRUE(_tc.wasUnblockedInTime()) << "Not all elements were initialized";
    // Now check that the state transition happened in the correct priority
    EXPECT_NO_FATAL_FAILURE(checkPrioritisedStateTransition(_high_prio_participants, 0, high_prio_part_count))
        << "Hi prio participants not initialized in correct order";
    EXPECT_NO_FATAL_FAILURE(checkPrioritisedStateTransition(_low_prio_participants, high_prio_part_count, _participant_names.size()))
        << "Low prio participants not initialized in correct order";

    // reset for the next state transition
    _tc.reset();
    // the high prio participants are started first
    EXPECT_CALL(_tc, StateInMock()).Times(high_prio_part_count);
    EXPECT_CALL(_tc, StateOutMock()).Times(high_prio_part_count);
    // the low prio participants are started second
    EXPECT_CALL(_tc, StateInMock()).Times(low_prio_part_count);
    EXPECT_CALL(_tc, StateOutMock()).Times(low_prio_part_count);
    _my_sys.start();
    // Make check after start is complete
    EXPECT_TRUE(_tc.wasUnblockedInTime()) << "Not all elements were started";
    // Now check that the state transition happened in the correct priority
    EXPECT_NO_FATAL_FAILURE(checkPrioritisedStateTransition(_high_prio_participants, 0, high_prio_part_count))
        << "Hi prio participants not started in correct order";
    EXPECT_NO_FATAL_FAILURE(checkPrioritisedStateTransition(_low_prio_participants, high_prio_part_count, _participant_names.size()))
        << "Low prio participants not started in correct order";

    // shutdown system
    _my_sys.stop();
    _my_sys.deinitialize();
    _my_sys.unload();
    _my_sys.shutdown();
}
