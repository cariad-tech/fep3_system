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

find_package(Python ${fep3_system_cmake_python_version} EXACT
                    REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(fep3_system_py fep_system_py.cpp)

target_link_libraries(fep3_system_py PRIVATE fep3_system)

target_sources(fep3_system_py PRIVATE event_monitor.py)
set_target_properties(fep3_system_py PROPERTIES FOLDER system_library
                                                OUTPUT_NAME fep3_system)

# copy event_monitor.py for integrated tests
add_custom_command(TARGET fep3_system_py POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                           ${CMAKE_CURRENT_LIST_DIR}/event_monitor.py $<TARGET_FILE_DIR:fep3_system_py>)

install(FILES event_monitor.py DESTINATION lib)
install(TARGETS fep3_system_py EXPORT fep3_system_export DESTINATION lib)
