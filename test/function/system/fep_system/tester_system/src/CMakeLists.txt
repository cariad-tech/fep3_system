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
fep3_participant_create_copy_target(tester_system)
fep3_system_create_copy_target(tester_system)
set_target_properties(fep_system_file_copy_tester_system PROPERTIES FOLDER test/fep_system/copy_helpers)
set_target_properties(fep_participant_file_copy_tester_system PROPERTIES FOLDER test/fep_system/copy_helpers)

##################################################################
# tester_system_discovery
##################################################################

set(_current_test_name tester_system_discovery)
add_executable(${_current_test_name} system_discovery.cpp fep_test_common.h)
target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main fep3_system fep3_participant_core dev_essential::process)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")


##################################################################
# tester_system
##################################################################

set(_current_test_name tester_system)
add_executable(${_current_test_name} system.cpp fep_test_common.h)
target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main fep3_system fep3_participant_core dev_essential::process)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)

set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

##################################################################
# tester_system_library_participant
##################################################################

set(_current_test_name tester_system_library_participant)
add_executable(${_current_test_name} participant.cpp fep_test_common.h)
target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main fep3_system fep3_participant_core dev_essential::process)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)

set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

##################################################################
# tester_system_library_part_config
##################################################################

set(_current_test_name tester_system_library_part_config)
add_executable(${_current_test_name} participant_configuration.cpp fep_test_common.h)
target_link_libraries(${_current_test_name}
        PRIVATE GTest::gmock GTest::gtest_main fep3_system fep3_participant_core dev_essential::process)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

##################################################################
# tester_system_library_logging
##################################################################

set(_current_test_name tester_system_library_logging)
add_executable(${_current_test_name} system_logging.cpp fep_test_common.h)
target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main fep3_system fep3_participant_core dev_essential::process)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

##################################################################
# tester_system_library_data_registry
##################################################################

set(_current_test_name tester_system_library_data_registry)
add_executable(${_current_test_name} participant_data_registry.cpp fep_test_common.h)
target_link_libraries(${_current_test_name}
                  PRIVATE GTest::gtest_main fep3_system fep3_participant_core dev_essential::process)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
     COMMAND ${_current_test_name}
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

##################################################################
# tester_system_library_health
##################################################################

set(_current_test_name tester_system_library_health)
add_executable(${_current_test_name} system_health.cpp fep_test_common.h)
target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main fep3_system fep3_participant_core dev_essential::process)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)

get_filename_component(PARENT_DIR ../ ABSOLUTE)
configure_file(${PARENT_DIR}/files/fep3_participant.fep_components.in ${PARENT_DIR}/files/fep3_participant.fep_components.in.temp)
file(GENERATE OUTPUT  ${PARENT_DIR}/files/fep3_participant_$<CONFIG>.fep_components 
               INPUT ${PARENT_DIR}/files/fep3_participant.fep_components.in.temp)

 target_compile_definitions(${_current_test_name} PRIVATE
     TEST_BUILD_DIR="${PARENT_DIR}/files/fep3_participant_$<CONFIG>.fep_components"
 )

set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

##################################################################
# tester_rpc_passthrough
##################################################################

set(_current_test_name tester_rpc_passthrough)
add_executable(${_current_test_name} rpc_passthrough.cpp fep_test_common.h)
target_link_libraries(${_current_test_name}
                      PRIVATE GTest::gtest_main fep3_system fep3_participant_core dev_essential::process)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
 target_compile_definitions(${_current_test_name} PRIVATE
     TEST_BUILD_DIR="${PARENT_DIR}/files/fep3_participant_$<CONFIG>.fep_components"
 )

set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

