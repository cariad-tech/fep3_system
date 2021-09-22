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
# <b>fep3_system_deploy(\<name\>)</b>
#
# This macro deploys the target the FEP System libraries (if neccessary)
#   to the folder of target directory of \<name\>. Use it to copy library
#   (and pdbs for MSVC) into the current target destination folder, which is 
#   either the build folder, or the package folder
# Arguments:
# \li \<name\>:
# The name of the target to install.
################################################################################
macro(fep3_system_deploy NAME)
    # no need to copy in build directory on linux since linker rpath takes care of that
    if (WIN32)
        add_custom_command(TARGET ${NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:fep3_system>" "$<TARGET_FILE_DIR:${NAME}>"
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${NAME}>/http"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:fep3_http_service_bus>" "$<TARGET_FILE_DIR:${NAME}>/http"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE_DIR:fep3_system>/fep3_system.plugins" "$<TARGET_FILE_DIR:${NAME}>"
        )
        if(MSVC)
            add_custom_command(TARGET ${NAME} POST_BUILD
                COMMAND "$<$<CONFIG:Debug>:${CMAKE_COMMAND}>$<$<NOT:$<CONFIG:Debug>>:echo>" $<$<CONFIG:Debug>:-E copy_if_different> "$<TARGET_FILE_DIR:fep3_system>/fep3_system$<$<CONFIG:Debug>:d>${fep3_system_pdb_version_str}.$<$<CONFIG:Debug>:pdb>" "$<TARGET_FILE_DIR:${NAME}>"
                COMMAND "$<$<CONFIG:Debug>:${CMAKE_COMMAND}>$<$<NOT:$<CONFIG:Debug>>:echo>" $<$<CONFIG:Debug>:-E copy_if_different> "$<TARGET_FILE_DIR:fep3_http_service_bus>/fep3_http_service_bus.$<$<CONFIG:Debug>:pdb>" "$<TARGET_FILE_DIR:${NAME}>/http"
            )
        endif()
    endif()


    set_target_properties(${NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
endmacro(fep3_system_deploy NAME)

