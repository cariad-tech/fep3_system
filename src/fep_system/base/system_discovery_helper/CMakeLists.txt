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

add_library(system_discovery_helper STATIC src/system_discovery_helper.cpp
                                           include/system_discovery_helper.h)
target_include_directories(system_discovery_helper
                           PUBLIC ./include
                                  ##for fep3_errors and fep3_participant_types.h
                                  ${fep3_participant_INCLUDE_DIR}
                                  ${fep3_participant_INCLUDE_DIR}/fep3/components/service_bus)
set_target_properties(system_discovery_helper PROPERTIES FOLDER "system_library/base")
target_link_libraries(system_discovery_helper PUBLIC dev_essential::result
                                              PRIVATE dev_essential::strings
                                                      Boost::headers)
