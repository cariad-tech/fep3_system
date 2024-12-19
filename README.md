<!--
  Copyright 2023 CARIAD SE. 

This Source Code Form is subject to the terms of the Mozilla
Public License, v. 2.0. If a copy of the MPL was not distributed
with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
-->

# FEP 3 SDK - System Library

## Description

This installed package contains (or pulls in) the FEP SDK - System Library

The FEP System Library connects the FEP Service Bus to discover and configure and control a system.

## How to use

The FEP System Library provides a CMake >= 3.17 configuration.
Here's how to use it from your own CMake projects:

```cmake
find_package(fep3_system REQUIRED)
```

You can append the FEP System Library target to your existing targets to add a dependency:

```cmake
target_link_libraries(my_existing_target PRIVATE fep3_system)

# this helper macro will install ALL necessary library dependencies to the targets directory
fep3_system_install(my_existing_target destination_path)

# this helper macro will deploy as post build process ALL necessary library dependencies to the
# targets build directory
fep3_system_deploy(my_existing_target)
```

Note that the fep_system target will transitively pull in all required include directories and
libraries.

The FEP System Library provides gMock classes mocking the FEP System interfaces.
These classes are useful for unit tests written with the GoogleTest framework if mock classes shall
be used instead of real FEP System classes. In order to add the include path containing the mock
headers just link your test target against the _fep3_system_test_ target:

```cmake
target_link_libraries(my_test PRIVATE fep3_system_test)
```

## How to build using only CMake

### Prerequisites

