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

internal_fep3_participant_create_copy_target(tester_system)
fep3_system_create_copy_target(tester_system)
set_target_properties(fep_system_file_copy_tester_system PROPERTIES FOLDER test/fep_system/copy_helpers)
set_target_properties(fep_participant_file_copy_tester_system PROPERTIES FOLDER test/fep_system/copy_helpers)

##################################################################
# tester_system_discovery
##################################################################
find_package(dev_essential REQUIRED COMPONENTS xml process)
find_package(Boost REQUIRED COMPONENTS filesystem)

add_executable(tester_system_discovery system_discovery.cpp)
target_link_libraries(tester_system_discovery
                      PRIVATE
                      GTest::gtest_main
                      fep3_system
                      fep3_participant_core
                      system_test_utils
                      dev_essential::xml
                      dev_essential::process
                      clipp::clipp
                      Boost::filesystem)

add_dependencies(tester_system_discovery fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(tester_system_discovery PROPERTIES FOLDER test/fep_system)
add_test(NAME tester_system_discovery_dds
         COMMAND tester_system_discovery --plugin dds_service_discovery
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
add_test(NAME tester_system_discovery_http
         COMMAND tester_system_discovery --plugin http_service_discovery
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(tester_system_discovery PROPERTIES INSTALL_RPATH "$ORIGIN")
target_include_directories(tester_system_discovery PRIVATE $<TARGET_FILE_DIR:tester_system_discovery>/include/)

if (MSVC)
    set_target_PROPERTIES(tester_system_discovery PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "--plugin dds_service_discovery")
    file(GENERATE
        OUTPUT $<TARGET_FILE_DIR:tester_system_discovery>/include/component_file_paths.h
        CONTENT [[
#pragma once

namespace
{
inline constexpr const char* components_file_path = "$<TARGET_FILE_DIR:tester_system_discovery>/fep3_participant.fep_components";
inline constexpr const char* system_plugin_file_path = "$<TARGET_FILE_DIR:tester_system_discovery>/fep3_system.fep_components";
}
    ]])
endif(MSVC)

if (UNIX)
        file(GENERATE
            OUTPUT $<TARGET_FILE_DIR:tester_system_discovery>/include/component_file_paths.h
            CONTENT [[
#pragma once

namespace
{
inline constexpr const char* components_file_path = "$<TARGET_FILE_DIR:tester_system_discovery>/$<CONFIG>/fep3_participant.fep_components";
inline constexpr const char* system_plugin_file_path = "$<TARGET_FILE_DIR:fep3_system>/fep3_system.fep_components";
}
    ]])
endif (UNIX)

##################################################################
# tester_system
##################################################################

set(_current_test_name tester_system)
add_executable(${_current_test_name} system.cpp)
target_link_libraries(${_current_test_name} PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    fep3_system
    fep3_participant_core
    system_test_utils
    dev_essential::process
    dev_essential::system
    Boost::headers
    )
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
# all the tests have a common working dir, so the header is generated for tester_system_discovery
target_include_directories(tester_system PRIVATE $<TARGET_FILE_DIR:tester_system_discovery>/include/)

add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)

set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
target_include_directories(tester_system PRIVATE  $<TARGET_FILE_DIR:tester_system>/include)


if (MSVC)
    file(GENERATE
            OUTPUT $<TARGET_FILE_DIR:tester_system>/files/comp_file/fep3_system.fep_components
            CONTENT [[

    <components xmlns="http://fep.vwgroup.com/fep_sdk/3.0/components">
        <schema_version>1.0.0</schema_version>
        <component>
            <source type="cpp-plugin">
                $<TARGET_FILE_DIR:tester_system>/rti/fep3_dds_service_bus_plugin
            </source>
            <iid>service_bus.catelyn.fep3.iid</iid>
        </component>
    </components>

            ]])

endif(MSVC)

if (UNIX)
    file(GENERATE
            OUTPUT $<TARGET_FILE_DIR:tester_system>/files/comp_file/fep3_system.fep_components
            CONTENT [[

    <components xmlns="http://fep.vwgroup.com/fep_sdk/3.0/components">
        <schema_version>1.0.0</schema_version>
        <component>
            <source type="cpp-plugin">
                $<TARGET_FILE_DIR:tester_system>/$<CONFIG>/rti/fep3_dds_service_bus_plugin
            </source>
            <iid>service_bus.catelyn.fep3.iid</iid>
        </component>
    </components>

            ]])
endif (UNIX)

file(GENERATE
        OUTPUT $<TARGET_FILE_DIR:tester_system>/include/component_file_path_from_env_var.h
        CONTENT [[
#pragma once

namespace
{
inline constexpr const char* component_file_path_from_env_var = "$<TARGET_FILE_DIR:tester_system>/files/comp_file/fep3_system.fep_components";

}
        ]])




##################################################################
# tester_system_library_participant
##################################################################

set(_current_test_name tester_system_library_participant)
add_executable(${_current_test_name} participant.cpp)
target_link_libraries(${_current_test_name} PRIVATE
    GTest::gtest_main
    fep3_system
    fep3_participant_core
    system_test_utils
    dev_essential::process
)
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
add_executable(${_current_test_name} participant_configuration.cpp)
target_link_libraries(${_current_test_name} PRIVATE
    GTest::gmock
    GTest::gtest_main
    fep3_system
    fep3_participant_core
    system_test_utils
    fep3_system_test
    dev_essential::process
)
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
add_executable(${_current_test_name} system_logging.cpp)
target_link_libraries(${_current_test_name} PRIVATE
    GTest::gtest_main
    fep3_system
    fep3_participant_core
    system_test_utils
    dev_essential::process
)
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
add_executable(${_current_test_name} participant_data_registry.cpp)
target_link_libraries(${_current_test_name} PRIVATE
    GTest::gtest_main
    fep3_system
    fep3_participant_core
    system_test_utils
    fep3_system_test
    dev_essential::process
)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
     COMMAND ${_current_test_name}
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

##################################################################
# tester_system_library_health
##################################################################
find_package(fep3_participant_cpp REQUIRED fep3_participant_cpp)

set(_current_test_name tester_system_library_health)
add_executable(${_current_test_name} system_health.cpp)
target_link_libraries(${_current_test_name} PRIVATE
    GTest::gtest_main
    fep3_system
    fep3_participant_cpp
    fep3_participant_core
    system_test_utils
    dev_essential::process
)
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
# tester_system_library_http_server
##################################################################

set(_current_test_name tester_system_library_http_server)
add_executable(${_current_test_name} system_http_server.cpp)
target_link_libraries(${_current_test_name} PRIVATE
    GTest::gtest_main
    fep3_system
    fep3_participant_core
    system_test_utils
    dev_essential::process
)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

##################################################################
# tester_rpc_passthrough
##################################################################

set(_current_test_name tester_rpc_passthrough)
add_executable(${_current_test_name} rpc_passthrough.cpp)
target_link_libraries(${_current_test_name} PRIVATE
    GTest::gtest_main
    fep3_system
    fep3_participant_core
    system_test_utils
    fep3_system_test
    dev_essential::process
)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_system fep_participant_file_copy_tester_system)
set_target_PROPERTIES(${_current_test_name} PROPERTIES FOLDER test/fep_system)
add_test(NAME ${_current_test_name}
         COMMAND ${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
 target_compile_definitions(${_current_test_name} PRIVATE
     TEST_BUILD_DIR="${PARENT_DIR}/files/fep3_participant_$<CONFIG>.fep_components"
 )

set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")

