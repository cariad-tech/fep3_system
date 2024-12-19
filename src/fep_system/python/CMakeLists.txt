# Copyright 2023 CARIAD SE. 
# 
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

find_package(Python ${fep3_system_cmake_python_version} EXACT
                    REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)
find_package(Boost 1.73.0 REQUIRED COMPONENTS date_time)

pybind11_add_module(fep3_system_py fep_system_py.cpp)

target_link_libraries(fep3_system_py 
        PRIVATE
        fep3_system
        dev_essential::strings
        Boost::date_time)

target_sources(fep3_system_py PRIVATE event_monitor.py)
set_target_properties(fep3_system_py PROPERTIES FOLDER system_library
                                                OUTPUT_NAME fep3_system)

# copy event_monitor.py for integrated tests
add_custom_command(TARGET fep3_system_py POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           ${CMAKE_CURRENT_LIST_DIR}/event_monitor.py $<TARGET_FILE_DIR:fep3_system_py>)

install(FILES event_monitor.py DESTINATION lib)
install(TARGETS fep3_system_py EXPORT fep3_system_export DESTINATION lib)
