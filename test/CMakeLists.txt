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

if(NOT FEP3_TESTS_INTEGRATED)
    cmake_minimum_required(VERSION 3.17.0 FATAL_ERROR)

    # Disable extensions here and require the chosen CMAKE_CXX_STANDARD (coming from e.g. Conan)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    if (NOT DEFINED CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 17)
    endif()

    cmake_policy(SET CMP0002 NEW)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0011 NEW)

    project(fep-sdk-system-library-tests)

    if(CONAN_COMPILER)
        message(STATUS "Including Conan build info")
        if ( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
            include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
        elseif ( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/../conanbuildinfo.cmake)
            include(${CMAKE_CURRENT_BINARY_DIR}/../conanbuildinfo.cmake)
        elseif ( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
            include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
        elseif ( EXISTS ${CMAKE_CURRENT_BINARY_DIR}/../conanbuildinfo_multi.cmake)
            include(${CMAKE_CURRENT_BINARY_DIR}/../conanbuildinfo_multi.cmake)
        else()
            message(FATAL_ERROR "Conan build info can't be found.")
        endif()

        if(CORTEX_WORKSPACE)
            conan_basic_setup(TARGETS)
        else()
            conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
        endif()
        set(fep3_system_DIR ${CONAN_FEP_SDK_SYSTEM_ROOT})
        set(fep3_participant_DIR ${CONAN_FEP_SDK_PARTICIPANT_ROOT})
    endif()

    # Enable project folder structure for Visual Studio IDE
    set_property(GLOBAL PROPERTY USE_FOLDERS true)

    include(../scripts/cmake/enable_multicore_compilation.cmake)
    include(../scripts/cmake/use_integrated_debug_symbols.cmake)

    include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
    set (FEP_POST_BUILD_COPY true)

    find_package(fep3_system REQUIRED)
else()
    set(fep3_system_DIR ${CMAKE_INSTALL_PREFIX})
endif()

enable_testing()
find_package(fep3_participant_core REQUIRED CONFIG PATHS ${PARTICIPANT_LIB_DIR})

find_package(GTest CONFIG REQUIRED COMPONENTS gtest gtest_main gmock gmock_main)

include(FEP_TEST_PRIVATE.cmake)

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

if (UNIX AND FEP3_TESTS_INTEGRATED)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ${CMAKE_BUILD_WITH_INSTALL_RPATH_BAK})
endif()
