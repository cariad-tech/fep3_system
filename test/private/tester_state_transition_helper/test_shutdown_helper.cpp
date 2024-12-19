/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "shutdown_helper.h"

#include <fep3/fep3_errors.h>
#include <fep3/fep3_filesystem.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
using namespace fep3;
using namespace testing;

struct SystemLogger : public fep3::ISystemLogger {
    MOCK_METHOD(void,
                logProxyError,
                (fep3::LoggerSeverity, const std::string&, const std::string&, const std::string&),
                (const, override));

    MOCK_METHOD(void, log, (fep3::LoggerSeverity, const std::string&), (const, override));
};

struct DummyParticipant {
    std::string getName() const
    {
        return name;
    }

    fep3::Result operator()()
    {
        ++times_called;
        return result;
    }

    std::string name;
    fep3::Result result = {};
    size_t times_called = 0;
};

using DummyParticipantRange = std::vector<std::reference_wrapper<DummyParticipant>>;

struct ShutDownFunctor {
    MOCK_METHOD(void, shutDownCalled, (const std::string&), (const));

    fep3::Result operator()(std::reference_wrapper<DummyParticipant>& ref)
    {
        shutDownCalled(ref.get().getName());
        return ref.get()();
    }
};

DummyParticipantRange getParticipantProxyRange(std::vector<DummyParticipant>& participants)
{
    DummyParticipantRange range;

    std::transform(participants.begin(),
                   participants.end(),
                   std::back_insert_iterator(range),
                   [](DummyParticipant& proxy) { return std::ref(proxy); });

    return range;
}

fep3::Result dummyShutDown(std::reference_wrapper<DummyParticipant>& ref)
{
    return ref.get()();
}

TEST(ShutdownHelper, successfullShutdown_participantsRemoved)
{
    std::vector<DummyParticipant> participants{
        {"participant1"}, {"participant2"}, {"participant3"}};
    auto proxy_range = getParticipantProxyRange(participants);

    ::testing::StrictMock<ShutDownFunctor> shutdown;

    EXPECT_CALL(shutdown, shutDownCalled("participant1")).Times(1);
    EXPECT_CALL(shutdown, shutDownCalled("participant2")).Times(1);
    EXPECT_CALL(shutdown, shutDownCalled("participant3")).Times(1);

    shutdownParticipants(participants, proxy_range, shutdown, nullptr);

    ASSERT_TRUE(participants.empty());
}

TEST(ShutdownHelper, notSuccessfullShutdown_participantNotRemoved)
{
    using SystemLoggerMock = ::testing::NiceMock<SystemLogger>;
    auto logger = std::make_shared<SystemLoggerMock>();

    std::vector<DummyParticipant> participants{
        {"participant1"},
        {"participant2", CREATE_ERROR_DESCRIPTION(-35, "Error happened")},
        {"participant3"}};
    auto proxy_range = getParticipantProxyRange(participants);

    using testing::_;
    EXPECT_CALL(*logger, log(fep3::LoggerSeverity::error, HasSubstr("participant2")))
        .Times(AnyNumber());
    EXPECT_CALL(*logger, log(fep3::LoggerSeverity::info, _)).Times(AnyNumber());
    EXPECT_CALL(*logger, log(fep3::LoggerSeverity::fatal, _)).Times(AnyNumber());

    ASSERT_THROW(shutdownParticipants(participants, proxy_range, dummyShutDown, logger),
                 std::runtime_error);

    ASSERT_EQ(participants.size(), static_cast<size_t>(1));
    ASSERT_EQ(participants[0].name, "participant2");
}

TEST(ShutdownHelper, successfullShutdownParticipantSubset_participantsRemoved)
{
    std::vector<DummyParticipant> participants{
        {"participant1"}, {"participant2"}, {"participant3"}};
    auto proxy_range = getParticipantProxyRange(participants);
    // do not shut down participant 2
    proxy_range.erase(std::next(proxy_range.begin()));
    ::testing::StrictMock<ShutDownFunctor> shutdown;

    EXPECT_CALL(shutdown, shutDownCalled("participant1")).Times(1);
    EXPECT_CALL(shutdown, shutDownCalled("participant3")).Times(1);

    shutdownParticipants(participants, proxy_range, shutdown, nullptr);

    ASSERT_EQ(participants.size(), static_cast<size_t>(1));
    ASSERT_EQ(participants[0].name, "participant2");
}

TEST(ShutdownHelper, unsuccessfullShutdownParticipantSubset_participantNotRemoved)
{
    std::vector<DummyParticipant> participants{
        {"participant1", CREATE_ERROR_DESCRIPTION(-35, "Error happened")},
        {"participant2"},
        {"participant3"}};
    auto proxy_range = getParticipantProxyRange(participants);
    // do not shut down participant 2
    proxy_range.erase(std::next(proxy_range.begin()));
    ::testing::StrictMock<ShutDownFunctor> shutdown;

    EXPECT_CALL(shutdown, shutDownCalled("participant1")).Times(1);
    EXPECT_CALL(shutdown, shutDownCalled("participant3")).Times(1);

    ASSERT_THROW(shutdownParticipants(participants, proxy_range, shutdown, nullptr),
                 std::runtime_error);

    ASSERT_EQ(participants.size(), static_cast<size_t>(2));
    ASSERT_THAT(participants, Contains(Field(&DummyParticipant::name, "participant1")));
    ASSERT_THAT(participants, Contains(Field(&DummyParticipant::name, "participant2")));
}
