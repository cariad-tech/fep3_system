# Copyright 2023 CARIAD SE. 
# 
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
add_subdirectory(utils)
add_definitions(-D_TEST_MACROS_INCLUDES_)
add_subdirectory(system)
add_subdirectory(determinism/src)
add_subdirectory(performance)

if(fep3_system_cmake_python_version)
    add_subdirectory(python/src)
endif()
