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


#include <fep3/cpp/datajob.h>
#include "determinism_helper.hpp"

class TransmitJobBase : public fep3::cpp::DataJob
{
public:
    TransmitJobBase(fep3::arya::JobConfiguration job_config);
    virtual ~TransmitJobBase() = 0;

    virtual fep3::Result process(fep3::Timestamp time) override;
    virtual fep3::Result reset() override;

protected:
    fep3::cpp::DataWriter* _data_writer;
    std::vector<int32_t> _values;
    uint8_t _selector = 0;
    uint8_t _counter = 0;
    fep3::test::TestFileLogger _logger;
    std::chrono::milliseconds _start_time_ms;

    fep3::cpp::PropertyVariable<int32_t> _delay{ 0 };
};