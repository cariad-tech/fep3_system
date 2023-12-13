# See https://cmake.org/cmake/help/latest/module/FindBoost.html#imported-targets
find_package(Boost REQUIRED)

option(fep3_system_cmake_boost_diagnostic_definitions
        "Enable/disable diagnostic information about Boost's automatic linking \
during compilation (adds -DBOOST_LIB_DIAGNOSTIC). (default: OFF)"
        OFF)
option(fep3_system_cmake_boost_disable_autolinking
        "Disable automatic linking with MSVC (adds -DBOOST_ALL_NO_LIB). (default: ON)"
        ON)
option(fep3_system_cmake_boost_dynamic_linking
        "Interface target to enable dynamic linking with MSVC (adds -DBOOST_ALL_DYN_LINK).\
 (default: OFF)"
        OFF)

# Boost::headers is the base target every Boost::<component> depends on
# Injecting the meta targets to its interface will (hopefully) pass them to the consuming targets
target_link_libraries(Boost::headers INTERFACE
                      "$<$<BOOL:${fep3_system_cmake_boost_diagnostic_definitions}>:Boost::diagnostic_definitions>"
                      "$<$<BOOL:${fep3_system_cmake_boost_disable_autolinking}>:Boost::disable_autolinking>"
                      "$<$<BOOL:${fep3_system_cmake_boost_dynamic_linking}>:Boost::dynamic_linking>")
