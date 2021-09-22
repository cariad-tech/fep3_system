<!--
  Copyright @ 2021 VW Group. All rights reserved.
  
      This Source Code Form is subject to the terms of the Mozilla
      Public License, v. 2.0. If a copy of the MPL was not distributed
      with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
  
  If it is not possible or desirable to put the notice in a particular file, then
  You may include the notice in a location (such as a LICENSE file in a
  relevant directory) where a recipient would be likely to look for such a notice.
  
  You may add additional accurate notices of copyright ownership.
  
  -->

FEP 3 SDK - System Library
============================

## Description ##

This installed package contains (or pulls in) the FEP SDK - System Library

The FEP System Library connects the FEP Service Bus to discover and configure and control a system.

## How to use ###

The FEP System Library provides a CMake >= 3.17 configuration. Here's how to use it from your own CMake projects:

    find_package(fep3_system REQUIRED)

You can append the FEP System Libray Target to your existing targets to add a dependency:

    target_link_libraries(my_existing_target PRIVATE fep3_system)
    #this helper macro will install ALL necessary library dependency to the targets directory
    fep3_system_install(my_existing_target destination_path)

    #this helper macro will deploy as post build process ALL necessary library dependency to the targets build directory
    fep3_system_deploy(my_existing_target)

Note that the fep_system target will transitively pull in all required include directories and libraries.

