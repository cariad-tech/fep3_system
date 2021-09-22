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


#include "local_system_clock_discrete.h"

#include <iostream>

#include <fep3/components/clock/clock_service_intf.h>
#include "../event_listener/event_listener_rpc_intf_def.h"
#include "../event_listener/event_listener_registration_impl.h"

namespace
{
    constexpr auto simulation_time_start_value = 0;
}

namespace fep3
{
namespace test
{

DiscreteClockUpdater::DiscreteClockUpdater()
    : _simulation_time(simulation_time_start_value)
    , _next_request_gettime(std::chrono::time_point<std::chrono::steady_clock>{Timestamp{ 0 }})
    , _step_size(FEP3_CLOCK_SIM_TIME_STEP_SIZE_DEFAULT_VALUE)
    , _time_factor(FEP3_CLOCK_SIM_TIME_TIME_FACTOR_DEFAULT_VALUE)
    , _stop(false)
{
}

DiscreteClockUpdater::~DiscreteClockUpdater()
{
    stopWorking();
}

void DiscreteClockUpdater::startWorking()
{
    using namespace std::chrono;

    _simulation_time = Timestamp{ 0 };
    _next_request_gettime = time_point<steady_clock>{Timestamp{ 0 }};
    _stop = false;
    _worker = std::thread([this] { work();  });
}

void DiscreteClockUpdater::stopWorking()
{
    _stop = true;
    if (_worker.joinable())
    {
        _worker.join();
    }
}

void DiscreteClockUpdater::work()
{
    using namespace std::chrono;

    while (!_stop)
    {
        if (time_point<steady_clock>{Timestamp{ 0 }} == _next_request_gettime)
        {
            // no need to wait
        }
        else
        {
            std::unique_lock<std::mutex> guard(_clock_updater_mutex);

            // If time factor is configured to be '0,0', we do not wait between time steps
            if (FEP3_CLOCK_SIM_TIME_TIME_FACTOR_AFAP_VALUE != _time_factor)
            {
                auto current_demand_time_diff = (
                    _next_request_gettime - steady_clock::now()) / _time_factor;

                // If the system timestamp for the next discrete time step is not reached yet, we
                // wait
                if (current_demand_time_diff > Timestamp{ 0 })
                {
                    _cycle_wait_condition.wait_for(
                        guard, current_demand_time_diff);
                }
            }
        }
        _next_request_gettime = steady_clock::now() + _step_size;

        try
        {
             _simulation_time += _step_size;

            {
                std::lock_guard<std::mutex> guard(_clock_updater_mutex);
                updateTime(_simulation_time);
            }

        }
        catch (std::exception& exception)
        {
            std::cout << "Caught an exception during update of simulation time: "
                << exception.what() << std::endl;
        }
    }
}

void DiscreteClockUpdater::updateConfiguration(const Duration step_size, const double time_factor)
{
    _step_size = step_size;
    _time_factor = time_factor;
}

LocalSystemSimClockWithTimeout::LocalSystemSimClockWithTimeout(IServiceBus* service_bus)
    : DiscreteClockUpdater()
    , DiscreteClock("local_system_simtime_with_timeout")
    , _service_bus(service_bus)
{
    _service_bus->getServer()->registerService(
        fep3::rpc::IRPCEventListenerRegistrationDef::getRPCDefaultName(),
        std::make_shared<EventListenerRegistrationServiceImpl>(*this));
}

LocalSystemSimClockWithTimeout::~LocalSystemSimClockWithTimeout()
{
    std::lock_guard<std::mutex> lock(_sync_clients);
    _service_bus->getServer()->unregisterService(fep3::rpc::IRPCEventListenerRegistrationDef::getRPCDefaultName());
    _service_bus = nullptr;
    _clients.clear();
}

void LocalSystemSimClockWithTimeout::start(const std::weak_ptr<IEventSink>& _sink)
{
    DiscreteClock::start(_sink);
    DiscreteClockUpdater::startWorking();
}

void LocalSystemSimClockWithTimeout::stop()
{
    DiscreteClockUpdater::stopWorking();
    DiscreteClock::stop();
}

void LocalSystemSimClockWithTimeout::updateTime(Timestamp new_time)
{
    if (new_time > _max_runtime)
    {
        std::lock_guard<std::mutex> lock(_sync_clients);

        for (const auto& current_client : _clients)
        {
            try
            {
                current_client.second->notify();
            }
            catch (const jsonrpc::JsonRpcException&)
            {
                //TODO: log exception
            }
            catch (const std::exception&)
            {
                //TODO: log exception
            }
        }
        stop();
    }
    else
    {
        DiscreteClock::setNewTime(new_time, true);
    }
}

int LocalSystemSimClockWithTimeout::registerEventListenerRPCService(const std::string& address)
{
    std::lock_guard<std::mutex> lock(_sync_clients);
    if (_service_bus)
    {
        _clients[address].reset(new EventListenerClient(rpc::IRPCEventListenerDef::getRPCDefaultName(),
            _service_bus->getRequester(address, true)));
        return 0;
    }
    else
    {
        // This should only happen during shutdown
        return ERR_INVALID_STATE.getCode();
    }
}

int LocalSystemSimClockWithTimeout::unregisterEventListenerRPCService(const std::string& address)
{
    std::lock_guard<std::mutex> lock(_sync_clients);
    _clients.erase(address);
    return 0;
}

void LocalSystemSimClockWithTimeout::setMaxRuntime(const Timestamp max_runtime)
{
    _max_runtime = max_runtime;
}

} // namespace native
} // namespace fep3
