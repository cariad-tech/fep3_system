# Copyright 2023 CARIAD SE. 
# 
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

##################################################################
# tester_system_participant_logger
##################################################################

set(test_name tester_system_participant_logger)
add_executable(${test_name}
               ${test_name}.cpp
               ${PROJECT_SOURCE_DIR}/src/fep_system/system_logger.h)

target_link_libraries(${test_name}
                      PRIVATE
                      Boost::headers
                      GTest::gtest_main
                      GTest::gmock_main
                      fep3_system
                      dev_essential::strings)

set_target_PROPERTIES(${test_name} PROPERTIES FOLDER test/fep_system/private)
add_test(NAME ${test_name}
         COMMAND ${test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