## How to build using only cmake ###
### Prerequisites
- Download [CMake](https://cmake.org/) at least in version 3.17.0
- Using [git](https://git-scm.com/), clone the repository and checkout the desired branch (e.g. `master`)
- <a id="howtodevessential"></a> Build the dev_essential library as described in the [Readme](https://www.github.com/dev-essential) file.
- <a id="howtofep3participant"></a> Build the fep3_participant library as described in the [Readme](https://www.github.com/fep3_participant) file.
- <a id="howtoboost"></a> Boost in version 1.73, can be compiled from [sources](https://boostorg.jfrog.io/artifactory/main/release/1.73.0/source/) or the built binaries can be directly downloaded. For Windows [Boost 1.73](https://sourceforge.net/projects/boost/files/boost-binaries/1.73.0/boost_1_73_0-msvc-14.1-64.exe/download) can be downloaded, for Linux/Debian distributions apt can be used.

### Optional
-   <a id="howtogtest"></a> [Gtest](https://github.com/google/googletest) version 1.10 or newer.
    - Gtest has to be compiled from sources. After checking out the gtest github repository, run the following commands inside the checked out folder (depending on your compiler or the configuration to be built, the cmake command should be adapted accordingly). After executing the commands, <gtest_install_dir> will contain the built libraries. *gtest_force_shared_crt* flag is needed for windows in order compile with the correct Windows Runtime Library and avoid linking errors later when building the *fep system* library.

    ```shell
     > mkdir build
     > cd build
     > cmake -G "Visual Studio 16 2019" -A x64 -T v142
     -DCMAKE_INSTALL_PREFIX=<gtest_install_dir> -Dgtest_force_shared_crt=ON ../
     > cmake --build . --target install --config Release
    ```
- <a id="howtodoxygen"></a> [Doxygen](https://www.doxygen.nl/index.html) version 1.8.14. Doxygen executable should be located under <doxygen_dir>/bin
- <a id="howtosphinx"></a> [Sphinx](https://pypi.org/project/Sphinx/) version 3.3.0.
### Build with cmake
- Run the following command, (adaptations may be needed in case a different Visual Studio version is used or different configuration should be built).
    ```shell
    > cmake.exe -H<root_dir> -B<build_dir> -G "Visual Studio 16 2019" -A x64 -T v142 -DCMAKE_INSTALL_PREFIX=<install_dir> -Dfep3_participant_DIR=<fep3_participant_dir> -Ddev_essential_DIR=<dev_essential_dir>\lib\cmake\dev_essential -DBUILD_SHARED_LIBS="ON" -DBoost_INCLUDE_DIR=<boost_install_dir> -Dfep3_system_cmake_enable_documentation=False
    > cmake --build . --target install --config Release
    ```
    - <root_dir> The path where the *fep system* library is checked out and the main CMakeLists.txt is located.
    - <build_dir> The build directory
    - <install_dir> Path where the built artifacts will be installed.
    - <dev_essential_dir> The path were the [*dev_essential*](#howtodevessential) library was installed. File *dev_essential-config.cmake* is located under <dev_essential_dir>\lib\cmake\dev_essential. Important is that the path is given with forward slashes.
    - <fep3_participant_dir> The path were the [*fep3_participant*](#howtofep3participant) library was installed. File *fep3_participant-config.cmake* is located in this folder.
    - <boost_install_dir> The installation path of [boost](#howtoboost), *version.hpp* is located under <boost_install_dir>/boost
    >  **Note**: The above cmake calls are exemplary for using Windows and Visual Studio as generator. For gcc the addition of -DCMAKE_POSITION_INDEPENDENT_CODE=True is needed. Also depending on the generator used, the *--config* in the build step could be ignored and the adaptation of CMAKE_CONFIGURATION_TYPES or CMAKE_BUILD_TYPE could be necessary for building in other configurations.
### Additional Cmake options

- Enable tests
    - **Dfep3_system_cmake_enable_tests** and **Dfep3_system_cmake_enable_functional_tests** variables control the activation of the tests. These flags are set by default to False, both should be set to True for compiling the tests. For the compilation of the tests, [gtest](#howtogtest) is required.
    - Apart from the above flags, the **GTest_DIR** cmake variable should be set to the path where *GTestConfig.cmake* is located. Assuming the [gtest compilation](#howtogtest) was followed, this path is *<gtest_install_dir>\lib\cmake\GTest*.
    - Also the Cmake variable fep3_participant_cpp_DIR has to be set to the <fep3_participant_dir> path.
    - A call to cmake with these flags could look like:
         ```shell
        > cmake.exe -H<root_dir> -B<build_dir> -G "Visual Studio 16 2019" -A x64 -T v142 -DCMAKE_INSTALL_PREFIX=<install_dir> -Dfep3_participant_DIR=<fep3_participant_dir> -Ddev_essential_DIR=<dev_essential_dir>\lib\cmake\dev_essential -DBUILD_SHARED_LIBS="ON" -DBoost_INCLUDE_DIR=<boost_install_dir> -Dfep3_system_cmake_enable_documentation=False -Dfep3_system_cmake_enable_tests=True -Dfep3_system_cmake_enable_functional_tests=True -Dfep3_participant_cpp_DIR=<fep3_participant_dir> -DGTest_DIR=<gtest_install_dir>\lib\cmake\GTest
        > cmake --build . --target install --config Release
        ```
- Enable the documentation
    - The **fep3_system_cmake_enable_documentation** variable activates the build of the documentation. Default value is True. For this flag [doxygen](#howtodoxygen) and [sphinx](#howtosphinx) are required. The doxygen executable should be located in *${DOXYGEN_ROOT}/bin/doxygen.exe* and the cmake variable *DOXYGEN_ROOT* should be set accordingly.
    - A call to cmake with documentation activated could look like:
        ```shell
        > cmake.exe -H<root_dir> -B<build_dir> -G "Visual Studio 16 2019" -A x64 -T v142 -DCMAKE_INSTALL_PREFIX=<install_dir> -Dfep3_participant_DIR=<fep3_participant_dir> -Ddev_essential_DIR=<dev_essential_dir>\lib\cmake\dev_essential -DBUILD_SHARED_LIBS="ON" -DBoost_INCLUDE_DIR=<boost_install_dir> -Dfep3_system_cmake_enable_documentation=True -DDOXYGEN_ROOT=<doxyxen_dir>
        > cmake --build . --target install --config Release
        ```
### Build Environment ####

The libraries are build and tested only under following compilers and operating systems:
- Windows 10 x64 with Visual Studio C++ 2019 and v142 Toolset.
- Linux Ubuntu 18.04 LTS x64 with GCC 7.5 and libstdc++14 (C++14 ABI)
