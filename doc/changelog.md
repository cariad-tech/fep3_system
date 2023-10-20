<!--
  Copyright @ 2021 VW Group. All rights reserved.
  
      This Source Code Form is subject to the terms of the Mozilla
      Public License, v. 2.0. If a copy of the MPL was not distributed
      with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
  
  -->

# FEP SDK System Library Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0) and this project adheres to [Semantic Versioning](https://semver.org/lang/en).

## [Unreleased]

## [3.2.0]

For the changes and bugfixes please have a look to the FEP SDK changelog.


## [3.1.0]

### Changes
- Based on FEP SDK Participant 3.0.1
- Update to dev_essential 1.1.0
- FEPSDK-3255 Add asynchronous addition of participants to system
- FEPSDK-3188 Disable Linux_armv8_gcc5
- FEPSDK-3137 Improve fep sdk timing documentation
- FEPSDK-3103 Add Windows_x64_vc142_VS2019 build profile
- FEPSDK-3101 Add Linux_armv8_gcc7 build profile
- FEPSDK-2961 Copy pdb to package on MSVC Build

### Bugfixes
- FEPSDK-3071 Adapted ParticpiantProxy to store its logger as shared_ptr instead of reference.
- FEPSDK-3065 enhance error message when participant denies state change (to make in unclear for example when triggering pause)
- FEPSDK-3027 Extended get/setProperties error log and exception by description
- FEPSDK-3027 FEPSDK-3190 Added descriptions to results being logged
- FEPSDK-2982 Monitoring FEP System may fail silently due to early event monitor registration

## [3.0.0] 2021-03-05

### Changes
- FEPSDK-2857 Set pause mode as unsupported
- FEPSDK-2757 Improve test for deterministic simulation
- FEPSDK-2664 Apply user documentation guidelines
- FEPSDK-2585 Prepare FEP3 SDK code to be distributed as OSS
- FEPSDK-2561 [FEP3] Use the new profiles [gcc5, v141] as base for the delivery packages
- FEPSDK-2491 Correct inconsistency in file names covering "stream types"
- FEPSDK-2363 Rework all enumerations to the guideline and unify them
- FEPSDK-2321 Integrate RPC Logging Service to FEP System API 3
- FEPSDK-2320 Integrate RPC Configuration Service to FEP System API 3
- FEPSDK-2311 discover*System* functions' documentation are the same in header file
- FEPSDK-2301 Create initial documentation
- FEPSDK-2133 Check and Change FEP_ defines to FEP3_ defines
- FEPSDK-2056 getCurrentSystemTiming() from fep::System in FEP System Library 3
- FEPSDK-2055 Adapt system api to use FEP 3
- FEPSDK-1340 [POC] Design a Health Service for FEP Participant Library