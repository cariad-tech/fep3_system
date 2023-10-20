#
# Copyright @ 2021 VW Group. All rights reserved.
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



##################################################################
# tester_health_service_helpers
##################################################################

set(_current_test_name tester_health_service_helpers)
add_executable(${_current_test_name}
                participant_health_aggregator.cpp
                participant_health_listener.cpp
                tester_health_service_helpers_common.h)

target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main GTest::gmock_main health_service_helper)

set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system/private)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
