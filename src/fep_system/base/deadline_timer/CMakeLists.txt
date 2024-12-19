# Copyright 2023 CARIAD SE. 
#
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

find_package(Boost 1.73.0 REQUIRED COMPONENTS date_time thread headers)

add_library(fep3_deadline_timer STATIC
src/deadline_timer.cpp
src/boost_asio_timer.h
include/deadline_timer.h
)

target_include_directories(fep3_deadline_timer
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(fep3_deadline_timer PROPERTIES FOLDER "system_library/base")

target_link_libraries(fep3_deadline_timer
    PRIVATE
    Boost::date_time  # check if needed
    Boost::headers  # check if needed
    Boost::thread)

target_compile_definitions(fep3_deadline_timer PUBLIC BOOST_THREAD_VERSION=4 BOOST_THREAD_PROVIDES_EXECUTORS)