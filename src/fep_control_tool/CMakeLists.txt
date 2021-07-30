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
################################################################
# as long as FEPSDK-FEPSDK-2196 we must do that hack
include_directories(CONAN_FEP_CONTROLLER_ROOT)
################################################################

find_package(fep3_system REQUIRED)

set(FEP_CONTROL_TOOL fep_control)

find_package(dev_essential REQUIRED COMPONENTS pkg_rpc xml filesystem)
find_package(Boost 1.73.0 REQUIRED COMPONENTS date_time) # beast since 1.66.0

add_executable(${FEP_CONTROL_TOOL}
    linenoise/linenoise.h
    linenoise/linenoise.c
    linenoise/utf8.h
    linenoise/utf8.c
    linenoise_wrapper.h
    linenoise_wrapper.cpp
    control_tool_common_helper.h
    helper.h
    helper.cpp
    fep_control.h
    fep_control.cpp
    fep_control_commandline.h
    fep_control_commandline.cpp
    fep_control_websocket.h
    fep_control_websocket.cpp
    fep_control_tool.cpp
    monitor.h
    monitor.cpp
)

target_link_libraries(${FEP_CONTROL_TOOL}
    PUBLIC fep3_system
    PRIVATE dev_essential::pkg_rpc
    dev_essential::xml
    dev_essential::filesystem
    Boost::date_time Boost::disable_autolinking
)

if (MSVC)
else ()
target_link_libraries(${FEP_CONTROL_TOOL}
PUBLIC
    pthread
    ${CMAKE_DL_LIBS})
endif()

install(TARGETS ${FEP_CONTROL_TOOL}
        RUNTIME DESTINATION bin)

install(FILES linenoise/linenoise.h
        DESTINATION doc/license/used_licenses/linenoise
        CONFIGURATIONS Release Debug RelWithDebInfo
        RENAME linenoise-license.txt
    )

fep3_system_install(${FEP_CONTROL_TOOL} bin)
set(fep3_system_no_copy_pluginfile True)
fep3_system_deploy(${FEP_CONTROL_TOOL})

set_target_properties(${FEP_CONTROL_TOOL} PROPERTIES INSTALL_RPATH "$ORIGIN")
set_target_properties(${FEP_CONTROL_TOOL} PROPERTIES FOLDER tools)

target_include_directories(${FEP_CONTROL_TOOL} PRIVATE ${PROJECT_SOURCE_DIR}/src)
