#include "system_discovery_helper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <queue>

using namespace ::testing;
using namespace std::literals::chrono_literals;

class SystemAccessMock :  public fep3::IServiceBus::ISystemAccess
{
public:
    using IParticipantServer = fep3::IRPCServer;
    using IParticipantRequester = fep3::IRPCRequester;

    MOCK_METHOD2(createServer, fep3::Result(const std::string&,const std::string&));
    MOCK_METHOD3(createServer, fep3::Result(const std::string&, const std::string&, bool));
    MOCK_METHOD0(releaseServer, void());
    MOCK_CONST_METHOD0(getServer, std::shared_ptr<IParticipantServer>());
    MOCK_CONST_METHOD1(getRequester, std::shared_ptr<IParticipantRequester>(const std::string&));
    MOCK_CONST_METHOD1(discover, std::multimap<std::string, std::string>(std::chrono::milliseconds));
    MOCK_CONST_METHOD0(getName, std::string());
    MOCK_METHOD1(registerUpdateEventSink, fep3::Result(fep3::IServiceBus::IServiceUpdateEventSink*));
    MOCK_METHOD1(deregisterUpdateEventSink, fep3::Result(fep3::IServiceBus::IServiceUpdateEventSink*));
};


struct DiscoverSystemParticipants : public testing::Test
{
public:
    std::multimap<std::string, std::string> saveAskedDuration(std::chrono::milliseconds duration)
    {
        _total_asked_discovery_duration += duration;
        return participants_to_return();
    }

protected:
    template<typename ...T>
    void performSimpleTest(bool assert_full_duration, T... arguments)
    {
        // how many times discovery is called does not matter for the test
        EXPECT_CALL(*_system_access_mock, discover(_)).WillRepeatedly(
            Invoke(this, &DiscoverSystemParticipants::saveAskedDuration));

        std::tie(_error_message, _discovered_participants) = 
            fep3::discoverSystemParticipants(*_system_access_mock, _discovery_duration, std::forward<T>(arguments)...);
        if (assert_full_duration)
        {
            ASSERT_EQ(_total_asked_discovery_duration, _discovery_duration);
        }
        else
        {
            ASSERT_LE(_total_asked_discovery_duration, _discovery_duration);
        }
    }

    void SetUp() override
    {
        _system_access_mock = std::make_unique<::testing::StrictMock<SystemAccessMock>>();
    }

    std::multimap<std::string, std::string> participants_to_return()
    {
        if (_participants_to_return.empty())
        {
            return{};
        }
        else
        {
            auto ret = _participants_to_return.front();
            _participants_to_return.pop();
            return ret;
        }
    }

    std::unique_ptr<::testing::StrictMock<SystemAccessMock>> _system_access_mock;
    std::chrono::milliseconds _discovery_duration{ 800 };
    std::chrono::milliseconds _total_asked_discovery_duration{0};
    std::optional<fep3::DiscoveredParticipants> _discovered_participants;
    std::queue<fep3::DiscoveredParticipants> _participants_to_return;
    std::string _error_message;
};

TEST_F(DiscoverSystemParticipants, simpleTest)
{
   EXPECT_CALL(*_system_access_mock, discover(_discovery_duration)).WillOnce(::testing::Return(std::multimap<std::string, std::string>{}));
   auto [error_message, discovered_participants] = fep3::discoverSystemParticipants(*_system_access_mock, _discovery_duration);
   ASSERT_TRUE(error_message.empty());
   ASSERT_TRUE(discovered_participants.has_value());
   ASSERT_TRUE(discovered_participants.value().empty());
}

TEST_F(DiscoverSystemParticipants, particiapantCount_DurationLessThanPollInterval)
{
    _discovery_duration = 800ms;

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(true, 2));
    ASSERT_FALSE(_discovered_participants.has_value());
}

TEST_F(DiscoverSystemParticipants, particiapantCount_DurationGreaterThanPollInterval)
{
    _discovery_duration = 2500ms;

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(true, 2));
    ASSERT_FALSE(_discovered_participants.has_value());
}

TEST_F(DiscoverSystemParticipants, particiapantCount_zeroDuration)
{
    _discovery_duration = 0ms;

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(true, 2));
    ASSERT_FALSE(_discovered_participants.has_value());
}

TEST_F(DiscoverSystemParticipants, particiapantCount_zeroParticipants)
{
    _discovery_duration = 0ms;

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(true, 0));

    ASSERT_TRUE(_discovered_participants.has_value());
    ASSERT_EQ(_discovered_participants.value().size(), 0);
}

TEST_F(DiscoverSystemParticipants, particiapantCount_found2Participants_expect2_onepass)
{
    _discovery_duration = 1800ms;
    std::multimap<std::string, std::string> system_participants = { {"part1" , "http://system1_url:9090"}, {"part2" , "http://system2_url:9091"} };
    _participants_to_return.push(system_participants);
   
    ASSERT_NO_FATAL_FAILURE(performSimpleTest(false, 2));

    ASSERT_TRUE(_discovered_participants.has_value());
    ASSERT_EQ(_discovered_participants.value().size(), 2);
    ASSERT_EQ(_discovered_participants, system_participants);
}

