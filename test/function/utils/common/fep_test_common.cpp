/**
 * @file
 * @copyright
 * @verbatim
Copyright @ 2021 VW Group. All rights reserved.

    This Source Code Form is subject to the terms of the Mozilla
    Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

If it is not possible or desirable to put the notice in a particular file, then
You may include the notice in a location (such as a LICENSE file in a
relevant directory) where a recipient would be likely to look for such a notice.

You may add additional accurate notices of copyright ownership.

@endverbatim
 */


#ifndef _FEP_TEST_COMMON_H_INC_
#define _FEP_TEST_COMMON_H_INC_

#include <cmath>
#include <assert.h>
#include <sstream>
#include <thread>

#include <a_util/process.h>
#include <a_util/strings.h>

#include "fep_test_common.h"

#define FEP_PREP_CMD_VERSION 1.0
#define FEP_PREP_REMOTE_PROP_TIMEOUT static_cast<timestamp_t>(5e6)
#define XSTR(x) STR(x)
#define STR(x) #x
#define ABC 123

#ifdef WIN32
    #pragma warning( push )
    #pragma warning( disable : 4250 )
#endif

/**
 * Create a platform (tester)-dependant name for stand-alone use.
 * @param [in] strOrigName  The original Module name
 * @return The modified name.
 */
const std::string makePlatformDepName(const char* strOrigName)
{
    std::string strModuleNameDep(strOrigName);

    #if (_MSC_VER < 1920)
        strModuleNameDep.append("_win64_vc141");
    #elif (_MSC_VER >= 1920)
        strModuleNameDep.append("_win64_vc142");
    #elif (defined (__linux))
        strModuleNameDep.append("_linux");
    #else
        // this goes for vc90, vc120 or apple or arm or whatever.
        #error "Platform currently not supported";
    #endif // Version check

    std::stringstream ss;
    ss << std::this_thread::get_id();

    strModuleNameDep += "_" + a_util::strings::toString(a_util::process::getCurrentProcessId());
    strModuleNameDep += "_" + ss.str();
    return strModuleNameDep;
}

#ifdef WIN32
    #pragma warning( pop )
#endif

#endif // _FEP_TEST_COMMON_H_INC_
