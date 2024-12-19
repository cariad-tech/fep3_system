/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
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
#include "participant_configuration_helper.hpp"
#include <fep_test_common.h>
#include <fep3/base/properties/propertynode.h>
#include <fep3/base/properties/propertynode_helper.h>
#include <fep_system/mock_event_monitor.h>
#include <gtest_asserts.h>
#include <fep3/components/service_bus/service_bus_intf.h>


using EventMonitorMock = testing::NiceMock<fep3::mock::EventMonitor>;
using namespace std::literals::chrono_literals;

TEST(ParticipantConfiguration, TestProxyConfigGetter)
{
    using namespace fep3;
    const std::string system_name = makePlatformDepName("Blackbox");

    auto parts = createTestParticipants({ "Participant1_configuration_test" }, system_name);
    fep3::System system_under_test = fep3::discoverSystem(system_name, { "Participant1_configuration_test" }, 4000ms);

    auto p1 = system_under_test.getParticipant("Participant1_configuration_test");

    auto config = p1.getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>();
    ASSERT_TRUE(static_cast<bool>(config));

    auto& part = parts["Participant1_configuration_test"];
    auto config_service = part->_part.getComponent<IConfigurationService>();

    auto props = std::make_shared<base::NativePropertyNode>("deeper");
    props->setChild(base::makeNativePropertyNode("bool_value", false));
    props->setChild(base::makeNativePropertyNode("int_value", 1));
    props->setChild(base::makeNativePropertyNode("double_value", double(0.1)));
    props->setChild(base::makeNativePropertyNode("string_value", std::string("test_value")));
    config_service->registerNode(props);

    bool bool_value = true;
    testGetter(*config_service, config.getInterface(), bool_value, "bool_value", "deeper");
    bool_value = false;
    testGetter(*config_service, config.getInterface(), bool_value, "bool_value", "deeper");

    int32_t int_value = 3456;
    testGetter(*config_service, config.getInterface(), int_value, "int_value", "deeper");

    double double_value = 1.0;
    testGetter(*config_service, config.getInterface(), double_value, "double_value", "deeper");

    std::string string_val = "this_is_may_string";
    testGetter(*config_service, config.getInterface(), string_val, "string_value", "deeper");
}


TEST(ParticipantConfiguration, TestProxyConfigSetter)
{
    using namespace fep3;
    const std::string system_name = makePlatformDepName("Blackbox");

    auto parts = createTestParticipants({ "Participant1_configuration_test" }, system_name);
    fep3::System system_under_test = fep3::discoverSystem(system_name, { "Participant1_configuration_test" }, 4000ms);

    auto p1 = system_under_test.getParticipant("Participant1_configuration_test");

    auto config = p1.getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>();
    ASSERT_TRUE(static_cast<bool>(config));

    auto& part = parts["Participant1_configuration_test"];
    auto config_service = part->_part.getComponent<IConfigurationService>();

    auto props = std::make_shared<base::NativePropertyNode>("deeper");
    props->setChild(base::makeNativePropertyNode("bool_value", false));
    props->setChild(base::makeNativePropertyNode("int_value", 1));
    props->setChild(base::makeNativePropertyNode("double_value", double(0.1)));
    props->setChild(base::makeNativePropertyNode("string_value", std::string("test_value")));
    config_service->registerNode(props);

    bool bool_value = true;
    testSetter(*config_service, config.getInterface(), bool_value, false, "bool_value", "deeper");
    bool_value = false;
    testSetter(*config_service, config.getInterface(), bool_value, true, "bool_value", "deeper");

    int32_t int_value = 3456;
    testSetter(*config_service, config.getInterface(), int_value, 1, "int_value", "deeper");

    double double_value = 1.0;
    testSetter(*config_service, config.getInterface(), double_value, 0.0, "double_value", "deeper");

    std::string string_val = "this_is_may_string";
    testSetter(*config_service, config.getInterface(), string_val,
        std::string("init_val"), std::string("string_value"), "deeper");
}

