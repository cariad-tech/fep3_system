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

## Description

This installed package contains (or pulls in) the FEP SDK - System Library

The FEP System Library connects the FEP Service Bus to discover and configure and control a system.

## How to use

The FEP System Library provides a CMake >= 3.17 configuration. Here's how to use it from your own CMake projects:

    find_package(fep3_system REQUIRED)

You can append the FEP System Libray Target to your existing targets to add a dependency:

    target_link_libraries(my_existing_target PRIVATE fep3_system)
    #this helper macro will install ALL necessary library dependency to the targets directory
    fep3_system_install(my_existing_target destination_path)

    #this helper macro will deploy as post build process ALL necessary library dependency to the targets build directory
    fep3_system_deploy(my_existing_target)

Note that the fep_system target will transitively pull in all required include directories and libraries.

The FEP System Library provides googlemock classes mocking the FEP System interfaces. These classes are useful for unit tests written with the googletest
framework if mock classes shall be used instead of real FEP System classes. In order to add the include path containing the mock headers
just link your test target against the *fep3_system_test* target:
    target_link_libraries(my_test PRIVATE fep3_system_test)
## How to build using only cmake
### Prerequisites
- A python version >= 3.7.
- Download [CMake](https://cmake.org/) at least in version 3.17.0
- Using [git](https://git-scm.com/), clone the repository and checkout the desired branch (e.g. `master`)
- <a id="howtodevessential"></a> Build the dev_essential library as described in the [Readme](https://www.cip.audi.de/bitbucket/projects/OPENDEV/repos/odautil/browse/README.md) file.
- <a id="howtofep3participant"></a> Build the fep3_participant library as described in the [Readme](https://www.cip.audi.de/bitbucket/projects/FEPSDK/repos/fep3_participant/browse/README.md) file.
- <a id="howtoboost"></a> Boost in version 1.73, can be compiled from [sources](https://boostorg.jfrog.io/artifactory/main/release/1.73.0/source/) or the built binaries can be directly downloaded. For Windows [Boost 1.73](https://sourceforge.net/projects/boost/files/boost-binaries/1.73.0) can be downloaded, for Linux/Debian distributions apt can be used.
- [pybind11](https://pybind11.readthedocs.io/en/stable/index.html) can be installed following its [install guide](https://pybind11.readthedocs.io/en/stable/installing.html).
- Using git, clone the [clipp](https://github.com/muellan/clipp/) repository, a command line interface for modern C++.

### Optional
-   <a id="howtogtest"></a> [Gtest](https://github.com/google/googletest) version 1.10 or newer.
    - Gtest has to be compiled from sources. After checking out the gtest github repository, run the following commands inside the checked out folder (depending on your compiler or the configuration to be built, the cmake command should be adapted accordingly). After executing the commands, <gtest_install_dir> will contain the built libraries. *gtest_force_shared_crt* flag is needed for windows in order compile with the correct Windows Runtime Library and avoid linking errors later when building the *fep system* library.

    ```shell
$ mkdir build
$ cd build
$ cmake -G "Visual Studio 16 2019" -A x64 -T v142 \
    -DCMAKE_INSTALL_PREFIX=<gtest_install_dir> \
    -Dgtest_force_shared_crt=ON \
    ../
$ cmake --build . --target install --config Release
    ```
- <a id="howtodoxygen"></a> [Doxygen](https://www.doxygen.nl/index.html) version 1.9.x. Doxygen executable should be located under <doxygen_dir>/bin
- <a id="howtosphinx"></a> [Sphinx](https://pypi.org/project/Sphinx/) version 3.3.0.
### Build with cmake
Run the following command, (adaptations may be needed in case a different Visual Studio version is used or different configuration should be built).
    ```shell
$ cmake -H<source_dir> -B<build_dir> -G "Visual Studio 16 2019" -A x64 -T v142 \
    -DCMAKE_INSTALL_PREFIX=<install_dir> \
    -DBUILD_SHARED_LIBS=ON \
    -Dfep3_participant_DIR=<fep3_participant_root> \
    -Dfep3_system_cmake_enable_documentation=OFF \
    -Dfep3_system_cmake_python_version=<python_version> \
    -Ddev_essential_DIR=<dev_essential_root>/lib/cmake/dev_essential \
    -DBoost_DIR=<boost_root>/lib/cmake/Boost-<boost_version>/ \
    -Dpybind11_ROOT=<pybind11_root>/
$ cmake --build . --target install --config Release
    ```
- \<source_dir\> The path where the *fep system* library is checked out and the main CMakeLists.txt is located.
- \<build_dir\>: The build directory
- \<install_dir\>: Path where the built artifacts will be installed.
- \<dev_essential_root\>: The path were the [*dev_essential*](#howtodevessential) library was installed. File *dev_essential-config.cmake* is located under \<dev_essential_root\>/lib/cmake/dev_essential.
- \<fep3_participant_root\>: The path were the [*fep3_participant*](#howtofep3participant) library was installed. File *fep3_participant-config.cmake* is located in this folder.
- \<boost_root\>: The installation path of boost. File *BoostConfig.cmake* is located under \<boost_root\>/lib/cmake/Boost-\<boost_version\> (e.g. Boost-1.73.0).
- \<pybind11_root\>: The installation path of pybind11. File *pybind11Config.cmake* is located under \<pybind11_root\>/lib/cmake/pybind11.
- \<python_version\>: The python version to use. This is optional and the entire flag can be omitted. If omitted, the first python version found is used.
>  **Note**: The above cmake calls are exemplary for using Windows and Visual Studio as generator. For gcc the addition of -DCMAKE_POSITION_INDEPENDENT_CODE=ON is needed. Also depending on the generator used, the *--config* in the build step could be ignored and the adaptation of CMAKE_CONFIGURATION_TYPES or CMAKE_BUILD_TYPE could be necessary for building in other configurations.
Depending on your CMake version and the configuration of third-party packages like Gtest, boost, etc. it might be necessary to use *-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON* as well.
Depending on your boost installation, it might be nessecary to set the following variables as well:
- *fep3_system_cmake_boost_diagnostic_definitions=ON|OFF*
- *fep3_system_cmake_boost_disable_autolinking=ON|OFF*
- *fep3_system_cmake_boost_dynamic_linking=ON|OFF*
### Additional Cmake options

#### Enable tests
CMake variables
- *fep3_system_cmake_enable_tests=[ON|OFF]* and
- *fep3_system_cmake_enable_functional_tests=[ON|OFF]*
- *fep3_system_cmake_enable_private_tests=[ON|OFF]* and
- *fep3_system_export_test_targets=[ON|OFF]*
control the activation of the tests. These flags are set by default to *OFF*, both should be set to *ON* for compiling the tests. For the compilation of the tests, [gtest](#howtogtest) is required.
- Apart from the above flags, the **GTest_DIR** cmake variable should be set to the path where *GTestConfig.cmake* is located. Assuming the [gtest compilation](#howtogtest) was followed, this path is *\<gtest_install_dir\>/lib/cmake/GTest*.
- Also the CMake variable *fep3_participant_cpp_DIR* has to be set to the \<fep3_participant_root\> path.
- Also the CMake variable *fep3_participant_core_DIR* has to be set to the \<fep3_participant_root\> path.
- Also the CMake variable *clipp_INCLUDE_DIR* has to be set to \<clipp_root\>/include path. The necessary *Findclipp.cmake* file is part of the fep3_participant delivery and the search path can be addded via *CMAKE_MODULE_PATH=<fep3_participant_root>/cmake/modules*.
A call to cmake with these flags could look like:
         ```shell
$ cmake -H<source_dir> -B<build_dir> -G "Visual Studio 16 2019" -A x64 -T v142 \
    -DCMAKE_INSTALL_PREFIX=<install_dir> \
    -DBUILD_SHARED_LIBS=ON \
    -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON \
    -DCMAKE_MODULE_PATH=<fep3_participant_root>/cmake/modules \
    -Dfep3_participant_DIR=<fep3_participant_root> \
    -Dfep3_system_cmake_enable_documentation=OFF \
    -Dfep3_system_cmake_enable_tests=ON \
    -Dfep3_system_cmake_enable_functional_tests=ON \
    -Dfep3_system_cmake_enable_private_tests=ON \
    -Dfep3_system_export_test_targets=ON \
    -Dfep3_participant_core_DIR=<fep3_participant_root> \
    -Dfep3_participant_cpp_DIR=<fep3_participant_root> \
    -Ddev_essential_DIR=<dev_essential_root>/lib/cmake/dev_essential \
    -DBoost_DIR=<boost_root>/lib/cmake/Boost-<boost_version>/ \
    -Dpybind11_ROOT=<pybind11_root>/ \
    -DGTest_DIR=<gtest_install_dir>/lib/cmake/GTest \
    -Dclipp_INCLUDE_DIR=<clipp_root>/include
$ cmake --build . --target install --config Release
        ```
#### Enable the documentation
CMake variable
- *fep3_system_cmake_enable_documentation=[ON|OFF]* 
activates the build of the documentation. Default value is *ON*. For this flag [doxygen](#howtodoxygen) and [sphinx](#howtosphinx) are required. The doxygen executable should be located in *\<doxygen_root\>/bin/doxygen.exe* and the cmake variable *Doxygen_ROOT* should be set accordingly.
A call to cmake with documentation activated could look like:
        ```shell
$ cmake -H<source_dir> -B<build_dir> -G "Visual Studio 16 2019" -A x64 -T v142 \
    -DCMAKE_INSTALL_PREFIX=<install_dir> \
    -DBUILD_SHARED_LIBS=ON \
    -Dfep3_participant_DIR=<fep3_participant_root> \
    -Dfep3_system_cmake_enable_documentation=ON \
    -Ddev_essential_DIR=<dev_essential_root>/lib/cmake/dev_essential \
    -DBoost_DIR=<boost_install_dir> \
    -Dpybind11_ROOT=<pybind11_root>/ \
    -DDoxygen_ROOT=<doxyxen_root>
$ cmake --build . --target install --config Release
        ```
### Build Environment

The libraries are build and tested only under following compilers and operating systems:
- Windows 10 x64 with Visual Studio C++ 2019 and v142 Toolset.
- Linux Ubuntu 18.04 LTS x64 with GCC 7.5 and libstdc++14 (C++14 ABI)
