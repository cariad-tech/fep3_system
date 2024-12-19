# Copyright 2023 CARIAD SE. 
# 
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.



##################################################################
# tester_system_discovery
##################################################################
internal_fep3_participant_create_copy_target(tester_discover_system_participants)
fep3_system_create_copy_target(tester_discover_system_participants)
set(_current_test_name tester_discover_system_participants)
add_executable(${_current_test_name} discover_system_participants.cpp)
target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main GTest::gmock_main system_discovery_helper)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_discover_system_participants fep_participant_file_copy_tester_discover_system_participants)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system/private)
gtest_discover_tests(${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")