TEST(ParticipantConfiguration, TestProxyConfigArraySetter)
{
    using namespace fep3;
    const std::string system_name = makePlatformDepName("Blackbox");

    auto parts = createTestParticipants({ "Participant1_configuration_test" }, system_name);
    fep3::System system_under_test = fep3::discoverSystem(system_name, { "Participant1_configuration_test" }, 4000ms);

    auto p1 = system_under_test.getParticipant("Participant1_configuration_test");

    auto config = p1.getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>();
    ASSERT_TRUE(static_cast<bool>(config));

    auto& part = parts["Participant1_configuration_test"];
    auto config_service = part->_part.getComponent<IConfigurationService>();

    auto props = std::make_shared<base::NativePropertyNode>("deeper");
    props->setChild(base::makeNativePropertyNode("test_bool_array", std::vector<bool>()));
    props->setChild(base::makeNativePropertyNode("test_int_array", std::vector<int32_t>()));
    props->setChild(base::makeNativePropertyNode("test_double_array", std::vector<double>()));
    props->setChild(base::makeNativePropertyNode("test_string_array", std::vector<std::string>()));
    config_service->registerNode(props);

    std::vector<bool> bool_value_array = { true, false, true };
    testSetter<std::vector<bool>>(*config_service,
        config.getInterface(), bool_value_array, { false, true, false }, "test_bool_array", "deeper");

    std::vector<int32_t> int_value_array = { 3456, 2, 3, 4};
    testSetter<std::vector<int32_t>>(*config_service,
        config.getInterface(), int_value_array, { 1, 2 }, "test_int_array", "deeper");

    std::vector<double> double_value_array = { 1.0, 2.0 };
    testSetter<std::vector<double>>(*config_service,
        config.getInterface(), double_value_array, { 0.0, 0.0 }, "test_double_array", "deeper");

    std::vector<std::string> string_val_array = { "this_is_may_string", "test2", "test3" };
    testSetter<std::vector<std::string>>(*config_service,
        config.getInterface(), string_val_array, { "init_val", "another_val" }, "test_string_array", "deeper");
}

/** @brief Test whether the system lib refuses to support the '.' property syntax.
*   This means the system shall not:
*   - set a value
*   - get the value
*   - get the type
*   of properties which use the '.' syntax.
*   Properties have to use a '/' delimiter instead of '.'.
*   @req_id FEPSDK-2171
*/
TEST(cTesterParticipantConfiguration, TestInvalidPropertySyntax)
{
    using namespace fep3;
    const std::string system_name = makePlatformDepName("Blackbox");

    auto parts = createTestParticipants({ "Participant1_configuration_test" }, system_name);
    fep3::System system_under_test = fep3::discoverSystem(system_name, { "Participant1_configuration_test" }, 4000ms);

    auto p1 = system_under_test.getParticipant("Participant1_configuration_test");

    auto config = p1.getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>();
    ASSERT_TRUE(static_cast<bool>(config));

    auto& part = parts["Participant1_configuration_test"];
    auto config_service = part->_part.getComponent<IConfigurationService>();

    auto props = std::make_shared<base::NativePropertyNode>("test_node");
    props->setChild(base::makeNativePropertyNode("bool_value", false));
    props->setChild(base::makeNativePropertyNode("int_value", 1));
    props->setChild(base::makeNativePropertyNode("double_value", double(0.1)));
    props->setChild(base::makeNativePropertyNode("string_value", std::string("test_value")));
    config_service->registerNode(props);

    // we have to set already existing properties because not existing properties may not be set using the system lib

    bool bool_value = true;
    testSetGetInvalidFormat(config.getInterface(), bool_value, "test_node.test_bool");

    int32_t int_value = 3456;
    testSetGetInvalidFormat(config.getInterface(), int_value, "test_node.int_value");

    double double_value = 1.0;
    testSetGetInvalidFormat(config.getInterface(), double_value, "test_node.double_value");

    std::string string_value = "test_value";
    testSetGetInvalidFormat(config.getInterface(), string_value, "test_node.string_value");
}

/**
 * @brief Test whether root properties may be set by setting a property with a fully specified path.
 * @req_id FEPSDK-2171
 */
TEST(cTesterParticipantConfiguration, TestRootPropertySetter)
{
    using namespace fep3;
    const std::string system_name = makePlatformDepName("Blackbox");

    auto parts = createTestParticipants({ "Participant1_configuration_test" }, system_name);
    fep3::System system_under_test = fep3::discoverSystem(system_name, { "Participant1_configuration_test" }, 4000ms);

    auto p1 = system_under_test.getParticipant("Participant1_configuration_test");

    auto config = p1.getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>();
    ASSERT_TRUE(static_cast<bool>(config));

    auto& part = parts["Participant1_configuration_test"];
    auto config_service = part->_part.getComponent<IConfigurationService>();

    auto props = std::make_shared<base::NativePropertyNode>("test_node");
    props->setChild(base::makeNativePropertyNode("bool_value", false));

    int32_t initial_int_value = 4;
    auto prop_sub = base::makeNativePropertyNode("int_value_sub", initial_int_value);
    props->setChild(prop_sub);
    prop_sub->setChild(base::makeNativePropertyNode("child_int_value", initial_int_value));

    config_service->registerNode(props);

    bool bool_value = true, initial_bool_value = false;
    testSetGetRoot(config.getInterface(), initial_bool_value, bool_value, "/test_node", "bool_value");

    bool_value = false;
    initial_bool_value = true;
    testSetGetRoot(config.getInterface(), initial_bool_value, bool_value, "", "test_node/bool_value");

    int32_t int_value = 5;
    initial_int_value = 4;
    testSetGetRoot(config.getInterface(), initial_int_value, int_value, "/test_node/int_value_sub", "child_int_value");

    int_value = 10;
    initial_int_value = 5;
    testSetGetRoot(config.getInterface(), initial_int_value, int_value, "/", "test_node/int_value_sub/child_int_value");

    int_value = 20;
    initial_int_value = 10;
    testSetGetRoot(config.getInterface(), initial_int_value, int_value, "test_node", "int_value_sub/child_int_value");

}

