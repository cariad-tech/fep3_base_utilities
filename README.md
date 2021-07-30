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
# FEP Base Utilities

# Description

## FEP Base Utilities functionality in a nutshell

### FEP Control Tool


# Dependencies

* self-contained package
* but at build time it depends on FEP System Library and at test time it depends on FEP Participant Library


# How to build using only cmake ###
### Prerequisites
- Download [CMake](https://cmake.org/) at least in version 3.17.0
- Using [git](https://git-scm.com/), clone the repository and checkout the desired branch (e.g. `master`)
- <a id="howtodevessential"></a> Build the dev_essential library as described in the [Readme](https://www.github.com/dev-essential) file.
- <a id="howtofep3system"></a> Build the fep3_system library as described in the [Readme](https://www.github.com/fep3_system) file.
- Boost in version 1.73, can be compiled from [sources](https://boostorg.jfrog.io/artifactory/main/release/1.73.0/source/) or the built binaries can be directly downloaded. For Windows [Boost 1.73](https://sourceforge.net/projects/boost/files/boost-binaries/1.73.0/boost_1_73_0-msvc-14.1-64.exe/download) can be downloaded, for Linux/Debian distributions apt can be used.

### Optional
- <a id="howtofep3participant"></a>  Build the fep3_participant library as described in the [Readme](https://www.github.com/fep3_participant) file.
-   <a id="howtogtest"></a> [Gtest](https://github.com/google/googletest) version 1.10 or newer.
    - Gtest has to be compiled from sources. After checking out the gtest github repository, run the following commands inside the checked out folder (depending on your compiler or the configuration to be built, the cmake command should be adapted accordingly). After executing the commands, &lt;gtest_install_dir&gt; will contain the built libraries. *gtest_force_shared_crt* flag is needed for windows in order compile with the correct Windows Runtime Library and avoid linking errors later when building the *fep participant* library.

     - `mkdir build`
     - `cd build`
     - `cmake -G "Visual Studio 16 2019" -A x64 -T v142
     -DCMAKE_INSTALL_PREFIX=<gtest_install_dir> -Dgtest_force_shared_crt=ON  ../`
     - `cmake --build . --target install --config Release`

### Build with cmake
- Run the following command, (adaptations may be needed in case a different Visual Studio version is used or different configuration should be built).

   - `cmake.exe -H<root_dir> -B<build_dir> -G "Visual Studio 16 2019" -A x64 -T v142 -DCMAKE_INSTALL_PREFIX=package -Dfep_base_utilities_cmake_enable_tests=False -Dfep3_system_DIR=<fep_system_dir> -Ddev_essential_DIR=<dev_essential_dir>/lib/cmake/dev_essential -DBoost_INCLUDE_DIR=<boost_install_dir>`
    - `cmake --build . --target install --config Release`

    - &lt;root_dir&gt; The path where the  *fep base utilities* library is checked out and the main CMakeLists.txt is located.
    - &lt;build_dir&gt; The build directory
    - &lt;install_dir&gt; Path where the built artifacts will be installed.
    - &lt;dev_essential_dir&gt; The path were the [*dev_essential*](#howtodevessential) library was installed. File *dev_essential-config.cmake* is located under &lt;dev_essential_dir&gt;/lib/cmake/dev_essential.
    - &lt;fep_system_dir&gt; The path were the [*fep3_system*](#howtofep3system) library was installed. File *fep3_system-config.cmake* is located in this folder.
    - &lt;boost_install_dir&gt; The installation path of boost, *version.hpp* is located under &lt;boost_install_dir&gt;/boost.
    >  **Note**: The above cmake calls are exemplary for using Windows and Visual Studio as generator. For gcc the addition of -DCMAKE_POSITION_INDEPENDENT_CODE=True is needed. Also depending on the generator used, the *--config* in the build step could be ignored and the adaptation of CMAKE_CONFIGURATION_TYPES or CMAKE_BUILD_TYPE could be necessary for building in other configurations.
### Additional Cmake options

- Enable tests
    - **fep_base_utilities_cmake_enable_tests** variable controls the activation of the tests. The variable is set by default to True. For activating this flag, [gtest](#howtogtest) is required.
    - Apart from **fep_base_utilities_cmake_enable_tests**, the **GTest_DIR** cmake variable should be set to the path where *GTestConfig.cmake* is located. Assuming the [gtest](#howtogtest) was followed, this path is *&lt;gtest_install_dir&gt;/lib/cmake/GTest*.
    - Additionally [*fep3_participant library*](#howtofep3participant) should be compiled and the following variables should be set:
        - **fep3_participant_cpp_DIR** and **Dfep3_participant_core_DIR** to the installation directory of the library.
    - A call to cmake with these flags could look like:

        - `cmake.exe -H<root_dir> -B<build_dir> -G "Visual Studio 16 2019" -A x64 -T v142 -DCMAKE_INSTALL_PREFIX=package -Dfep_base_utilities_cmake_enable_tests=True -Dfep3_system_DIR=<fep_system_dir> -Ddev_essential_DIR=<dev_essential_dir>/lib/cmake/dev_essential -DBoost_INCLUDE_DIR=<boost_install_dir> -Dfep3_participant_core_DIR=<fep3_participant_dir> -Dfep3_participant_cpp_DIR= <fep3_participant_dir> -DGTest_DIR=<gtest_install_dir>\lib\cmake\GTest`
        - `cmake --build . --target install --config Release`
 
        &lt;fep3_participant_dir&gt; is the installation directory of the *fep3_participant library* where the files
        *fep3_participant_cpp-config.cmake* and *fep3_participant_core-config.cmake* are located.
### Tested compilers
- Windows 10 x64 with Visual Studio C++ 2019 and v142 Toolset.
- Linux Ubuntu 18.04 LTS x64 with GCC 7.5 and libstdc++14 (C++14 ABI)
