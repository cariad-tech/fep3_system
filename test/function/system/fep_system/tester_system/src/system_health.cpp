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

#include <gtest/gtest.h>

#include "fep_test_common.h"
#include <fep_system/fep_system.h>
#include <fep3/components/experimental/health_service_intf.h>
#include "../../../../../function/utils/common/gtest_asserts.h"

using namespace fep3;

struct SystemLibrarySingleParticipant : public testing::Test
{
    SystemLibrarySingleParticipant()
        : _system(makePlatformDepName(_system_name))
    {
    }

    void SetUp() override
    {
        // health state tests require the tested participant to provide a health service component which is not enabled by default currently
        const std::string components_file_path(std::string(TEST_BUILD_DIR) + "/../files/fep3_participant.fep_components");
        a_util::process::setEnvVar("FEP3_PARTICIPANT_COMPONENTS_FILE_PATH", components_file_path);

        _participants = createTestParticipants({ _participant_name }, _system.getSystemName());

        ASSERT_NO_THROW(
            _system.add(_participant_name);
        );
    }

    void TearDown() override
    {
        a_util::process::setEnvVar("FEP3_PARTICIPANT_COMPONENTS_FILE_PATH", "");
    }

    const char* _system_name{ "test_system" };
    experimental::System _system;

    TestParticipants _participants{};
    const std::string _participant_name{"test_participant"};
};

/**
 * @brief Test whether the system library returns the correct participant health state via rpc using getHealth.
 */
TEST_F(SystemLibrarySingleParticipant, TestParticpantGetHealthStateViaRPC)
{
    auto* health_service = _participants[_participant_name]->_part.getComponent<fep3::experimental::IHealthService>();
    ASSERT_TRUE(health_service);
    const auto participant_proxy = _system.getParticipant(_participant_name);

    auto health_service_proxy = participant_proxy.getRPCComponentProxy<fep3::experimental::IRPCHealthService>();

    ASSERT_EQ(health_service_proxy->getRPCDefaultName(), rpc::getRPCDefaultName<fep3::experimental::IRPCHealthServiceDef>());
    ASSERT_EQ(health_service_proxy->getRPCIID(), rpc::getRPCIID<fep3::experimental::IRPCHealthServiceDef>());
    ASSERT_EQ(health_service->getHealth(), experimental::HealthState::ok);
    ASSERT_EQ(health_service_proxy->getHealth(), experimental::HealthState::ok);

    // actual test
    {
        ASSERT_EQ(_system.getHealth(_participant_name), experimental::HealthState::ok);
    }

    ASSERT_FEP3_NOERROR(health_service->setHealthToError(""));
    ASSERT_EQ(health_service->getHealth(), experimental::HealthState::error);
    ASSERT_EQ(health_service_proxy->getHealth(), experimental::HealthState::error);

    // actual test
    {
        ASSERT_EQ(_system.getHealth(_participant_name), experimental::HealthState::error);
    }
}

/**
 * @brief Test whether the system library successfully resets a participant health state to state ok via rpc using resetHealth.
 */
TEST_F(SystemLibrarySingleParticipant, TestParticpantResetHealthStateViaRPC)
{
    auto* health_service = _participants[_participant_name]->_part.getComponent<fep3::experimental::IHealthService>();
    ASSERT_TRUE(health_service);

    ASSERT_EQ(health_service->getHealth(), experimental::HealthState::ok);
    ASSERT_FEP3_NOERROR(health_service->setHealthToError(""));
    ASSERT_EQ(health_service->getHealth(), experimental::HealthState::error);

    // actual test
    {
        ASSERT_FEP3_NOERROR(_system.resetHealth(_participant_name, ""));
        ASSERT_EQ(health_service->getHealth(), experimental::HealthState::ok);
    }
}

bool setParticipantsHealthStatesToError(TestParticipants& participants,
    const std::vector<std::string>& participant_names)
{
    for (const auto& participant_name : participant_names)
    {
        auto* health_service = participants[participant_name]->_part.getComponent<fep3::experimental::IHealthService>();
        if (!health_service)
        {
            return false;
        }
        health_service->setHealthToError("");
    }

    return true;
}

