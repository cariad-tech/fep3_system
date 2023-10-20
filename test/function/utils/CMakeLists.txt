#
# Copyright @ 2022 VW Group. All rights reserved.
# 
#     This Source Code Form is subject to the terms of the Mozilla
#     Public License, v. 2.0. If a copy of the MPL was not distributed
#     with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
# 
# If it is not possible or desirable to put the notice in a particular file, then
# You may include the notice in a location (such as a LICENSE file in a
# relevant directory) where a recipient would be likely to look for such a notice.
# 
# You may add additional accurate notices of copyright ownership.
# 
#

### the test utils library
set(SYSTEM_TEST_UTILS system_test_utils)

add_library(${SYSTEM_TEST_UTILS} STATIC)

target_sources(${SYSTEM_TEST_UTILS} PRIVATE
    common/fep_test_common.h
    common/fep_test_common.cpp
    common/gtest_asserts.h
    common/gmock_async_helper.h
)

set_target_properties(${SYSTEM_TEST_UTILS} PROPERTIES FOLDER "test/function/utils")

target_include_directories(${SYSTEM_TEST_UTILS} PUBLIC
    .
    ./common
)

target_link_libraries(${SYSTEM_TEST_UTILS} 
    PUBLIC
        fep3_participant_core
    PRIVATE
        dev_essential::process
)

### the basic participant
find_package(fep3_participant_cpp REQUIRED)
get_target_property(SYSTEM_TEST_UTILS_SOURCE_DIR system_test_utils SOURCE_DIR)
add_executable(basic_participant
    participants/basic_participant.cpp
    participants/basic_participant.h
    )
target_link_libraries(basic_participant PRIVATE
    fep3_participant_cpp
    )
set_target_PROPERTIES(basic_participant PROPERTIES FOLDER "test/function/utils")
set_target_properties(basic_participant PROPERTIES INSTALL_RPATH "$ORIGIN")
internal_fep3_participant_create_copy_target(basic_participant)
fep3_system_create_copy_target(basic_participant)
set_target_properties(fep_participant_file_copy_basic_participant PROPERTIES FOLDER "test/function/utils/copy_helpers")
add_dependencies(basic_participant fep_participant_file_copy_basic_participant)