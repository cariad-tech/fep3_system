/**
 * Copyright 2024 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "participant_shutdown_listener.h"
#include <gtest/gtest.h>

using namespace ::testing;

struct ParticipantShutdownListenerTest : public testing::Test
{
    void participant_shutdown_notification(const std::string participant_name) {
        if (_participant_name == participant_name) {
            _shutdown_notification = true;
        }
    }

    const std::string _system_name = "sytem_name";
    const std::string _participant_name = "test_participant";
    fep3::ParticipantShutdownListener _shutdown_listener{ _system_name, [&](const std::string participant_name){participant_shutdown_notification(participant_name);}};
    bool _shutdown_notification = false;
};

TEST_F(ParticipantShutdownListenerTest, updateEventAlive)
{
    _shutdown_listener.updateEvent(
        fep3::IServiceBus::ServiceUpdateEvent{
            _participant_name ,
            _system_name,
            "url",
            fep3::IServiceBus::ServiceUpdateEventType::notify_alive});

    ASSERT_FALSE(_shutdown_notification);
}

TEST_F(ParticipantShutdownListenerTest, updateEventByeBye)
{
    _shutdown_listener.updateEvent(
        fep3::IServiceBus::ServiceUpdateEvent{
            _participant_name ,
            _system_name,
            "url",
            fep3::IServiceBus::ServiceUpdateEventType::notify_byebye });

    ASSERT_TRUE(_shutdown_notification);
}

TEST_F(ParticipantShutdownListenerTest, updateDifferentSystemName)
{
    _shutdown_listener.updateEvent(
        fep3::IServiceBus::ServiceUpdateEvent{
            _participant_name ,
            "another_system",
            "url",
            fep3::IServiceBus::ServiceUpdateEventType::notify_byebye });

    ASSERT_FALSE(_shutdown_notification);
}