TEST_F(DiscoverSystemParticipants, particiapantCount_found2Participants_expect2_twopass)
{
    _discovery_duration = 1800ms;

    std::multimap<std::string, std::string> ret_parts_complete = { {"part1" , "http://system1_url:9090"}, {"part2" , "http://system2_url:9091"} };
    std::multimap<std::string, std::string> ret_parts_first = { {"http://system1_url:9090" , "part1"}};
    _participants_to_return.push(ret_parts_first);
    _participants_to_return.push(ret_parts_complete);

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(false, 2));

    ASSERT_TRUE(_discovered_participants.has_value());
    ASSERT_EQ(_discovered_participants.value().size(), 2);
    ASSERT_EQ(_discovered_participants, ret_parts_complete);
}

TEST_F(DiscoverSystemParticipants, particiapantCount_found1Participant_expect2)
{
    _discovery_duration = 1800ms;

    std::multimap<std::string, std::string> ret_parts_first = { {"part1" , "http://system1_url:9090"} };
    _participants_to_return.push(ret_parts_first);

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(true, 2));

    ASSERT_FALSE(_discovered_participants.has_value());
}

TEST_F(DiscoverSystemParticipants, particiapantCount_found3Participants_expect2)
{
    _discovery_duration = 1800ms;

    std::multimap<std::string, std::string> ret_parts_complete = { {"part1" , "http://system1_url:9090"}, {"part2" , "http://system2_url:9091"}, {"part2" , "http://system1_url:9090"} };
    std::multimap<std::string, std::string> ret_parts_first = { {"part1" , "http://system1_url:9090"} };
    _participants_to_return.push(ret_parts_first);
    _participants_to_return.push(ret_parts_complete);

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(false, 2));

    ASSERT_TRUE(_discovered_participants.has_value());
    ASSERT_EQ(_discovered_participants.value().size(), 3);
    ASSERT_EQ(_discovered_participants, ret_parts_complete);
}

TEST_F(DiscoverSystemParticipants, particiapantNames_found3Participants_expect2)
{
    _discovery_duration = 1800ms;

    std::multimap<std::string, std::string> ret_parts_complete = { {"part1" , "http://system1_url:9090"}, {"part2" , "http://system2_url:9091"}, {"part2" , "http://system1_url:9090"} };
    std::multimap<std::string, std::string> ret_parts_first = { {"part1" , "http://system1_url:9090"} };
    _participants_to_return.push(ret_parts_first);
    _participants_to_return.push(ret_parts_complete);

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(false, std::vector<std::string>{ "part1", "part2" }, false));

    ASSERT_TRUE(_discovered_participants.has_value());
    ASSERT_EQ(_discovered_participants.value().size(), 3);
    ASSERT_EQ(_discovered_participants, ret_parts_complete);
}

TEST_F(DiscoverSystemParticipants, particiapantNames_found3Participants_expect3_any_order)
{
    _discovery_duration = 1800ms;

    std::multimap<std::string, std::string> ret_parts_complete = { {"part1" , "http://system1_url:9090"}, {"part2" , "http://system1_url:9090"}, {"part3" , "http://system1_url:9090"} };

    _participants_to_return.push(ret_parts_complete);

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(false, std::vector<std::string>{ "part2" , "part1", "part3" },  false));

    ASSERT_TRUE(_discovered_participants.has_value());
    ASSERT_EQ(_discovered_participants.value().size(), 3);
    ASSERT_EQ(_discovered_participants, ret_parts_complete);
}

TEST_F(DiscoverSystemParticipants, particiapantCount_found3Participants_expect2_all_systems)
{
    _discovery_duration = 1800ms;

    std::multimap<std::string, std::string> ret_parts_complete = { {"part1@system1", "http://system1_url:9090"},
        {"part1@system1" , "http://system2_url:9091"},
        {"part2@system1" , "http://system1_url:9090"} };

    std::multimap<std::string, std::string> ret_parts_first = { {"part1@system1" , "http://system1_url:9090"} };
    _participants_to_return.push(ret_parts_first);
    _participants_to_return.push(ret_parts_complete);

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(false, std::vector<std::string>{ "part1", "part2" }, true));

    ASSERT_TRUE(_discovered_participants.has_value());
    ASSERT_EQ(_discovered_participants.value().size(), 3);
    ASSERT_EQ(_discovered_participants, ret_parts_complete);
}

TEST_F(DiscoverSystemParticipants, particiapantCount_parse_error2_all_systems)
{
    _discovery_duration = 1800ms;

    std::multimap<std::string, std::string> ret_parts_complete = { {"part1", "http://system1_url:9090"},
        {"part1@system1" , "http://system2_url:9091"},
        {"part2@system1" , "http://system1_url:9090"} };

    std::multimap<std::string, std::string> ret_parts_first = { {"part1@system1" , "http://system1_url:9090"} };
    _participants_to_return.push(ret_parts_first);
    _participants_to_return.push(ret_parts_complete);

    ASSERT_NO_FATAL_FAILURE(performSimpleTest(false, std::vector<std::string>{ "part1", "part2" }, true));

    ASSERT_FALSE(_discovered_participants.has_value());
    ASSERT_GT(_error_message.size(), 0);
}
