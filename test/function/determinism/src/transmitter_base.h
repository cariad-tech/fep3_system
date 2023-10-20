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
#include <fep3/fep3_errors.h>
#include <fep3/components/logging/logging_service_intf.h>
#include <fep3/components/logging/easy_logger.h>
#include "determinism_helper.hpp"
#include <thread>

using namespace fep3;

class TransmitJobBase : public cpp::DataJob
{
public:
    TransmitJobBase(const fep3::IComponents& components, const std::string& logger_name);
    virtual ~TransmitJobBase() = 0;

    virtual fep3::Result process(fep3::Timestamp time) override;
    virtual fep3::Result reset() override;

protected:
    cpp::DataWriter* _data_writer;
    uint32_t _current_data_value{0};
    uint32_t _sample_counter{ 1 };
    test::TestFileLogger _data_logger;

    fep3::cpp::PropertyVariable<bool> _verbose{ false };
    fep3::cpp::PropertyVariable<std::string> _log_file_path_property_variable{};
    fep3::cpp::PropertyVariable<uint32_t> _number_of_transmissions_property_variable{};
    fep3::cpp::PropertyVariable<uint64_t> _wait_real_time_ns_property_variable{};
    uint32_t _number_of_transmissions_done{ 0 };
    bool _finished{ false };
};