- A python version >= 3.7.
- Download [CMake](https://cmake.org/) at least in version 3.17.0
- Using [git](https://git-scm.com/), clone the repository and checkout the desired branch
  (e.g. `main`).
- <a id="howtodevessential"></a> Build the _dev_essential_ libraries as described in the
  [README](https://devstack.vwgroup.com/bitbucket/projects/OPENDEV/repos/dev_essential/browse/README.md)
  file.
- <a id="howtofep3participant"></a> Build the _fep3_participant_ library as described in the
  [README](https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_participant/browse/README.md)
  file.
- <a id="howtoboost"></a> Boost version >= 1.73.0
  - can be compiled from [sources](https://boostorg.jfrog.io/artifactory/main/release/1.73.0/source/)
  - can be downloaded as prebuilt binaries for Windows [here](https://sourceforge.net/projects/boost/files/boost-binaries/1.73.0)
  - can be installed via package manager for Linux/Debian distributions
- [pybind11](https://pybind11.readthedocs.io/en/stable/index.html) can be installed following its
  [install guide](https://pybind11.readthedocs.io/en/stable/installing.html).
- Using git, clone the [clipp](https://github.com/muellan/clipp/) repository - a command line
  interface for modern C++. Afterwards, build and install it using CMake.

### Optional

- <a id="howtogtest"></a> [GTest](https://github.com/google/googletest) version 1.13 or newer.
  - GTest has to be compiled from source. After cloning the GTest repository, run the following
    commands inside the cloned folder (depending on your compiler or the configuration to be built,
    the CMake command should be adapted accordingly).

    ```sh
    $ mkdir build
    $ cd build
    $ cmake -G "Visual Studio 16 2019" -A x64 -T v142 \
        -DCMAKE_INSTALL_PREFIX=<gtest_install_dir> \
        -Dgtest_force_shared_crt=ON \
        ../
    $ cmake --build . --target install --config Release
    ```

  After executing the commands, `<gtest_install_dir>` will contain the header files and built
  libraries. The `gtest_force_shared_crt` flag is needed for Windows in order to compile with the
  correct Windows Runtime Library and to avoid linker errors when building the FEP System library.

- <a id="howtodoxygen"></a> [Doxygen](https://www.doxygen.nl/index.html) version 1.9.x.
  Doxygen executable should be located under `<doxygen_dir>/bin`.
- <a id="howtosphinx"></a> [Sphinx](https://pypi.org/project/Sphinx/) version 3.3.0.

### Build with CMake

Run the following command, (adaptations may be needed in case a different Visual Studio version is
used or a different configuration should be built).

```sh
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

- `<source_dir>`: Path to the FEP System library root source folder containing the `CMakeLists.txt`.
- `<build_dir>`: The build directory.
- `<install_dir>`: Path where the built artifacts will be installed.
- `<dev_essential_root>`: The path were the [_dev_essential_](#howtodevessential) library was
  installed. File `dev_essential-config.cmake` is located under
  `<dev_essential_root>/lib/cmake/dev_essential`.
- `<fep3_participant_root>`: The path were the [_fep3_participant_](#howtofep3participant) library
  was installed. File `fep3_participant-config.cmake` is located in this folder.
- `<boost_root>`: The installation path of boost.
- `<pybind11_root>`: The installation path of pybind11.
- `<python_version>`: The python version to use. This is optional and the entire flag can be
  omitted. If omitted, the first python version found is used.

> **_Note_**: The above CMake calls are exemplary for Windows and Visual Studio.
  For GCC the addition of `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` is needed. Also, depending on the
  generator used, the `--config` in the build step could be ignored and the adaptation of
  `CMAKE_CONFIGURATION_TYPES` or `CMAKE_BUILD_TYPE` could be necessary for building in other
  configurations.

Depending on your CMake version and the configuration of third-party packages like GTest, boost,
etc. it might be necessary to use `-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=ON` as well.
Depending on your boost installation, it might be necessary to set the following variables as well:

- `fep3_system_cmake_boost_diagnostic_definitions=ON|OFF`
- `fep3_system_cmake_boost_disable_autolinking=ON|OFF`
- `fep3_system_cmake_boost_dynamic_linking=ON|OFF`

### Additional CMake options

#### Enable tests

CMake variables

- `fep3_system_cmake_enable_tests=[ON|OFF]`
- `fep3_system_cmake_enable_functional_tests=[ON|OFF]`
- `fep3_system_cmake_enable_private_tests=[ON|OFF]`
- `fep3_system_export_test_targets=[ON|OFF]`

control the activation of the tests. These flags are set by default to `OFF`. They must be set
to `ON` to compile the tests. To compile the tests [GTest](#howtogtest) is required.
Apart from the above flags

- the **GTest_DIR** CMake variable should be set to the path where `GTestConfig.cmake` is located.
  Assuming the [gtest compilation](#howtogtest) was followed, this path is
  `<gtest_install_dir>/lib/cmake/GTest`.
- the CMake variable `fep3_participant_cpp_DIR` has to be set to the `<fep3_participant_root>` path.
- the CMake variable `fep3_participant_core_DIR` has to be set to the `<fep3_participant_root>` path.
- the CMake variable `CMAKE_PREFIX_PATH` has to be set to `<clipp_root>` path. The necessary
  `Findclipp.cmake` file is part of the dev_essential delivery.

A call to CMake with these flags could look like:

```sh
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
    -CMAKE_PREFIX_PATH=<clipp_root>
$ cmake --build . --target install --config Release
```

#### Enable the documentation

CMake variable

- `fep3_system_cmake_enable_documentation=[ON|OFF]`

activates the build of the documentation.
Default value is `ON`. For this flag [Doxygen](#howtodoxygen) and [sphinx](#howtosphinx) are
required. The Doxygen executable should be located in `<doxygen_root>/bin` and the CMake variable
`Doxygen_ROOT` should be set accordingly.

A call to CMake with documentation activated could look like:

```sh
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

The libraries are currently built and tested with following compilers and operating systems:

- Windows 10 x64 with Visual Studio C++ 2019 and v142 toolset.
- Linux Ubuntu 18.04 LTS x64 with GCC 7.5 and libstdc++14 (C++14 ABI)
