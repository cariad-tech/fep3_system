# Copyright 2024 CARIAD SE. 
# 
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.



##################################################################
# tester_service_bus_helper
##################################################################

set(_current_test_name tester_service_bus_helper)
add_executable(${_current_test_name}
                participant_shutdown_listener.cpp)

target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main GTest::gmock_main service_bus_helper)

set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system/private)
gtest_discover_tests(${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
