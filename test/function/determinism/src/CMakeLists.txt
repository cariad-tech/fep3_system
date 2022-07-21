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
fep3_participant_create_copy_target(tester_determinism)
fep3_system_create_copy_target(tester_determinism)
set_target_properties(fep_system_file_copy_tester_determinism PROPERTIES FOLDER test/determinism/copy_helpers)
set_target_properties(fep_participant_file_copy_tester_determinism PROPERTIES FOLDER test/determinism/copy_helpers)

find_package(fep3_participant_cpp REQUIRED)
find_package(Boost 1.73.0 REQUIRED COMPONENTS filesystem)

find_package(dev_essential REQUIRED COMPONENTS process filesystem)

##################################################################
# RPC
##################################################################

jsonrpc_generate_server_stub(event_listener/event_listener.json
                             fep3::rpc_stubs::RPCEventListenerServiceStub
                             ${CMAKE_CURRENT_BINARY_DIR}/event_listener_service_stub.h)

jsonrpc_generate_client_stub(event_listener/event_listener.json
                             fep3::rpc_stubs::RPCEventListenerClientStub
                             ${CMAKE_CURRENT_BINARY_DIR}/event_listener_client_stub.h)

jsonrpc_generate_server_stub(event_listener/event_listener_registration.json
                             fep3::rpc_stubs::RPCEventListenerRegistrationServiceStub
                             ${CMAKE_CURRENT_BINARY_DIR}/event_listener_registration_service_stub.h)

jsonrpc_generate_client_stub(event_listener/event_listener_registration.json
                             fep3::rpc_stubs::RPCEventListenerRegistrationClientStub
                             ${CMAKE_CURRENT_BINARY_DIR}/event_listener_registration_client_stub.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

##################################################################
# tester_determinism
##################################################################
add_executable(tester_determinism
    determinism.cpp
    event_listener/gmock_async_helper.h
    event_listener/event_listener.h
    event_listener/event_listener.cpp
    event_listener/event_listener_rpc_intf.h
    ${CMAKE_CURRENT_BINARY_DIR}/event_listener_service_stub.h
    ${CMAKE_CURRENT_BINARY_DIR}/event_listener_registration_client_stub.h)

#needed for boost::process, since process is not defined as target and is header only
message("BOOST INCLUDE DIR PROCESS ${Boost_INCLUDE_DIR}")
target_include_directories(tester_determinism PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(tester_determinism PRIVATE
    GTest::gtest_main
    fep3_system
    fep3_participant_core
    dev_essential::process
    dev_essential::pkg_rpc
    dev_essential::filesystem
    Boost::filesystem
    Boost::disable_autolinking)

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
    determinism_helper.hpp
    clock/local_system_clock_discrete.h
    clock/local_system_clock_discrete.cpp
    event_listener/event_listener_registration_impl.h
    event_listener/event_listener_registration_impl.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/event_listener_client_stub.h
    ${CMAKE_CURRENT_BINARY_DIR}/event_listener_registration_service_stub.h)

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
    dev_essential::pkg_rpc)

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
    dev_essential::pkg_rpc)

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
    dev_essential::pkg_rpc)

set_target_PROPERTIES(receiver PROPERTIES FOLDER test/determinism/participants)
set_target_properties(receiver PROPERTIES INSTALL_RPATH "$ORIGIN")
add_dependencies(receiver fep_participant_file_copy_tester_determinism)
