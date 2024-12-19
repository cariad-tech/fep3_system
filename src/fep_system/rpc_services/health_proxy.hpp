/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#pragma once


#include "health_service_proxy_stub.h"
#include "rpc_services/health/health_service_rpc_intf.h"
#include "rpc_services/base/fep_rpc_json_to_result.h"

#include <components/service_bus/rpc/fep_rpc_stubs_client.h>
#include <fep3/components/service_bus/rpc/fep_rpc.h>
#include <string>
#include <cassert>

namespace fep3::rpc::catelyn
{

//we use the namespace here to create versioned Proxies if something changes in future

class HealthServiceProxy : public rpc::arya::RPCServiceClientProxy<rpc_proxy_stub::RPCHealthServiceProxy,
    IRPCHealthService>
{
private:
    using base_type =
        rpc::RPCServiceClientProxy<rpc_proxy_stub::RPCHealthServiceProxy, IRPCHealthService>;

public:
    using base_type::GetStub;
    HealthServiceProxy(
        std::string rpc_component_name,
        std::shared_ptr<::fep3::IRPCRequester> rpc) :
        base_type(rpc_component_name, rpc)
    {
    }
    //TODO MAYBE OPTIONAL
    std::vector<JobHealthiness> getHealth() const override
    {
       try
        {
            auto ret_value = GetStub().getHealth();
            if (!ret_value.isMember("jobs_healthiness"))
            {
                return {};
            }

            auto json_value_array = ret_value["jobs_healthiness"];
            if (json_value_array.isArray())
            {
                std::vector<JobHealthiness> jobs_healthiness;
                jobs_healthiness.reserve(json_value_array.size());

                for (auto json_value_job_health = json_value_array.begin(); json_value_job_health != json_value_array.end(); ++json_value_job_health)
                {
                    auto& healthiness = (*json_value_job_health);
                    JobHealthiness job_healthiness =

                    {
                       healthiness["job_name"].asString(),
                       getJobInfo(healthiness),
                       std::chrono::nanoseconds(healthiness["simulation_timestamp"].asInt64()) };

                    job_healthiness.execute_data_in_error = json_to_execute_error(healthiness["last_execute_data_in_error"]);
                    job_healthiness.execute_error = json_to_execute_error(healthiness["last_execute_error"]);
                    job_healthiness.execute_data_out_error = json_to_execute_error(healthiness["last_execute_data_out_error"]);

                    jobs_healthiness.push_back(std::move(job_healthiness));
                }
                return jobs_healthiness;
            }
            else
            {
                return {};
            }
        }
        catch (const std::exception&)
        {
            return {};
        }
    }

    fep3::Result resetHealth() override
    {
        try
        {
            return fep3::rpc::arya::jsonToResult(GetStub().resetHealth());
        }
        catch (const std::exception& exception)
        {
            return fep3::Result{ERR_UNKNOWN, exception.what(), 0, "", ""};
        }
    }
private:
    std::variant<fep3::JobHealthiness::ClockTriggeredJobInfo, fep3::JobHealthiness::DataTriggeredJobInfo>
        getJobInfo(const Json::Value& value) const
    {
        if (!value["cycle_time"].empty())
        {
            return fep3::JobHealthiness::ClockTriggeredJobInfo{ std::chrono::nanoseconds(value["cycle_time"].asInt64()) };
        }
        else
        {
            assert(value["trigger_signals"].isArray());
            const auto& array_val = value["trigger_signals"];
            std::vector<std::string> trigger_signals;
            trigger_signals.reserve(value["trigger_signals"].size());

            for (decltype(value["trigger_signals"].size()) i = 0; i < value["trigger_signals"].size(); ++i)
            {
                trigger_signals.push_back(array_val[i].asString());
            }
            return fep3::JobHealthiness::DataTriggeredJobInfo{ std::move(trigger_signals) };
        }
    }

    JobHealthiness::ExecuteError json_to_execute_error(const Json::Value& value) const
    {
        return JobHealthiness::ExecuteError{ value["error_count"].asUInt64(),
        std::chrono::nanoseconds(value["simulation_timestamp"].asInt64()),
        json_to_error(value["last_error"])};
    }

    fep3::JobHealthiness::ExecuteResult json_to_error(const Json::Value& value) const
    {
        return fep3::JobHealthiness::ExecuteResult{
            value["error_code"].asInt(),
            value["description"].asCString(),
            value["line"].asInt(),
            value["file"].asCString(),
            value["function"].asCString() };
    }
};

}
