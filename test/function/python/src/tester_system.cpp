/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#include <gtest/gtest.h>
#include <fep_system/fep_system.h>
#include "fep_test_common.h"
#include <fep3/fep3_filesystem.h>


struct SystemLibraryWithTestSystem : public ::testing::Test {
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

TEST_F(SystemLibraryWithTestSystem, TestPybind11)
{
    const auto current_dir = fs::current_path();
    const auto working_dir = fs::path(WORKING_DIRECTORY);
    ASSERT_TRUE(fs::exists(working_dir));
    ASSERT_NO_THROW(fs::current_path(working_dir));
    const std::string cmd = std::string(ACTIVATE_VENV_EXEC) + " && python -m pytest -q " + std::string(PYTESTS_TESTS_DIR) + "/tester_system.py -W ignore::pytest.PytestCollectionWarning";
    int res = system(cmd.c_str());

    fs::current_path(current_dir);

    EXPECT_FALSE(res);

}
