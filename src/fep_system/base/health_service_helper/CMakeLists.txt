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

add_library(health_service_helper STATIC
                                src/participant_health_aggregator.cpp
                                src/participant_health_listener.cpp
                                include/participant_health_aggregator.h
                                include/participant_health_listener.h)

target_compile_definitions(health_service_helper PRIVATE FEP3_SYSTEM_LIB_DO_EXPORT)

target_include_directories(health_service_helper
                           PUBLIC ./include
                                  ##for fep3 Timestamp
                                  ${fep3_participant_INCLUDE_DIR}/
                                  ## for fep_rpc_client
                                  ${fep3_participant_INCLUDE_DIR}/fep3
                                  ${fep3_participant_INCLUDE_DIR}/fep3/components/service_bus
                                  ${fep3_participant_INCLUDE_DIR}/fep3/components/health_service
                                  ${PROJECT_BINARY_DIR}/include
                                  ${PROJECT_BINARY_DIR}/include/fep_system
                                  ${PROJECT_BINARY_DIR}/src/fep_system
                                  ${PROJECT_BINARY_DIR}/src/fep_system/fep_system_stubs
                                  ${PROJECT_SOURCE_DIR}/include/
                                  ${PROJECT_SOURCE_DIR}/include/fep_system)
set_target_properties(health_service_helper PROPERTIES FOLDER "system_library/base")
target_link_libraries(health_service_helper PUBLIC dev_essential::result Boost::headers)
