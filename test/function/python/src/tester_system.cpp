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


#include <gtest/gtest.h>
#include <fep_system/fep_system.h>
#include "fep_test_common.h"
//#include <a_util/process.h>
#include <a_util/filesystem.h>  // for replacement implementation


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
    // due to a bug in a_util::process::execute does not work
    // switch to this implementation, when the bug has been fixed
    /*// call python and execute testcases
    std::string std_out;
    uint32_t res = a_util::process::execute(PYTHON_EXECUTABLE,
        "-m pytest -q " PYTESTS_TESTS_DIR "/tester_system.py", WORKING_DIRECTORY, std_out);

    // on error print error message
    if (res) {
        std::cout << std_out << std::endl;
    }*/

    // replacement implementation for bug in a_util::process::execute
    auto current_dir = a_util::filesystem::getWorkingDirectory();
    a_util::filesystem::setWorkingDirectory(WORKING_DIRECTORY);
    int res = system("\"" PYTHON_EXECUTABLE "\"" " -m pytest -q " PYTESTS_TESTS_DIR
        "/tester_system.py -W ignore::pytest.PytestCollectionWarning");
    a_util::filesystem::setWorkingDirectory(current_dir);

    EXPECT_FALSE(res);

}

/*TEST(SystemLibrary, TestMonitorSystemOK)
{
}*/