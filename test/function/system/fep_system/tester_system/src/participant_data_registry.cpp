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
#include <fep_test_common.h>
#include <fep3/base/stream_type/stream_type.h>
#include <fep3/base/properties/propertynode.h>
#include <fep_system/mock_event_monitor.h>
#include <gtest_asserts.h>

using EventMonitorMock = testing::NiceMock<fep3::mock::EventMonitor>;

const auto struct_name = "tTestStruct";
const auto ddl_description = R"(<?xml version="1.0" encoding="utf-8" standalone="no"?>
<!--
Copyright @ 2021 VW Group. All rights reserved.
 
    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 
If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.
 
You may add additional accurate notices of copyright ownership.
-->
<adtf:ddl xmlns:adtf="adtf">
 <header>
  <language_version>3.00</language_version>
  <author>fep_team</author>
  <date_creation>20.02.2020</date_creation>
  <date_change>20.02.2020</date_change>
  <description>Simplistic DDL for testing purposes</description>
 </header>
 <units />
 <datatypes>
  <datatype description="predefined ADTF tBool datatype" name="tBool" size="8" />
  <datatype description="predefined ADTF tChar datatype" name="tChar" size="8" />
  <datatype description="predefined ADTF tUInt8 datatype" name="tUInt8" size="8" />
  <datatype description="predefined ADTF tInt8 datatype" name="tInt8" size="8" />
  <datatype description="predefined ADTF tUInt16 datatype" name="tUInt16" size="16" />
  <datatype description="predefined ADTF tInt16 datatype" name="tInt16" size="16" />
  <datatype description="predefined ADTF tUInt32 datatype" name="tUInt32" size="32" />
  <datatype description="predefined ADTF tInt32 datatype" name="tInt32" size="32" />
  <datatype description="predefined ADTF tUInt64 datatype" name="tUInt64" size="64" />
  <datatype description="predefined ADTF tInt64 datatype" name="tInt64" size="64" />
  <datatype description="predefined ADTF tFloat32 datatype" name="tFloat32" size="32" />
  <datatype description="predefined, ADTF tFloat64 datatype" name="tFloat64" size="64" />
 </datatypes>
 <enums>
 </enums>
 <structs>
  <struct alignment="1" name="tTestStruct" version="1">
   <element alignment="1" arraysize="1" byteorder="LE" bytepos="0" name="ui8First" type="tUInt8" default="0"/>
   <element alignment="1" arraysize="1" byteorder="LE" bytepos="1" name="ui8Second" type="tUInt8" default="0"/>
  </struct>
 </structs>
 <streams />
</adtf:ddl>)";

/**
 * @brief Test IRPCDataRegistry::getStreamType
 * Test using plain type and complex ddl streamtypes containing special characters like e.g. ','.
 * @req_id FEPSDK-2741
 */
TEST(ParticipantConfiguration, TestDataRegistry)
{
    using namespace fep3;

    constexpr const char* participant_name = "Participant1_configuration_test";
    constexpr const char* reader_name = "reader_1";
    constexpr const char* reader_name2 = "reader_2";
    constexpr const char* writer_name = "writer_1";
    constexpr const char* meta_type_name = "plain-ctype";
    constexpr const char* datatype_property = "datatype";
    constexpr const char* ddl_struct_property = "ddlstruct";
    constexpr const char* ddl_description_property = "ddldescription";

    ASSERT_TRUE(std::string(ddl_description).find(",") != std::string::npos)
        << "We explicitly test for special characters (',') within this test but none are part of the description provided";

    const std::string system_name = makePlatformDepName("Blackbox");

    auto parts = createTestParticipants({ participant_name }, system_name);
    using namespace std::literals::chrono_literals;
    fep3::System system_under_test = fep3::discoverSystem(system_name, { participant_name }, 4000ms);

    const auto stream_type_ddl = fep3::base::StreamTypeDDL(struct_name, ddl_description);
    ASSERT_EQ(parts.begin()->second->_part.getComponent<IDataRegistry>()->registerDataIn(
        reader_name2, stream_type_ddl), a_util::result::Result());

    system_under_test.load();
    system_under_test.initialize();

    auto participant = system_under_test.getParticipant(participant_name);

    auto data_registry = participant.getRPCComponentProxyByIID<fep3::rpc::IRPCDataRegistry>();
    ASSERT_TRUE(static_cast<bool>(data_registry));
    auto data_signals_in = data_registry->getSignalsIn();
    auto data_signals_out = data_registry->getSignalsOut();

    ASSERT_EQ(data_signals_in.size(), 2);
    ASSERT_EQ(data_signals_out.size(), 1);

    ASSERT_FALSE(std::find(data_signals_in.begin(), data_signals_in.end(), reader_name) == data_signals_in.end());
    ASSERT_FALSE(std::find(data_signals_in.begin(), data_signals_in.end(), reader_name2) == data_signals_in.end());
    ASSERT_EQ(data_signals_out[0], writer_name);

    auto stream_type_in = data_registry->getStreamType(reader_name);
    auto stream_type_out = data_registry->getStreamType(writer_name);

    ASSERT_TRUE(stream_type_in);
    ASSERT_TRUE(stream_type_out);

    EXPECT_EQ(stream_type_in->getMetaTypeName(), meta_type_name);
    EXPECT_EQ(stream_type_out->getMetaTypeName(), meta_type_name);

    EXPECT_EQ(stream_type_in->getProperty(datatype_property), "int32_t");
    EXPECT_EQ(stream_type_out->getProperty(datatype_property), "int64_t");

    // check whether a ddl streamtype containing special characters like ',' can be retrieved
    auto stream_type_in_ddl = data_registry->getStreamType(reader_name2);
    ASSERT_TRUE(stream_type_in_ddl);
    ASSERT_EQ(stream_type_ddl.getMetaTypeName(), stream_type_in_ddl->getMetaTypeName());
    ASSERT_EQ(stream_type_ddl.getPropertyNames(), stream_type_in_ddl->getPropertyNames());
    ASSERT_EQ(stream_type_ddl.getProperty(ddl_struct_property), stream_type_in_ddl->getProperty(ddl_struct_property));
    ASSERT_EQ(stream_type_ddl.getProperty(ddl_description_property), stream_type_in_ddl->getProperty(ddl_description_property));
}

