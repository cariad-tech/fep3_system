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
#include <fep_system/fep_system.h>
#include <string>
#include <fep_test_common.h>
#include <a_util/system.h>

namespace
{
    bool logEntriesContainString(const std::vector<std::string> &strings_to_check, const std::vector<std::string> &expected_sub_string)
    {
        for (const std::string &log_message : strings_to_check)
        {
            if (!std::any_of(expected_sub_string.begin(), expected_sub_string.end(),
                [&log_message](const std::string &expected)
                {
                    return log_message.find(expected) != std::string::npos;
                })
                )
                {
                    return false;
                }
        }

        return true;
    }
}

struct TestEventMonLog : public fep3::IEventMonitor
{
    TestEventMonLog()
    {
        _logcount = 0;
    }
    ~TestEventMonLog()
    {
    }
    void onLog(std::chrono::milliseconds /*log_time*/,
               fep3::LoggerSeverity severity_level,
               const std::string &participant_name,
               const std::string &logger_name,
               const std::string &message) override
    {
        if (_logEverything ||
            (logger_name == "Testelement.element"))
        {
            _messages[severity_level].push_back(logger_name + "---" + participant_name + "---" + message);
            _logcount++;
        }
    }
    std::map<fep3::LoggerSeverity, std::vector<std::string>> _messages;
    std::atomic<int32_t> _logcount;
    bool _logEverything = false;
};

TEST(SystemLibrary, TestParticpantInfo)
{
    auto sys_name = makePlatformDepName("test_system");
    const auto participant_names = std::vector<std::string>{"test_participant1", "test_participant2"};
    const TestParticipants test_parts = createTestParticipants(participant_names, sys_name);

    using namespace std::literals::chrono_literals;

    fep3::System my_system = fep3::discoverSystem(sys_name, participant_names, 10000ms);
    TestEventMonLog test_log;
    my_system.registerMonitoring(test_log);

    ASSERT_TRUE(my_system.getParticipant("test_participant2").loggingRegistered());
    ASSERT_TRUE(my_system.getParticipant("test_participant1").loggingRegistered());

    my_system.load();
    my_system.initialize();

    int try_count = 20;
    while (test_log._logcount < 16 && try_count > 0)
    {
        a_util::system::sleepMilliseconds(200);
        try_count--;
    }
    my_system.unregisterMonitoring(test_log);

    EXPECT_EQ(test_log._messages[fep3::LoggerSeverity::debug].size(), 0);
    EXPECT_EQ(test_log._messages[fep3::LoggerSeverity::info].size(), 4);
    EXPECT_EQ(test_log._messages[fep3::LoggerSeverity::warning].size(), 4);
    EXPECT_EQ(test_log._messages[fep3::LoggerSeverity::error].size(), 4);
    EXPECT_EQ(test_log._messages[fep3::LoggerSeverity::fatal].size(), 4);

    std::vector<std::string> expectedLogContent = {"initializing", "loading"};

    EXPECT_TRUE(logEntriesContainString(test_log._messages[fep3::LoggerSeverity::warning], expectedLogContent));
    EXPECT_TRUE(logEntriesContainString(test_log._messages[fep3::LoggerSeverity::error], expectedLogContent));
    EXPECT_TRUE(logEntriesContainString(test_log._messages[fep3::LoggerSeverity::fatal], expectedLogContent));

    expectedLogContent.push_back(sys_name);
    logEntriesContainString(test_log._messages[fep3::LoggerSeverity::info], expectedLogContent);
}

/* Tests the case that the Event Monitoring is registered before the participant
    is completely ready. See FEPSDK-2982 and SUP-4001
*/
TEST(SystemLibrary, TestEaryEventMonitorRegister)
{
    auto sys_name = makePlatformDepName("test_system");
    const std::string not_ready_participant{"test_participant1"};

    fep3::System my_system(sys_name);
    my_system.add(not_ready_participant);

    ASSERT_FALSE(my_system.getParticipant(not_ready_participant).loggingRegistered());

    TestEventMonLog test_log;
    test_log._logEverything = true;
    my_system.registerMonitoring(test_log);

    EXPECT_EQ(test_log._messages[fep3::LoggerSeverity::warning].size(), 1);

    std::vector<std::string> expected_log_content = {"Participant test_participant1 has no registered logging interface"};

    ASSERT_TRUE(logEntriesContainString(test_log._messages[fep3::LoggerSeverity::warning], expected_log_content));
}
