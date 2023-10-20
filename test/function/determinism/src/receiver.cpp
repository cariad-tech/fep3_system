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
#include <fep3/cpp/element_base.h>
#include <fep3/cpp/participant.h>
#include <fep3/components/logging/easy_logger.h>
#include <fep3/base/sample/raw_memory.h>
#include "determinism_helper.hpp"

#include <iostream>

using namespace fep3;
using namespace cpp;

class ReceiveJob : public DataJob
{
public:
    ReceiveJob(const fep3::IComponents& components) :
        DataJob("receive", std::chrono::seconds(1))
    {
        if(!initLogger(components, "receive_job_logger"))
        {
            throw std::runtime_error("failed to inititalize logger");
        }
        _reader_data_one = addDataIn("data_one", fep3::base::StreamTypePlain<int32_t>());
        _reader_data_two = addDataIn("data_two", fep3::base::StreamTypePlain<int32_t>());
        registerPropertyVariable(_verbose, "verbose");
        registerPropertyVariable(_log_file_path_data_one_property_variable, "log_file_path_data_one");
        registerPropertyVariable(_log_file_path_data_two_property_variable, "log_file_path_data_two");
        registerPropertyVariable(_number_of_receptions_property_variable, "number_of_receptions");
    }

    fep3::Result process(fep3::Timestamp) override
    {
        if(_number_of_receptions_done < _number_of_receptions_property_variable)
        {
            while(const auto sample = _reader_data_one->popSampleOldest())
            {
                const auto counter = sample->getCounter();
                const auto time_stamp = sample->getTime();
                uint32_t value;
                fep3::base::RawMemoryStandardType<uint32_t> memory(value);
                sample->read(memory);

                if(_verbose)
                {
                    FEP3_LOG_INFO("Popped sample from \"data_one\": counter = " + std::to_string(counter) + " time stamp = " + std::to_string(time_stamp.count()) + "ns");
                }

                _data_one_logger.logToFile
                    (std::to_string(counter)
                    + " @ " + std::to_string(time_stamp.count()) + "ns: "
                    + std::to_string(value)
                    );
            }
            while(const auto sample = _reader_data_two->popSampleOldest())
            {
                const auto counter = sample->getCounter();
                const auto time_stamp = sample->getTime();
                uint32_t value;
                fep3::base::RawMemoryStandardType<uint32_t> memory(value);
                sample->read(memory);

                if(_verbose)
                {
                    FEP3_LOG_INFO("Popped sample from \"data_two\": counter = " + std::to_string(counter) + " time stamp = " + std::to_string(time_stamp.count()) + "ns");
                }

                _data_two_logger.logToFile
                    (std::to_string(counter)
                    + " @ " + std::to_string(time_stamp.count()) + "ns: "
                    + std::to_string(value)
                    );
            }
            _number_of_receptions_done++;
        }
        else
        {
            if(_verbose)
            {
                FEP3_LOG_INFO("No further reception performed because number of receptions has been reached.");
            }
            if (!_finished)
            {
                _finished = true;
                // notify tester about finish
                FEP3_LOG_INFO("Participant finished work");
            }
        }

        return {};
    }

    fep3::Result reset()
    {
        updatePropertyVariables();
        _number_of_receptions_done = 0;
        if (!_data_one_logger.isReady())
        {
            _data_one_logger.openLogFile(_log_file_path_data_one_property_variable);
        }
        if (!_data_two_logger.isReady())
        {
            _data_two_logger.openLogFile(_log_file_path_data_two_property_variable);
        }
        return {};
    }

private:
    DataReader* _reader_data_one;
    DataReader* _reader_data_two;
    fep3::Optional<int32_t> _value_data_one{};
    fep3::Optional<int32_t> _value_data_two{};
    test::TestFileLogger _data_one_logger;
    test::TestFileLogger _data_two_logger;

    fep3::cpp::PropertyVariable<bool> _verbose{ false };
    fep3::cpp::PropertyVariable<std::string> _log_file_path_data_one_property_variable{};
    fep3::cpp::PropertyVariable<std::string> _log_file_path_data_two_property_variable{};
    fep3::cpp::PropertyVariable<uint32_t> _number_of_receptions_property_variable{};
    uint32_t _number_of_receptions_done{ 0 };
    bool _finished{ false };
};

int main(int argc, char* argv[])
{
    try
    {
        auto participant = core::createParticipant<test::DataJobElementFactoryWithComponents<ReceiveJob>>
            (argc
            , argv
            , FEP3_PARTICIPANT_LIBRARY_VERSION_STR
            );
        return participant.exec();
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what();
        return -1;
    }
    catch (...)
    {
        std::cerr << "exception caught";
        return -1;
    }
}
