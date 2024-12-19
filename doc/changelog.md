<!--
  Copyright 2023 CARIAD SE. 
  
This Source Code Form is subject to the terms of the Mozilla
Public License, v. 2.0. If a copy of the MPL was not distributed
with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
-->

# FEP SDK System Library Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0) and this project adheres to [Semantic Versioning](https://semver.org/lang/en).

<h3><a href="#fep_sdk_system_library">FEP SDK System Library</a></h3>

<!-- DOC GENERATOR HEADER -->
* [3.2.1](#FEP_SDK_3_2_1) \| [Changes](#FEP_SDK_3_2_1_changes) \| [Fixes](#FEP_SDK_3_2_1_fixes) \| [Known issues](#FEP_SDK_3_2_1_known_issues) \| Release date: 2023-11-06
* [3.2.0](#FEP_SDK_3_2_0) \| [Changes](#FEP_SDK_3_2_0_changes) \| [Fixes](#FEP_SDK_3_2_0_fixes) \| [Known issues](#FEP_SDK_3_2_0_known_issues) \| Release date: 2023-07-23
* [3.1.0](#FEP_SYSTEM_3_1_0) \| [Changes](#FEP_SYSTEM_3_1_0_changes) \| [Fixes](#FEP_SYSTEM_3_1_0_fixes) \| [Known issues](#FEP_SYSTEM_3_1_0_known_issues) \| Release date: 2022/04/13
* [3.0.1-beta](#FEP_SYSTEM_3_0_1) \| [Changes](#FEP_SYSTEM_3_0_1_changes) \| [Fixes](#FEP_SYSTEM_3_0_1_fixes) \| [Known issues](#FEP_SYSTEM_3_0_1_known_issues) \| Release date: 2022/02/02
* [3.0.0](#FEP_SYSTEM_3_0_0) \| [Changes](#FEP_SYSTEM_3_0_0_changes) \| [Fixes](#FEP_SYSTEM_3_0_0_fixes) \| [Known issues](#FEP_SYSTEM_3_0_0_known_issues) \| Release date: 2021/03/25

<!-- DOC GENERATOR BODY -->

<a name="FEP_SDK_3_2_1"></a>
 <h3><a href="No url">FEP SDK System 3.2.1</a> - Release date: 2023-11-06</h3>

<a name="FEP_SDK_3_2_1_changes"></a>
#### Changes
<a name="FEP_SDK_3_2_1_fixes"></a>
#### Fixes

_**Done**_

- [FEPSDK-3685][] - <a name="FEPSDK-3685_internal_link"></a> RPC logging is not working if a FEP System is discovered at second run [\[view\]][36539ec719e8ac74740014c3f32970ab62a53682] 
    * Perform RPC logging when participant is shut down abruptly
    
    
- [FEPSDK-3717][] - <a name="FEPSDK-3717_internal_link"></a> setParticipantState to unreachable fails on the second Participant [\[view\]][2b33dd924508b23136a47f9027ef6bd5f69f4f8e] 
    * when calling setParticipantState to UNREACHABLE, the correct Participant Proxy is removed from the system instance
    
    
- [FEPSDK-3718][] - <a name="FEPSDK-3718_internal_link"></a> setParticipantState does not forward logs to event monitors [\[view\]][99548c5ec746b2861da042b54551fa605e7d360a] 
    * setParticipantState forwards system logs to EventMonitor
     * state Undefined is never returned from getSystemState and getParticipantState
    
    

<a name="FEP_SDK_3_2_1_known_issues"></a>
<h4><a href="https://devstack.vwgroup.com/jira/issues/?jql=project%3D%22FEPSDK%22%20AND%20type%3D%22Bug%22%20AND%20level%3D%22public%22%20AND%20affectedVersion%20%3D%20%22FEP%20SDK%203.2.1%22">Known issues</a></h4>

<a name="FEP_SDK_3_2_1"></a>
<!--- Issue links -->
[FEPSDK-3685]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3685
[FEPSDK-3718]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3718
[FEPSDK-3717]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3717

<!--- Commit links -->
[36539ec719e8ac74740014c3f32970ab62a53682]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/36539ec719e8ac74740014c3f32970ab62a53682
[2b33dd924508b23136a47f9027ef6bd5f69f4f8e]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/2b33dd924508b23136a47f9027ef6bd5f69f4f8e
[99548c5ec746b2861da042b54551fa605e7d360a]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/99548c5ec746b2861da042b54551fa605e7d360a

<a name="FEP_SDK_3_2_0"></a>
 <h3><a href="https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/browse?at=refs%2Ftags%2Fv3.2.0">FEP SDK System 3.2.0</a> - 2023-07-23</h3>

<a name="FEP_SDK_3_2_0_changes"></a>
#### Changes

_**Done**_

- [FEPSDK-2856][] - <a name="FEPSDK-2856_internal_link"></a> start- and init-priority should support parallelism [\[view\]][0c4486818b319bab146aaed0373a219b56b5218b] [\[view\]][ad422f21ad9eac71fe431c90ec0d608c755d4b38] 
- [FEPSDK-3018][] - <a name="FEPSDK-3018_internal_link"></a> Move FEP SDK Participant API into non versioning namespaces [\[view\]][9a399658b0216757288c68f7dc605fc72f982b8e] 
    * Namespaces adapted as in description
     * Change advertised in DevTalk
     * Added portation guide in Readme
     * Did not adapt any coding guidelines in the confluence pages (fep3::base::arya::Configuration and fep3::base::arya::EasyLogging not changed)
     * Adapted documentation and tests/code in the 3 other repositoried
    
    
- [FEPSDK-3083][] - <a name="FEPSDK-3083_internal_link"></a> Copy C-API POC to fep_sdk_system library 
- [FEPSDK-3130][] - <a name="FEPSDK-3130_internal_link"></a> Create fep3::system::discoverSystem function which accepts participants and timeout [\[view\]][3ea6c2bdcc5fc468f74beaf4796c9747bcc3cbb4] [\[view\]][67202480d03b7e87dfb2bfeb8519da872d811844] 
    * HttpsystemAccess takes into account the timeout
     * added overloaded functions in fep3_system for discoverSystem(byUrl) discoverAllSystems(byUrl)
     * Adapted fep_control to use the new overloads, adapted the md file
     * fep3_sdk, documentation describes the new overloads. Snippets added in examples section
    
    
- [FEPSDK-3239][] - <a name="FEPSDK-3239_internal_link"></a> Provide plugin versions via RPC 
    * RPC Service ``component_registry.bronn.rpc.fep3.iid`` implemented
     * RPC signature for fep_control ``call_rpc demo_system demo_cpp_receiver component_registry component_registry.bronn.rpc.fep3.iid getPluginVersion '{&quot;service_iid&quot;: &quot;logging_service.arya.fep3.iid&quot;}``
    
    
- [FEPSDK-3257][] - <a name="FEPSDK-3257_internal_link"></a> Allow System State Transitions on a heterogeneous system [\[view\]][46395d4aee0e9ef8197295d1271af448de2d9136] 
    * FEP SDK System
        * Adapted setSystemLevel to allow transition of heterogeneous systems
        * Adapted tests to use test fixture
        * Added /doc/build to .gitignore
        * Removed redundant moves
    * FEP SDK Base Utilities
        * Added setSystemState tests
        * Fixed setParticipantState error message
        * Adapted tests to use test fixture
    
    
- [FEPSDK-3266][] - <a name="FEPSDK-3266_internal_link"></a> Provide convenience API for participant manipulation in FEP System library [\[view\]][b9395d13aa1f9da914b8343eeaa5d3c9c05fa799] 
    * added functions
     - setParticipantProperty(const std::string& participant_name, const std::string& property_path, const std::string& property_value)
     - getParticipantProperty(const std::string& participant_name, const std::string& property_path)
     - getParticipantState(const std::string& participant_name)
     * replaced function setPropertyValue() by new function setParticipantProperty() to avoid duplicated code
    
    
- [FEPSDK-3315][] - <a name="FEPSDK-3315_internal_link"></a> Deliver DDS Service Discovery as default plugin [\[view\]][ebe4a28143f9ec26e2234e2fc0856bd490fbe7e7] 
- [FEPSDK-3320][] - <a name="FEPSDK-3320_internal_link"></a> Health Service for FEP Participant and FEP System  
- [FEPSDK-3349][] - <a name="FEPSDK-3349_internal_link"></a> Provide setParticipantState and shutdown participant function in fep_system [\[view\]][d74302f3ca4f5b5c3817b2b2cc1ddb814c08dbfa] 
- [FEPSDK-3350][] - <a name="FEPSDK-3350_internal_link"></a> [Feature] Switch to Clipp command line parser [\[view\]][6b0d4282231795dba59adf07c716044d45abc23f] 
    *  Used [find cmake script from ddl | https://www.cip.audi.de/bitbucket/projects/DDLUTILITY/repos/ddl_utility/commits/b54df2eb3e351a0fede63a6938cd8094c23924f0#scripts/cmake/modules/Findclipp.cmake] and changed to use a cmake target
    * FindClipp.cmake is in the fep participant package
    * Participant command line parser supports clara and clipp  , and tests also test both
    * User defined arguments can be injected with clipp parameter and group instead of clara::parser
    * Deprecated warning when clara is used
    * fep system test uses clipp instead of clara
    * fep sdk examples and snippets used clipp instead of clara
    
    
- [FEPSDK-3421][] - <a name="FEPSDK-3421_internal_link"></a> Provide Python binding in system library [\[view\]][2828bc18f98b47740e7ed4a1413b863700979083] 
    * python bindings are added as a separate target
     * rudimentary test, added with
     - discoverSystem
     - loadSystem
     - initializeSystem
    
    
- [FEPSDK-3462][] - <a name="FEPSDK-3462_internal_link"></a> Same component loading mechanism in system and participant [\[view\]][76ca7053b271ee556e1bc4c9517a1e2f5c282470] 
    * Removed c plugins, implementation 
     * New interface in Service bus  to createServer by making the server non discoverable 
     * Redesigned System Library Serview Bus Wrapper
     * Used same compoment file mechanism and extension as in fep participant
    
    
- [FEPSDK-3477][] - <a name="FEPSDK-3477_internal_link"></a> Complete Python binding in system library [\[view\]][9c3bf4b9f52628e138079a8bbb02b322cb71c5d7] 
    * fep_control_api.py does support the agreed content
     * default-version for python is 3.7 at the moment (due installations on Jenkins)
    
    
- [FEPSDK-3532][] - <a name="FEPSDK-3532_internal_link"></a> Use dev_essential 1.3.0 integration [\[view\]][e4284da29c0706214d38ca75aa0ac35881c31909] 
    * Updates to dev_essential/1.3.0@fep/integration
     * Uses Findclipp.cmake from dev_essential installation
     * Removes Findclipp.cmake from fep_sdk_participant/3.2.0
     * Removed support fom clara as command line parser
     * Refactors deprecated a_util::result::isOk|isFailed occurrences using the new bool conversion operator
    
    
- [FEPSDK-3534][] - <a name="FEPSDK-3534_internal_link"></a> Deliver conan package with Python - C++ 3.8 3.9 3.10 bindings [\[view\]][7a35283c0274f097bab45b3e6f38225e38699919] 
    * Default option for python_versions set to 3.8,3.9,3.10 in conanfile
     * Supported Python versions in Python - C++ bindings are extended to Python 3.8, 3.9 & 3.10
    
    
- [FEPSDK-3602][] - <a name="FEPSDK-3602_internal_link"></a> Participant init and start priorities are persistent in system library instances [\[view\]][8c6ad932361508d19ad1c69c93eba06ea6169c36] [\[view\]][927a4222163d29777eb9e497c4b3fb948bd12c24] 
    *  properties initPriority  and startPriority created in service_bus FEP Component
     * API in fep system adjusted
     * For particiapants not having the update  the non persistent init priority is used. 
    
    
- [FEPSDK-3605][] - <a name="FEPSDK-3605_internal_link"></a> Improve error logging on state transitions [\[view\]][16346a185119dee9711ded6a73f13c6ac938d9a3] 
    * see attachment and open points in comments
     * Follow up tickets: FEPSDK-3637 FEPSDK-3636
    
    
- [FEPSDK-3606][] - <a name="FEPSDK-3606_internal_link"></a> Fep System library should throw exceptions or log errors on failed rpc client communication [\[view\]][b90a719a4cb31251a0124027a74190c755974f8c] [\[view\]][6d539b16b0b1648200450374cee934d1ddb7708b] 
    Errors logged in case of:
     * failed communication with participant state machine rpc service
     * failed communication with participant info rpc servcie
     * failed proxy access
     * failed logger service registration
    
    
- [FEPSDK-3614][] - <a name="FEPSDK-3614_internal_link"></a> FEP System Logger  configurable with severity and has file sink [\[view\]][3ae25b7a576f4251294f42510b19eea8496d8270] [\[view\]][d1966ecc11bb91ecfd8498e6675e72eab26e71b7] [\[view\]][81f4b3978177b62783532b8ae22186f01abcbd3c] [\[view\]][3707fff78021dc97107f5171cda27395bfdeba5e] 
- [FEPSDK-3618][] - <a name="FEPSDK-3618_internal_link"></a> Update MPL license header [\[view\]][c99e74e6cac7dfa7f8dbff1acf238e177ec90876] 
- [FEPSDK-3636][] - <a name="FEPSDK-3636_internal_link"></a> Refactor setSystemState() participant state transition error handling 
    * setSystemState throws an exception as soon as one participants' state transition failed
     * System documentation in fep sdk updated
    
    
- [FEPSDK-3637][] - <a name="FEPSDK-3637_internal_link"></a> FEP System state transition cancellation [\[view\]][34b01682aa54fdb9d48bb7552b03e7f5791a9931] 
    * System state transition stops on the first Participant's transition error
    * Execution policy  applies for all state transitions
    
    
    

_**Duplicate**_

- [FEPSDK-3608][] - <a name="FEPSDK-3608_internal_link"></a> FEP System provides error code and description of the Participant that failed a state Transition 
<a name="FEP_SDK_3_2_0_fixes"></a>
#### Fixes

_**Done**_

- [FEPSDK-2752][] - <a name="FEPSDK-2752_internal_link"></a> Multiple "discoverSystem" calls are needed when participants reside on different machines 
- [FEPSDK-3065][] - <a name="FEPSDK-3065_internal_link"></a> Error message unclear when triggering pause [\[view\]][c141f26a2cbe455590d90a1874b0a528fdd809ad] 
    * FEP3 system library's state change functions' error messages now contain the (failing) participant name as well
    
    
- [FEPSDK-3362][] - <a name="FEPSDK-3362_internal_link"></a> Logging is deregistered when fep3::System is copied or moved [\[view\]][f67b31b6529d80e40180910d368e7a36b5fc4d73] 
    * Fixed early deregistration of RPC Participant Logging
- [FEPSDK-3402][] - <a name="FEPSDK-3402_internal_link"></a> Call to setParticipantState with argument shutdown or unreachable crashes in fep system library [\[view\]][e29e31d455fd26037f4e965dedf474677f41889c] 
    * Fix implemented
     * wrote a test to catch the exception
    
    
- [FEPSDK-3495][] - <a name="FEPSDK-3495_internal_link"></a> FEP System Library Python Binding wont work for callRPC and getSystemState [\[view\]][e48abd35caa3d639bb457c2c9acbf4c63955e016] [\[view\]][9508c9097ecd0105a70789badfba0fbed70cf663] 
    * expanded class SystemState to enable reading _homogeneous
     * switched function 'call' to a lambda implementation to enable return value
    call returns a tuple now (boolean result of call and std::string response)
     * switched to pyton 3.8
    
    
- [FEPSDK-3511][] - <a name="FEPSDK-3511_internal_link"></a> No check for valid RPC Clients [\[view\]][4e6b3397b63957f957aaa949d134561f77b9853c] 
    * Avoid crash by checking return of part.getRPCComponentProxyByIID<IRPCConfiguration>()
    
    
- [FEPSDK-3533][] - <a name="FEPSDK-3533_internal_link"></a> FEP System Library hangs when logs occurs [\[view\]][a32c92cc26f82374c18a638aa0e1c92d95509cff] 
    * Release GIL when entering C++ code
     * Aquire GIL when entering Python code
     * Native started threads do not run Python code simulatanously
    
    
- [FEPSDK-3543][] - <a name="FEPSDK-3543_internal_link"></a> Fix setSystemState(Fep3State.UNREACHABLE) in FEP System Library [\[view\]][a80c0f56115859a3978bd1f905918fd6f9d32847] 
    * setSystemState(unreachable) should work as setParticipantState(unreachable)
         * set State to unloaded
         * call shutdownSystem
    
    
- [FEPSDK-3612][] - <a name="FEPSDK-3612_internal_link"></a> Python event_monitor imports wrong fep3_system library [\[view\]][1c3772b565f8c038247e080eb2e3a056c59e4f00] 
    * Use relative import for fep3_system first, so fep3_system and event_monitor are on the same path hierachy level. This is the site_packages delivery location
    * If this failed try to import fep3_system. This is used from conan package, e.g. folder [conan_package]/lib is in the python path

_**Not A Bug**_

- [FEPSDK-3424][] - <a name="FEPSDK-3424_internal_link"></a> [FEP 2] FEP System will throw if initialize() returned error code.  

_**Unresolved**_

- [FEPSDK-3675][] - <a name="FEPSDK-3675_internal_link"></a> cpython version range in conan recipe leads to "missing" packages [\[view\]][969e6f3663ff82f90966d25d83ee828897478738] [\[view\]][41ac96cece72ea6863adc0920514661f9a90131c] 

_**Won't Do**_

- [FEPSDK-3086][] - <a name="FEPSDK-3086_internal_link"></a> Shared libraries are located in bin/ instead of lib/ 

<a name="FEP_SDK_3_2_0_known_issues"></a>
<h4><a href="https://devstack.vwgroup.com/jira/issues/?jql=project%3D%22FEPSDK%22%20AND%20type%3D%22Bug%22%20AND%20level%3D%22public%22%20AND%20affectedVersion%20%3D%20%22FEP%20SDK%203.2.0%22">Known issues</a></h4>

<a name="FEP_SDK_3_2_0"></a>
<!--- Issue links -->
[FEPSDK-3257]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3257
[FEPSDK-3402]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3402
[FEPSDK-3477]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3477
[FEPSDK-3083]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3083
[FEPSDK-3130]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3130
[FEPSDK-3315]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3315
[FEPSDK-3534]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3534
[FEPSDK-3065]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3065
[FEPSDK-3495]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3495
[FEPSDK-3533]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3533
[FEPSDK-3614]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3614
[FEPSDK-3608]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3608
[FEPSDK-3637]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3637
[FEPSDK-3606]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3606
[FEPSDK-3543]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3543
[FEPSDK-3320]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3320
[FEPSDK-3605]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3605
[FEPSDK-3362]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3362
[FEPSDK-3018]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3018
[FEPSDK-2752]: https://devstack.vwgroup.com/jira/browse/FEPSDK-2752
[FEPSDK-3511]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3511
[FEPSDK-3602]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3602
[FEPSDK-3421]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3421
[FEPSDK-3266]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3266
[FEPSDK-3267]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3267
[FEPSDK-3239]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3239
[FEPSDK-3349]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3349
[FEPSDK-3612]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3612
[FEPSDK-3636]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3636
[FEPSDK-3185]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3185
[FEPSDK-3186]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3186
[FEPSDK-3462]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3462
[FEPSDK-3086]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3086
[FEPSDK-3618]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3618
[FEPSDK-3532]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3532
[FEPSDK-3424]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3424
[FEPSDK-3350]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3350
[FEPSDK-3675]: https://devstack.vwgroup.com/jira/browse/FEPSDK-3675
[FEPSDK-2856]: https://devstack.vwgroup.com/jira/browse/FEPSDK-2856

<!--- Commit links -->
[01b75d2f798621dbde7577bbb3805329547445ab]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/01b75d2f798621dbde7577bbb3805329547445ab
[c141f26a2cbe455590d90a1874b0a528fdd809ad]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/c141f26a2cbe455590d90a1874b0a528fdd809ad
[0c4486818b319bab146aaed0373a219b56b5218b]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/0c4486818b319bab146aaed0373a219b56b5218b
[ad422f21ad9eac71fe431c90ec0d608c755d4b38]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/ad422f21ad9eac71fe431c90ec0d608c755d4b38
[77e58cb9bda0e4bba1a2f6d6995b6a0ad9318989]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/77e58cb9bda0e4bba1a2f6d6995b6a0ad9318989
[153bc7cb6f784eba8e88777999a0c3eda166ce41]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/153bc7cb6f784eba8e88777999a0c3eda166ce41
[9a399658b0216757288c68f7dc605fc72f982b8e]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/9a399658b0216757288c68f7dc605fc72f982b8e
[433868b8a7f43c087136443076694fefee65ed6e]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/433868b8a7f43c087136443076694fefee65ed6e
[67202480d03b7e87dfb2bfeb8519da872d811844]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/67202480d03b7e87dfb2bfeb8519da872d811844
[ebe4a28143f9ec26e2234e2fc0856bd490fbe7e7]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/ebe4a28143f9ec26e2234e2fc0856bd490fbe7e7
[3ea6c2bdcc5fc468f74beaf4796c9747bcc3cbb4]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/3ea6c2bdcc5fc468f74beaf4796c9747bcc3cbb4
[46395d4aee0e9ef8197295d1271af448de2d9136]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/46395d4aee0e9ef8197295d1271af448de2d9136
[b9395d13aa1f9da914b8343eeaa5d3c9c05fa799]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/b9395d13aa1f9da914b8343eeaa5d3c9c05fa799
[6b0d4282231795dba59adf07c716044d45abc23f]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/6b0d4282231795dba59adf07c716044d45abc23f
[d74302f3ca4f5b5c3817b2b2cc1ddb814c08dbfa]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/d74302f3ca4f5b5c3817b2b2cc1ddb814c08dbfa
[2828bc18f98b47740e7ed4a1413b863700979083]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/2828bc18f98b47740e7ed4a1413b863700979083
[e29e31d455fd26037f4e965dedf474677f41889c]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/e29e31d455fd26037f4e965dedf474677f41889c
[9c3bf4b9f52628e138079a8bbb02b322cb71c5d7]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/9c3bf4b9f52628e138079a8bbb02b322cb71c5d7
[e48abd35caa3d639bb457c2c9acbf4c63955e016]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/e48abd35caa3d639bb457c2c9acbf4c63955e016
[9508c9097ecd0105a70789badfba0fbed70cf663]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/9508c9097ecd0105a70789badfba0fbed70cf663
[4e6b3397b63957f957aaa949d134561f77b9853c]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/4e6b3397b63957f957aaa949d134561f77b9853c
[a32c92cc26f82374c18a638aa0e1c92d95509cff]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/a32c92cc26f82374c18a638aa0e1c92d95509cff
[e4284da29c0706214d38ca75aa0ac35881c31909]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/e4284da29c0706214d38ca75aa0ac35881c31909
[76ca7053b271ee556e1bc4c9517a1e2f5c282470]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/76ca7053b271ee556e1bc4c9517a1e2f5c282470
[a80c0f56115859a3978bd1f905918fd6f9d32847]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/a80c0f56115859a3978bd1f905918fd6f9d32847
[7a35283c0274f097bab45b3e6f38225e38699919]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/7a35283c0274f097bab45b3e6f38225e38699919
[927a4222163d29777eb9e497c4b3fb948bd12c24]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/927a4222163d29777eb9e497c4b3fb948bd12c24
[8c6ad932361508d19ad1c69c93eba06ea6169c36]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/8c6ad932361508d19ad1c69c93eba06ea6169c36
[3707fff78021dc97107f5171cda27395bfdeba5e]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/3707fff78021dc97107f5171cda27395bfdeba5e
[81f4b3978177b62783532b8ae22186f01abcbd3c]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/81f4b3978177b62783532b8ae22186f01abcbd3c
[d1966ecc11bb91ecfd8498e6675e72eab26e71b7]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/d1966ecc11bb91ecfd8498e6675e72eab26e71b7
[1c3772b565f8c038247e080eb2e3a056c59e4f00]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/1c3772b565f8c038247e080eb2e3a056c59e4f00
[6d539b16b0b1648200450374cee934d1ddb7708b]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/6d539b16b0b1648200450374cee934d1ddb7708b
[b90a719a4cb31251a0124027a74190c755974f8c]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/b90a719a4cb31251a0124027a74190c755974f8c
[16346a185119dee9711ded6a73f13c6ac938d9a3]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/16346a185119dee9711ded6a73f13c6ac938d9a3
[3ae25b7a576f4251294f42510b19eea8496d8270]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/3ae25b7a576f4251294f42510b19eea8496d8270
[f67b31b6529d80e40180910d368e7a36b5fc4d73]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/f67b31b6529d80e40180910d368e7a36b5fc4d73
[c99e74e6cac7dfa7f8dbff1acf238e177ec90876]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/c99e74e6cac7dfa7f8dbff1acf238e177ec90876
[34b01682aa54fdb9d48bb7552b03e7f5791a9931]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/34b01682aa54fdb9d48bb7552b03e7f5791a9931
[41ac96cece72ea6863adc0920514661f9a90131c]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/41ac96cece72ea6863adc0920514661f9a90131c
[969e6f3663ff82f90966d25d83ee828897478738]: https://devstack.vwgroup.com/bitbucket/projects/FEPSDK/repos/fep3_system/commits/969e6f3663ff82f90966d25d83ee828897478738

<a name="FEP_SYSTEM_3_1_0"></a>
## [3.1.0]

<a name="FEP_SYSTEM_3_1_0_changes"></a>

### Changes
- FEPSDK-3185 remove gcc5 profiles
- FEPSDK-3137 Improve fep sdk timing documentation
- FEPSDK-3103 Add Windows_x64_vc142_VS2019 build profile
- FEPSDK-3101 Add Linux_armv8_gcc7 build profile
- FEPSDK-2856 Parallel initialization and tests

<a name="FEP_SYSTEM_3_0_1"></a>
## [3.0.1-beta]

<a name="FEP_SYSTEM_3_0_1_changes"></a>

### Changes
- Based on FEP SDK Participant 3.0.1
- Update to dev_essential 1.1.0
- FEPSDK-3255 Add asynchronous addition of participants to system
- FEPSDK-3188 Disable Linux_armv8_gcc5
- FEPSDK-3137 Improve fep sdk timing documentation
- FEPSDK-3103 Add Windows_x64_vc142_VS2019 build profile
- FEPSDK-3101 Add Linux_armv8_gcc7 build profile
- FEPSDK-2961 Copy pdb to package on MSVC Build

<a name="FEP_SYSTEM_3_0_1_fixes"></a>

### Bugfixes
- FEPSDK-3071 Adapted ParticpiantProxy to store its logger as shared_ptr instead of reference.
- FEPSDK-3065 enhance error message when participant denies state change (to make in unclear for example when triggering pause)
- FEPSDK-3027 Extended get/setProperties error log and exception by description
- FEPSDK-2982 Monitoring FEP System may fail silently due to early event monitor registration

<a name="FEP_SYSTEM_3_0_0"></a>
## [3.0.0]

<a name="FEP_SYSTEM_3_0_0_changes"></a>

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
