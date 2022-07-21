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
macro(fep3_system_set_folder NAME FOLDER)
    set_property(TARGET ${NAME} PROPERTY FOLDER ${FOLDER})
endmacro(fep3_system_set_folder NAME FOLDER)

################################################################################
## \page page_cmake_commands
# <hr>
# <b>fep_install(\<name\> \<destination\>)</b>
#
# This macro installs the target \<name\>, together with the FEP SDK libraries (if neccessary)
#   to the folder \<destination\>
# Arguments:
# \li \<name\>:
# The name of the library to install.
# \li \<destination\>:
# The relative path to the install subdirectory
################################################################################
macro(fep3_system_install NAME DESTINATION)
    install(TARGETS ${NAME} DESTINATION ${DESTINATION})

    install(FILES $<TARGET_FILE:fep3_system> DESTINATION ${DESTINATION})
    install(FILES $<TARGET_FILE_DIR:fep3_system>/fep3_system.plugins DESTINATION ${DESTINATION})
    install(FILES $<TARGET_FILE:fep3_http_service_bus> DESTINATION ${DESTINATION}/http)

    if(MSVC)
        install(FILES $<TARGET_FILE_DIR:fep3_system>/fep3_system$<$<CONFIG:Debug>:d>${fep3_system_pdb_version_str}.$<$<CONFIG:Debug>:pdb> DESTINATION ${DESTINATION} CONFIGURATIONS Debug)
        install(FILES $<TARGET_FILE_DIR:fep3_http_service_bus>/fep3_http_service_bus.$<$<CONFIG:Debug>:pdb> DESTINATION ${DESTINATION}/http CONFIGURATIONS Debug)
    endif()
endmacro(fep3_system_install NAME DESTINATION)

################################################################################
## \page page_cmake_commands
# <hr>
# <b>fep3_system_deploy_helper(\<name\> \<target_folder\>)</b>
#
# This macro deploys the target the FEP System libraries (if neccessary)
#   to the folder \<target_folder\>. Use it to copy library
#   (and pdbs for MSVC) into the current target destination folder, which is 
#   either the build folder, or the package folder
# DO NOT USE THIS IF THE DESTINATION FOLDER IS USED IN MORE THAN ONE TARGETS.
# see https://cmake.org/cmake/help/v3.0/command/add_custom_command.html
# In this case, use fep3_system_deploy_test_target instead.
# Arguments:
# \li \<name\>:
# The name of the target to install.
################################################################################
macro(fep3_system_deploy_helper NAME TARGET_FOLDER)
    # no need to copy in build directory on linux since linker rpath takes care of that
    if (WIN32)
        add_custom_command(TARGET ${NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:fep3_system>" "${TARGET_FOLDER}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${TARGET_FOLDER}/http"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:fep3_http_service_bus>" "${TARGET_FOLDER}/http"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:fep3_system>/fep3_system.plugins" "${TARGET_FOLDER}"
        )
        if(MSVC)
            add_custom_command(TARGET ${NAME} POST_BUILD
                COMMAND "$<$<CONFIG:Debug>:${CMAKE_COMMAND}>$<$<NOT:$<CONFIG:Debug>>:echo>" $<$<CONFIG:Debug>:-E copy_if_different> "$<TARGET_FILE_DIR:fep3_system>/fep3_system$<$<CONFIG:Debug>:d>${fep3_system_pdb_version_str}.$<$<CONFIG:Debug>:pdb>" "${TARGET_FOLDER}"
                COMMAND "$<$<CONFIG:Debug>:${CMAKE_COMMAND}>$<$<NOT:$<CONFIG:Debug>>:echo>" $<$<CONFIG:Debug>:-E copy_if_different> "$<TARGET_FILE_DIR:fep3_http_service_bus>/fep3_http_service_bus.$<$<CONFIG:Debug>:pdb>" "${TARGET_FOLDER}/http"
            )
        endif()
    endif()


    set_target_properties(${NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
endmacro(fep3_system_deploy_helper NAME)

################################################################################
## \page page_cmake_commands
# <hr>
# <b>fep3_system_deploy(\<name\>)</b>
#
# This macro deploys the target the FEP System libraries (if neccessary)
#   to the folder of target directory of \<name\>. Use it to copy library
#   (and pdbs for MSVC) into the current target destination folder, which is
#   either the build folder, or the package folder
# DO NOT USE THIS IF THE DESTINATION FOLDER IS USED IN MORE THAN ONE TARGETS.
# see https://cmake.org/cmake/help/v3.0/command/add_custom_command.html
# In this case, use fep3_system_deploy_test_target instead.
# Arguments:
# \li \<name\>:
# The name of the target to install.
################################################################################
macro(fep3_system_deploy NAME)
    fep3_system_deploy_helper(${NAME}  $<TARGET_FILE_DIR:${NAME}>)
endmacro(fep3_system_deploy NAME)

################################################################################
## \page page_cmake_commands
# <hr>
# <b>fep3_system_deploy_test_target(NAME \<TEST_COPY_TARGET\>)</b>
#
# This macro deploys the fep system library to the CMAKE_CURRENT_BINARY_DIR folder appended
# with the current configuration name
# see https://cmake.org/cmake/help/v3.0/command/add_custom_command.html
# Arguments:
# \li \<name\>:
# The name of the target to obtain the folder where to copy the fep system library
# binaries to.
################################################################################
macro(fep3_system_deploy_test_target TEST_COPY_TARGET)
    # no need to copy in build directory on linux since linker rpath takes care of that
    if (WIN32)
        add_custom_command(TARGET ${TEST_COPY_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
        )
    endif()
    fep3_system_deploy_helper(${TEST_COPY_TARGET}  ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
endmacro(fep3_system_deploy_test_target TEST_COPY_TARGET)

macro(fep3_system_create_copy_target TEST_SUITE)

    ##################################################################
    # fep_system_file_copy
    ##################################################################
    add_custom_target(fep_system_file_copy_${TEST_SUITE})
    fep3_system_deploy_test_target(fep_system_file_copy_${TEST_SUITE})

endmacro(fep3_system_create_copy_target TEST_SUITE)