bool setParticipantsHealthStatesToOk(TestParticipants& participants,
    const std::vector<std::string>& participant_names)
{
    for (const auto& participant_name : participant_names)
    {
        auto* health_service = participants[participant_name]->_part.getComponent<fep3::experimental::IHealthService>();
        if (!health_service)
        {
            return false;
        }
        health_service->resetHealth("");
    }

    return true;
}

struct SystemLibraryThreeParticipants : public testing::Test
{
    SystemLibraryThreeParticipants()
        : _system(makePlatformDepName(_system_name))
    {
    }

    void SetUp() override
    {
        // health state tests require the tested participant to provide a health service component which is not enabled by default currently
        const std::string components_file_path(std::string(TEST_BUILD_DIR) + "/../files/fep3_participant.fep_components");
        a_util::process::setEnvVar("FEP3_PARTICIPANT_COMPONENTS_FILE_PATH", components_file_path);

        _participants = createTestParticipants({
            _participant_name_1, _participant_name_2, _participant_name_3},
            _system.getSystemName());

        ASSERT_NO_THROW(
            _system.add(_participant_name_1);
            _system.add(_participant_name_2);
            _system.add(_participant_name_3);
        );
    }

    void TearDown() override
    {
        a_util::process::setEnvVar("FEP3_PARTICIPANT_COMPONENTS_FILE_PATH", "");
    }

    const char* _system_name{ "test_system" };
    fep3::experimental::System _system;
    TestParticipants _participants{};
    const std::string _participant_name_1{ "test_participant_1" };
    const std::string _participant_name_2{ "test_participant_2" };
    const std::string _participant_name_3{ "test_participant_3" };
};

/**
 * @brief Test whether the system library returns the correct system health state for a homogeneous system via rpc using getSystemHealth.
 */
TEST_F(SystemLibraryThreeParticipants, TestGetSystemHealthStateHomogenous)
{
    // actual test
    {
        const auto system_health_state = _system.getSystemHealth();
        ASSERT_TRUE(system_health_state._homogeneous);
        ASSERT_EQ(fep3::experimental::HealthState::ok, system_health_state._system_health_state);
    }
    ASSERT_TRUE(setParticipantsHealthStatesToError(_participants, { _participant_name_1, _participant_name_2, _participant_name_3 }));

    // actual test
    {
        const auto system_health_state = _system.getSystemHealth();
        ASSERT_TRUE(system_health_state._homogeneous);
        ASSERT_EQ(fep3::experimental::HealthState::error, system_health_state._system_health_state);
    }
    ASSERT_TRUE(setParticipantsHealthStatesToOk(_participants, { _participant_name_1, _participant_name_2, _participant_name_3 }));

    // actual test
    {
        const auto system_health_state = _system.getSystemHealth();
        ASSERT_TRUE(system_health_state._homogeneous);
        ASSERT_EQ(fep3::experimental::HealthState::ok, system_health_state._system_health_state);
    }
}

/**
 * @brief Test whether the system library returns the correct system health state for an inhomogeneous system via rpc using getSystemHealth.
 */
TEST_F(SystemLibraryThreeParticipants, TestGetSystemHealthStateInhomogenous)
{
    ASSERT_TRUE(setParticipantsHealthStatesToError(_participants, { _participant_name_2 }));

    // actual test
    {
        const auto system_health_state = _system.getSystemHealth();
        ASSERT_FALSE(system_health_state._homogeneous);
        ASSERT_EQ(fep3::experimental::HealthState::error, system_health_state._system_health_state);
    }
    ASSERT_TRUE(setParticipantsHealthStatesToOk(_participants, { _participant_name_2 }));
    ASSERT_TRUE(setParticipantsHealthStatesToError(_participants, { _participant_name_1, _participant_name_3 }));

    // actual test
    {
        const auto system_health_state = _system.getSystemHealth();
        ASSERT_FALSE(system_health_state._homogeneous);
        ASSERT_EQ(fep3::experimental::HealthState::error, system_health_state._system_health_state);
    }
}
