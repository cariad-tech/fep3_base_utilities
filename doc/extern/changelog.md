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
# FEP SDK Base Utilities Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0) and this project adheres to [Semantic Versioning](https://semver.org/lang/en).

## [Unreleased]
## [3.1.0]

### Changes
- FEPSDK-3311 Based on dev_essential/1.1.1
- FEPSDK-3302 Based on fep_sdk_system/3.1.0
- FEPSDK-3278 switch to c++ 17 standard
- FEPSDK-3185 Remove gcc5 profile
- FEPSDK-3103 Add VS 2019 v142 profile
- FEPSDK-3101 Add armv8 gcc7 profiles
- FEPSDK-2961 Provide all RPC commands in fep_control

### Bugfixes
- FEPSDK-3222 FEP Control returns multiple answers for discoverAllSystems and getParticipantState
- FEPSDK-3251 Inconsistent FEP Control WebSocket Interface
- FEPSDK-3027 Useless error message "Error: (unknown)" when trying to set a non existing property via fep_control
- FEPSDK-3034 handle spaces and special characters correctly in completion and interpreting the command line
## [3.0.0]

### Changes
- FEPUTILITY-854 FEP Control informs clients about shutdown
- FEPSDK-2916 Fix properties to use nanoseconds unit
- FEPSDK-2857 Set pause mode as unsupported
- FEPSDK-2768 Rename property cycle_time_ms of clock service 
- FEPSDK-2747 Add Remote Endpoint to FEP SDK Base Utilities
- FEPSDK-2723 Port to FEP3: Json as return value for fep_control
- FEPSDK-2721 FEP Control tool command call does not return after the execution
- FEPSDK-2561 Use the new profiles [gcc5, v141] as base for the delivery packages
- FEPSDK-2493 shutdown test freeze in FEP control tool
- FEPSDK-2189 Create a base FEP Control Tool