/**
 * @brief Test whether root properties may be set by setting a property with a fully specified path.
 * @req_id FEPSDK-2164
 */
TEST(cTesterParticipantConfiguration, TestProxyConfigPropNotExistsNoCreate)
{
    using namespace fep3;
    const std::string system_name = makePlatformDepName("Blackbox");

    auto parts = createTestParticipants({ "Participant1_configuration_test" }, system_name);
    fep3::System system_under_test = fep3::discoverSystem(system_name, { "Participant1_configuration_test" }, 4000ms);

    auto p1 = system_under_test.getParticipant("Participant1_configuration_test");

    auto config = p1.getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>();
    ASSERT_TRUE(static_cast<bool>(config));

    auto& part = parts["Participant1_configuration_test"];
    auto config_service = part->_part.getComponent<IConfigurationService>();

    auto prop_opt = base::getPropertyValue<std::string>(*config_service, "test_does_not_exists");
    //check if it does not exists
    ASSERT_FALSE(prop_opt);

    auto props = config->getProperties("/");
    auto retval = props->getProperty("test_does_not_exists");
    auto retval_type = props->getPropertyType("test_does_not_exists");
    //check if empty
    ASSERT_EQ(retval, std::string());
    ASSERT_EQ(retval_type, std::string());
    props->setProperty("test_does_not_exists", "value", "string");
    //check if still empty
    retval = props->getProperty("test_does_not_exists");
    retval_type = props->getPropertyType("test_does_not_exists");
    ASSERT_EQ(retval, std::string());
    ASSERT_EQ(retval_type, std::string());

    prop_opt = base::getPropertyValue<std::string>(*config_service, "test_does_not_exists");
    //check if it does not exists
    ASSERT_FALSE(prop_opt);
}

/**
 * @brief Test getter and setter for start priority and init priority
 * @req_id FEPSDK-3602
 */
TEST(cTesterParticipantConfiguration, TestProxyConfigPrioritySetterAndGetter)
{
    using namespace fep3;
    const std::string system_name = makePlatformDepName("system1");

    const std::string participant_name = "Participant1_configuration_test";

    auto parts = createTestParticipants({ participant_name }, system_name);
    fep3::System system1 = fep3::discoverSystem(system_name, { participant_name }, 4000ms);
    ParticipantProxy p1 = system1.getParticipant(participant_name);
    ASSERT_EQ(p1.getStartPriority(), 0);
    p1.setStartPriority(10);
    ASSERT_EQ(p1.getStartPriority(), 10);

    ASSERT_EQ(p1.getInitPriority(), 0);
    p1.setInitPriority(20);
    ASSERT_EQ(p1.getInitPriority(), 20);

    fep3::System system2 = fep3::discoverSystem(system_name, { participant_name }, 4000ms);
    ParticipantProxy p2 = system2.getParticipant(participant_name);

    ASSERT_EQ(p2.getStartPriority(), 10);
    ASSERT_EQ(p2.getInitPriority(), 20);
}

/**
 * @brief Test getter and setter for start priority and init priority in old version
 * @req_id FEPSDK-3602
 */
TEST(cTesterParticipantConfiguration, TestProxyConfigPriorityCompability)
{
    using namespace fep3;
    const std::string system_name = makePlatformDepName("Blackbox");
    const std::string participant_name = "Participant1_configuration_test";

    // Remove the properties for priority, as the old version of participant do not have them
    auto parts = createTestParticipants({ participant_name }, system_name);
    auto& part = parts[participant_name];
    auto config_service = part->_part.getComponent<IConfigurationService>();
    config_service->unregisterNode(FEP3_SERVICE_BUS_CONFIG);

    // Make sure the property does not exist 
    fep3::System system1 = fep3::discoverSystem(system_name, { participant_name }, 4000ms);
    ParticipantProxy p1 = system1.getParticipant(participant_name);
    auto config = p1.getRPCComponentProxyByIID<fep3::rpc::IRPCConfiguration>();
    ASSERT_TRUE(static_cast<bool>(config));

    auto props = config->getProperties("/");
    auto retval = props->getProperty(FEP3_SERVICE_BUS_CONFIG);
    ASSERT_EQ(retval, std::string());

    // Test the compability

    ASSERT_EQ(p1.getInitPriority(), 0);
    p1.setInitPriority(10);
    ASSERT_EQ(p1.getInitPriority(), 10);

    ASSERT_EQ(p1.getStartPriority(), 0);
    p1.setStartPriority(20);
    ASSERT_EQ(p1.getStartPriority(), 20);

    // Test the old behavior: the priority is local
    fep3::System system2 = fep3::discoverSystem(system_name, { participant_name }, 4000ms);
    ParticipantProxy p2 = system2.getParticipant(participant_name);

    ASSERT_EQ(p2.getInitPriority(), 0);
    ASSERT_EQ(p2.getStartPriority(), 0);
}

