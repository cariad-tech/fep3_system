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
 * Test Title:  FEP System Data Registry test
 * Description: Test data registry interface
 * Strategy:    Invoke Controller and issue commands
 * Passed If:   no errors occur
 * Ticket:      -
 * Requirement: -
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fep_system/fep_system.h>
#include "participant_configuration_helper.hpp"
#include "fep_test_common.h"
#include <fep3/base/stream_type/stream_type.h>
#include <fep3/base/properties/propertynode.h>
#include <fep_system/mock/event_monitor.h>
#include "../../../../../function/utils/common/gtest_asserts.h"

using EventMonitorMock = testing::NiceMock<fep3::mock::EventMonitor>;

/**
 * @brief Test IRPCDataRegistry::getStreamType
 * @req_id FEPSDK-2741
 */
TEST(ParticipantConfiguration, TestDataRegistry)
{
    using namespace fep3;

    constexpr const char* participant_name = "Participant1_configuration_test";
    constexpr const char* reader_name = "reader_1";
    constexpr const char* writer_name = "writer_1";
    constexpr const char* meta_type_name = "plain-ctype";
    constexpr const char* datatype_property = "datatype";

    System system_under_test(makePlatformDepName("Blackbox"));

    auto parts = createTestParticipants({ participant_name }, system_under_test.getSystemName());
    system_under_test.add(participant_name);

    system_under_test.load();
    system_under_test.initialize();

    auto participant = system_under_test.getParticipant(participant_name);

    auto data_registry = participant.getRPCComponentProxyByIID<fep3::rpc::IRPCDataRegistry>();
    ASSERT_TRUE(static_cast<bool>(data_registry));
    auto data_signals_in = data_registry->getSignalsIn();
    auto data_signals_out = data_registry->getSignalsOut();

    ASSERT_EQ(data_signals_in.size(), 1);
    ASSERT_EQ(data_signals_out.size(), 1);

    ASSERT_EQ(data_signals_in[0], reader_name);
    ASSERT_EQ(data_signals_out[0], writer_name);

    auto stream_type_in = data_registry->getStreamType(reader_name);
    auto stream_type_out = data_registry->getStreamType(writer_name);

    ASSERT_TRUE(stream_type_in);
    ASSERT_TRUE(stream_type_out);

    EXPECT_EQ(stream_type_in->getMetaTypeName(), meta_type_name);
    EXPECT_EQ(stream_type_out->getMetaTypeName(), meta_type_name);

    EXPECT_EQ(stream_type_in->getProperty(datatype_property), "int32_t");
    EXPECT_EQ(stream_type_out->getProperty(datatype_property), "int64_t");
}

