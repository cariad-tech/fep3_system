# Copyright 2024 CARIAD SE. 
#
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

add_library(service_bus_helper STATIC
                                src/participant_shutdown_listener.cpp
                                include/participant_shutdown_listener.h)

target_compile_definitions(service_bus_helper PRIVATE FEP3_SYSTEM_LIB_DO_EXPORT)

target_include_directories(service_bus_helper
                           PUBLIC ./include
                                  ##for fep3 Timestamp
                                  ${fep3_participant_INCLUDE_DIR}/
                                  ## for fep_rpc_client
                                  ${fep3_participant_INCLUDE_DIR}/fep3
                                  ${fep3_participant_INCLUDE_DIR}/fep3/components/service_bus
                                  ${PROJECT_BINARY_DIR}/include
                                  ${PROJECT_BINARY_DIR}/include/fep_system
                                  ${PROJECT_BINARY_DIR}/src/fep_system
                                  ${PROJECT_BINARY_DIR}/src/fep_system/fep_system_stubs
                                  ${PROJECT_SOURCE_DIR}/include/
                                  ${PROJECT_SOURCE_DIR}/include/fep_system)
set_target_properties(service_bus_helper PROPERTIES FOLDER "system_library/base")
target_link_libraries(service_bus_helper PUBLIC dev_essential::result)
