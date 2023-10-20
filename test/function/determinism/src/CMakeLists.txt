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
internal_fep3_participant_create_copy_target(tester_determinism)
fep3_system_create_copy_target(tester_determinism)
set_target_properties(fep_system_file_copy_tester_determinism PROPERTIES FOLDER test/determinism/copy_helpers)
set_target_properties(fep_participant_file_copy_tester_determinism PROPERTIES FOLDER test/determinism/copy_helpers)

find_package(Boost REQUIRED COMPONENTS filesystem)

if(NOT TARGET fep3_participant_cpp)
    find_package(fep3_participant_cpp REQUIRED)
endif()

find_package(dev_essential REQUIRED COMPONENTS process filesystem)

##################################################################
# tester_determinism
##################################################################
add_executable(tester_determinism
    tester_determinism_main.cpp
    tester_determinism.cpp
    test_environment.h)

target_link_libraries(tester_determinism PRIVATE
    GTest::gtest_main
    fep3_system
    fep3_participant_core
    system_test_utils
    dev_essential::process
    dev_essential::pkg_rpc
    dev_essential::filesystem
    Boost::filesystem)

target_compile_definitions(tester_determinism PRIVATE 
    TIMING_MASTER_EXECUTABLE_NAME="$<TARGET_FILE:basic_participant>"
    )
set_target_PROPERTIES(tester_determinism PROPERTIES FOLDER test/determinism)
add_test(NAME tester_determinism
         COMMAND tester_determinism
         WORKING_DIRECTORY $<TARGET_FILE_DIR:receiver>)
if(UNIX)
    set_tests_properties(tester_determinism PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:receiver> TIMEOUT 60)
else()
# aev_testing_pkg adds a configuration type to the test name...
    foreach (CONFIGURATION ${CMAKE_CONFIGURATION_TYPES})
        if(TEST tester_determinism_${CONFIGURATION})
            set_tests_properties(tester_determinism_${CONFIGURATION} PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CONFIGURATION} TIMEOUT 60)
        endif()
    endforeach()
    set_target_properties(tester_determinism PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)")
endif()
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
add_dependencies(tester_determinism fep_system_file_copy_tester_determinism)

##################################################################
# common files
##################################################################

set(COMMON_SOURCES
    determinism_helper.hpp)

##################################################################
# transmitter_one
##################################################################

add_executable(transmitter_one
    transmitter_one.cpp
    transmitter_base.h
    transmitter_base.cpp
    ${COMMON_SOURCES})

target_link_libraries(transmitter_one PRIVATE
    fep3_participant_cpp
    dev_essential::filesystem
    dev_essential::pkg_rpc
    clipp::clipp)

set_target_PROPERTIES(transmitter_one PROPERTIES FOLDER test/determinism/participants)
set_target_properties(transmitter_one PROPERTIES INSTALL_RPATH "$ORIGIN")
add_dependencies(transmitter_one fep_participant_file_copy_tester_determinism)

##################################################################
# transmitter_two
##################################################################

add_executable(transmitter_two
    transmitter_two.cpp
    transmitter_base.h
    transmitter_base.cpp
    ${COMMON_SOURCES})

target_link_libraries(transmitter_two PRIVATE
    fep3_participant_cpp
    dev_essential::filesystem
    dev_essential::pkg_rpc
    clipp::clipp)


set_target_PROPERTIES(transmitter_two PROPERTIES FOLDER test/determinism/participants)
set_target_properties(transmitter_two PROPERTIES INSTALL_RPATH "$ORIGIN")
add_dependencies(transmitter_two fep_participant_file_copy_tester_determinism)

##################################################################
# receiver
##################################################################

add_executable(receiver
    receiver.cpp
    ${COMMON_SOURCES})

target_link_libraries(receiver PRIVATE
    fep3_participant_cpp
    dev_essential::filesystem
    dev_essential::pkg_rpc
    clipp::clipp)

set_target_PROPERTIES(receiver PROPERTIES FOLDER test/determinism/participants)
set_target_properties(receiver PROPERTIES INSTALL_RPATH "$ORIGIN")
add_dependencies(receiver fep_participant_file_copy_tester_determinism)
