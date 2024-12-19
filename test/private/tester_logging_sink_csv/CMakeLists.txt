# Copyright 2023 CARIAD SE. 
# 
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

##################################################################
# tester_logging_sink_csv
##################################################################

set(test_name tester_logging_sink_csv)
add_executable(${test_name}
               ${test_name}.cpp
               ${PROJECT_SOURCE_DIR}/src/fep_system/logging_sink_csv.h)
find_package(Boost 1.73.0 REQUIRED COMPONENTS date_time filesystem)
find_package(dev_essential REQUIRED COMPONENTS strings)

target_link_libraries(${test_name}
                      PRIVATE
                      GTest::gtest_main
                      GTest::gmock_main
                      fep3_system
                      Boost::date_time
                      Boost::filesystem
                      dev_essential::strings)
get_filename_component(test_log_files_dir "./../files"
                       REALPATH  BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(${test_name}
            PRIVATE TEST_LOGFILES_DIR="${test_log_files_dir}"
        )
set_target_PROPERTIES(${test_name} PROPERTIES FOLDER test/fep_system/private)
gtest_discover_tests(${test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)
set_target_properties(${test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
