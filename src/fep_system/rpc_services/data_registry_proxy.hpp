/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#pragma once
#include <string>

#include <fep3/components/service_bus/rpc/fep_rpc.h>
#include <fep3/base/stream_type/default_stream_type.h>
//this will be installed !!
#include "rpc_services/data_registry/data_registry_rpc_intf.h"
#include <fep_system_stubs/data_registry_proxy_stub.h>
#include "system_logger_intf.h"
#ifdef SEVERITY_ERROR
#undef SEVERITY_ERROR
#endif

namespace fep3
{
namespace rpc
{
namespace catelyn
{
class DataRegistryProxy : public RPCServiceClientProxy<
    rpc_proxy_stub::RPCDataRegistryProxy,
    IRPCDataRegistry>
{
private:
    typedef RPCServiceClientProxy< rpc_proxy_stub::RPCDataRegistryProxy,
                                   IRPCDataRegistry > base_type;

public:
    using base_type::GetStub;
    DataRegistryProxy(
        std::string rpc_component_name,
        std::shared_ptr<IRPCRequester> rpc) :
        base_type(rpc_component_name, rpc)
    {
    }
    std::vector<std::string> getSignalsIn() const override
    {
        try
        {
            std::string signal_list = GetStub().getSignalInNames();
            return a_util::strings::split(signal_list, ",");
        }
        catch (...)
        {
            return std::vector<std::string>();
        }
    }
    std::vector<std::string> getSignalsOut() const override
    {
        try
        {
            std::string signal_list = GetStub().getSignalOutNames();
            return a_util::strings::split(signal_list, ",");
        }
        catch (...)
        {
            return std::vector<std::string>();
        }
    }

    std::shared_ptr<fep3::arya::IStreamType> getStreamType(const std::string& signal_name) const override
    {
         try {
             const auto json_value = GetStub().getStreamType(signal_name);
             auto stream_type = std::make_shared<base::StreamType>(
                 base::StreamMetaType(json_value["meta_type"].asString()));

             const auto properties = json_value["properties"];
             auto current_property = properties.begin();

             while (current_property != properties.end())
             {
                 stream_type->setProperty((*current_property)["name"].asString(),
                     (*current_property)["value"].asString(),
                     (*current_property)["type"].asString());
                 current_property++;
             }

             return stream_type;
         }
         catch (...)
         {
             return nullptr;
         }
    }

};
} // catelyn
} // rpc
} // fep3
