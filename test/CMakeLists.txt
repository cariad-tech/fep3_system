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
option(fep3_system_cmake_enable_functional_tests
       "Enable functional tests - requires googletest (default: OFF)" OFF)
option(fep3_system_cmake_enable_private_tests
       "Enable private tests - requires googletest (default: OFF)" OFF)

if(NOT FEP3_TESTS_INTEGRATED)
    cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    if (NOT DEFINED CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 17)
    endif()

    cmake_policy(SET CMP0002 NEW)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0011 NEW)

    project(fep-sdk-system-library-tests)

    # Enable project folder structure for Visual Studio IDE
    set_property(GLOBAL PROPERTY USE_FOLDERS true)

    include(../scripts/cmake/enable_multicore_compilation.cmake)
    include(../scripts/cmake/use_integrated_debug_symbols.cmake)
    include(../scripts/cmake/set_boost_interface_targets.cmake)

    include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
    set (FEP_POST_BUILD_COPY true)

    find_package(fep3_system REQUIRED)
    find_package(fep3_system_test REQUIRED)
else()
    set(fep3_system_DIR ${CMAKE_INSTALL_PREFIX})
endif()

enable_testing()
if(NOT TARGET fep3_participant_core)
    find_package(fep3_participant_core REQUIRED)
endif()


find_package(GTest REQUIRED COMPONENTS gtest gtest_main gmock gmock_main)

# for test_setup.h
include_directories(${CMAKE_CURRENT_LIST_DIR})

# tests have access to internal header
include_directories(../src)

set(CMAKE_BUILD_WITH_INSTALL_RPATH_BAK ${CMAKE_BUILD_WITH_INSTALL_RPATH})
if (UNIX AND FEP3_TESTS_INTEGRATED)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH OFF)
endif()

if(fep3_system_cmake_enable_functional_tests)
    add_subdirectory(function)
endif()

if(fep3_system_cmake_enable_private_tests)
    add_subdirectory(private)
endif()

if (UNIX AND FEP3_TESTS_INTEGRATED)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ${CMAKE_BUILD_WITH_INSTALL_RPATH_BAK})
endif()

if (fep3_system_export_test_targets)
    ##################################################
    # setup a target for the fep3_system test library (header-only)
    ##################################################
    add_library(fep3_system_test INTERFACE)

    set(SYSTEM_TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/fep_system)
    target_sources(fep3_system_test INTERFACE
        "$<BUILD_INTERFACE:${SYSTEM_TEST_INCLUDE_DIR}/mock_event_monitor.h>"
    )
    target_include_directories(fep3_system_test INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>"
        "$<INSTALL_INTERFACE:test/include/>"
    )
    install(
        TARGETS fep3_system_test
        EXPORT fep3_system_test_targets
        LIBRARY NAMELINK_SKIP DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib
    )
    install(EXPORT fep3_system_test_targets DESTINATION lib/cmake)

    # install content from test/include directory
    install(
        DIRECTORY
            include
        DESTINATION
            ./test/
    )

    ###########################################
    # configure the install package
    ###########################################
    # Create version config
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/fep3_system_test-config-version.cmake
        COMPATIBILITY AnyNewerVersion)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fep3_system_test-config-version.cmake DESTINATION .)
    install(FILES fep3_system_test-config.cmake DESTINATION .)
endif()
