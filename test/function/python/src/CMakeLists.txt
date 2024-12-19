# Copyright 2023 CARIAD SE. 
#
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#
#

internal_fep3_participant_create_copy_target(tester_python)
fep3_system_create_copy_target(tester_python)
set_target_properties(fep_system_file_copy_tester_python PROPERTIES FOLDER test/python/copy_helpers)
set_target_properties(fep_participant_file_copy_tester_python PROPERTIES FOLDER test/python/copy_helpers)

##################################################################
# tester_system_pybind
##################################################################
set(_current_test_name tester_python_pybind)
#find_package (Python ${fep3_system_cmake_python_version} EXACT REQUIRED COMPONENTS Interpreter)
add_executable(${_current_test_name} tester_system.cpp)
target_link_libraries(${_current_test_name} PRIVATE fep3_system
                                                    system_test_utils
                                                    GTest::gtest_main
                                                    fep3_participant_core)
add_dependencies(${_current_test_name} fep_system_file_copy_tester_python
                                       fep_participant_file_copy_tester_python)
set_target_properties(${_current_test_name} PROPERTIES FOLDER test/python)

target_sources(${_current_test_name} PRIVATE tester_system.py)

if(WIN32)
    file(TO_CMAKE_PATH "${Python_ROOT_DIR}/activate" ACTIVATE_VENV_PATH)
else()
    # for linux builds source activate script
    file(TO_CMAKE_PATH ". ${Python_ROOT_DIR}/activate" ACTIVATE_VENV_PATH)
endif()

target_compile_definitions(${_current_test_name} PRIVATE PYTESTS_TESTS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"
                                                         ACTIVATE_VENV_EXEC=\"${ACTIVATE_VENV_PATH}\"
                                                         WORKING_DIRECTORY=\"$<TARGET_FILE_DIR:${_current_test_name}>\")
add_custom_command(TARGET ${_current_test_name} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           $<TARGET_FILE:fep3_system_py> $<TARGET_FILE_DIR:${_current_test_name}>
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           $<TARGET_FILE_DIR:fep3_system_py>/event_monitor.py $<TARGET_FILE_DIR:${_current_test_name}>)

gtest_discover_tests(${_current_test_name}
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../)

set_target_properties(${_current_test_name} PROPERTIES INSTALL_RPATH "$ORIGIN")
set_target_properties(${_current_test_name} PROPERTIES TIMEOUT 120)
