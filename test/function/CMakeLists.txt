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
add_subdirectory(utils)
add_definitions(-D_TEST_MACROS_INCLUDES_)
add_subdirectory(system)
add_subdirectory(determinism/src)
add_subdirectory(performance)

if(fep3_system_cmake_python_version)
    add_subdirectory(python/src)
endif()
