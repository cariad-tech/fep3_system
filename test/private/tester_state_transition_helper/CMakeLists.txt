# Copyright 2023 CARIAD SE. 
# 
#     This Source Code Form is subject to the terms of the Mozilla
#     Public License, v. 2.0. If a copy of the MPL was not distributed
#     with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
# 
#



##################################################################
# tester_rpc_participant_state_machine
##################################################################
find_package(Boost REQUIRED)

set(_current_test_name tester_state_transition_helper)
add_executable(${_current_test_name}
                test_state_transition_helper.cpp
                test_fep_system_state_tree.cpp
                test_execution_policy.cpp
                test_excecution_with_error_handling.cpp
                test_fep_state_transition_controller.cpp
                test_shutdown_helper.cpp
                test_state_transition_helper_common.h)

target_link_libraries(${_current_test_name}
                      PRIVATE
                      GTest::gtest_main
                      GTest::gmock_main
                      state_transition_helper
                      Boost::boost)

target_include_directories(${_current_test_name} PUBLIC
    ${PROJECT_SOURCE_DIR}/src/fep_system
    ${fep3_participant_INCLUDE_DIR}
    $<TARGET_PROPERTY:fep3_system,INTERFACE_INCLUDE_DIRECTORIES>
)

set_target_PROPERTIES(${_current_test_name} PROPERTIES 
    FOLDER test/fep_system/private
    INSTALL_RPATH "$ORIGIN")
gtest_discover_tests(${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)