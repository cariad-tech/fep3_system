# Copyright 2023 CARIAD SE. 
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
if(NOT TARGET dev_essential)
    find_package(dev_essential REQUIRED result)
endif()

find_package(Boost REQUIRED thread)
add_library(state_transition_helper STATIC
                                include/execution_policy.h
                                include/fep_system_state_transition.h
                                include/fep_system_state_tree.h
                                include/fep_system_state_transition_helper.h
                                include/fep_system_state_transition_error_handling.h
                                include/fep_state_transition_controller.h
                                include/shutdown_helper.h
                                include/execution_timer.h
                                src/fep_system_state_transition_error_handling.cpp
                                src/fep_system_state_transition_helper.cpp
                                src/fep_system_state_tree.cpp
                                src/fep_state_transition_controller.cpp)

target_include_directories(state_transition_helper
                           PUBLIC ./include
                           # for participant state
                           ${fep3_participant_INCLUDE_DIR}/fep3/rpc_services/participant_statemachine
                           #for logging
                            ${fep3_participant_INCLUDE_DIR}/fep3
                           ${PROJECT_SOURCE_DIR}/include/
                           PRIVATE  ${fep3_participant_INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/src/fep_system )
set_target_properties(state_transition_helper PROPERTIES FOLDER "system_library/base")
target_link_libraries(state_transition_helper PUBLIC Boost::headers dev_essential::result fep3_deadline_timer)
