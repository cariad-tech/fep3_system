# Copyright 2023 CARIAD SE. 
#
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#
#

find_package(Boost REQUIRED COMPONENTS filesystem)

set(TESTER_PERFORMANCE_HEALTH_SERVICE tester_performance_health_service)

add_executable(${TESTER_PERFORMANCE_HEALTH_SERVICE}
    src/tester_performance_health_service_main.cpp
    src/tester_performance_health_service.cpp
    src/test_environment.h
    )
target_link_libraries(${TESTER_PERFORMANCE_HEALTH_SERVICE} PRIVATE
    Boost::filesystem
    GTest::gtest
    fep3_system
    system_test_utils
    fep3_system_test
    clipp::clipp
    )
target_compile_definitions(${TESTER_PERFORMANCE_HEALTH_SERVICE} PRIVATE
    PARTICIPANT_EXECUTABLE_NAME="$<TARGET_FILE:basic_participant>"
    )

set_target_PROPERTIES(${TESTER_PERFORMANCE_HEALTH_SERVICE} PROPERTIES FOLDER test/performance/health_service)
gtest_discover_tests(${TESTER_PERFORMANCE_HEALTH_SERVICE}
     WORKING_DIRECTORY $<TARGET_FILE_DIR:${TESTER_PERFORMANCE_HEALTH_SERVICE}>
     )
# Could use generator expression $<$<CXX_COMPILER_ID:MSVC>:"${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)"> but CMake 3.18 has a bug that prevents it,
# see https://gitlab.kitware.com/cmake/cmake/-/issues/21074
# Therefore we make use of the old style:
if(WIN32)
    set_target_properties(${TESTER_PERFORMANCE_HEALTH_SERVICE} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)")
endif()
set_target_properties(${TESTER_PERFORMANCE_HEALTH_SERVICE} PROPERTIES INSTALL_RPATH "$ORIGIN")

internal_fep3_participant_create_copy_target(${TESTER_PERFORMANCE_HEALTH_SERVICE})
fep3_system_create_copy_target(${TESTER_PERFORMANCE_HEALTH_SERVICE})
set_target_properties(fep_system_file_copy_${TESTER_PERFORMANCE_HEALTH_SERVICE} PROPERTIES FOLDER test/performance/health_service/copy_helpers)
add_dependencies(${TESTER_PERFORMANCE_HEALTH_SERVICE} fep_system_file_copy_${TESTER_PERFORMANCE_HEALTH_SERVICE})
