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


#pragma once

#include <gmock/gmock.h>

#include <fep_system/logging_sink_csv.h>

namespace fep3
{
namespace mock
{

struct LogFileWrapper : public ILogFileWrapper
{
    MOCK_METHOD(void, open, (const std::string&, std::ios_base::openmode), (override));
    MOCK_METHOD(bool, is_open, (), (const, override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, fail, (), (const, override));
    MOCK_METHOD(void, flush, (), (override));
    MOCK_METHOD(void, write, (const std::string&), (override));

};

}
}
