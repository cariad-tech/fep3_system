/**
 * @file
 * @copyright
 * @verbatim
Copyright 2023 CARIAD SE. 

This Source Code Form is subject to the terms of the Mozilla
Public License, v. 2.0. If a copy of the MPL was not distributed
with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
@endverbatim
 */


#ifndef _FEP3_SYSTEM_EXPORT_H_INCLUDED_
#define _FEP3_SYSTEM_EXPORT_H_INCLUDED_

#ifdef WIN32
    #ifdef FEP3_SYSTEM_LIB_DO_EXPORT
        /// Macro switching between export / import of the fep sdk shared object
        #define FEP3_SYSTEM_EXPORT __declspec( dllexport )
    #else   // FEP_SYSTEM_LIB_DO_EXPORT
        /// Macro switching between export / import of the fep sdk shared object
        #define FEP3_SYSTEM_EXPORT __declspec( dllimport )
    #endif
#else   // WIN32
    #ifdef FEP3_SYSTEM_LIB_DO_EXPORT
        /// Macro switching between export / import of the fep sdk shared object
        #define FEP3_SYSTEM_EXPORT __attribute__ ((visibility("default")))
    #else   // FEP_SYSTEM_LIB_DO_EXPORT
        /// Macro switching between export / import of the fep sdk shared object
        #define FEP3_SYSTEM_EXPORT
    #endif
#endif
#endif // _FEP3_SYSTEM_EXPORT_H_INCLUDED_
