#
# Copyright @ 2022 VW Group. All rights reserved.
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

if(fep3_system_test_FOUND)
    return()
endif()

# gtest and gmock is required for target fep3_components_test
find_package(GTest CONFIG REQUIRED COMPONENTS gtest gtest_main gmock gmock_main)

include(${CMAKE_CURRENT_LIST_DIR}/lib/cmake/fep3_system_test_targets.cmake)

set(fep3_system_test_FOUND true)
