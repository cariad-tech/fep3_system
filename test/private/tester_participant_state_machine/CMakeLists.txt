# Copyright 2023 CARIAD SE. 
# 
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.



##################################################################
# tester_rpc_participant_state_machine
##################################################################

find_package(dev_essential REQUIRED COMPONENTS result pkg_rpc)
set(_current_test_name tester_rpc_participant_state_machine)
add_executable(${_current_test_name}
                rpc_participant_state_machine.cpp)
add_dependencies(${_current_test_name} fep_system_rpc_stub_generator)
target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main 
                      dev_essential::pkg_rpc)

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