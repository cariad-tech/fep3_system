/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */


#include <fep_system/participant_proxy.h>
#include <private_participant_proxy.hpp>

namespace fep3
{

ParticipantProxy::~ParticipantProxy()
{
}

ParticipantProxy::ParticipantProxy(const std::string& participant_name,
    const std::string& participant_url,
    const std::string& system_name,
    const std::string& system_discovery_url,
    const std::string& rpc_server_url,
    std::shared_ptr<ISystemLogger> system_logger,
    std::chrono::milliseconds default_timeout)
{
    _impl.reset(new Implementation(participant_name,
                                   participant_url,
                                   system_name,
                                   system_discovery_url,
                                   rpc_server_url,
                                   system_logger,
                                   default_timeout));
}
ParticipantProxy::ParticipantProxy(ParticipantProxy&& other)
{
    _impl = other._impl;
}

ParticipantProxy& ParticipantProxy::operator=(ParticipantProxy&& other)
{
    _impl = other._impl;
    return *this;
}

ParticipantProxy::ParticipantProxy(const ParticipantProxy& other) : _impl(other._impl)
{
}

ParticipantProxy& ParticipantProxy::operator=(const ParticipantProxy& other)
{
    _impl = other._impl;
    return *this;
}

void ParticipantProxy::copyValuesTo(ParticipantProxy& other) const
{
    _impl->copyValuesTo(*(other._impl));
}

void ParticipantProxy::setInitPriority(int32_t priority)
{
    _impl->setInitPriority(priority);
}

void ParticipantProxy::setStartPriority(int32_t priority)
{
    _impl->setStartPriority(priority);
}

int32_t ParticipantProxy::getInitPriority() const
{
    return _impl->getInitPriority();
}

int32_t ParticipantProxy::getStartPriority() const
{
    return _impl->getStartPriority();
}

std::string ParticipantProxy::getName() const
{
    return _impl->getParticipantName();
}

std::string ParticipantProxy::getUrl() const
{
    return _impl->getParticipantURL();
}


void ParticipantProxy::setAdditionalInfo(const std::string& key, const std::string& value)
{
    _impl->setAdditionalInfo(key, value);
}

std::string ParticipantProxy::getAdditionalInfo(const std::string& key, const std::string& value_default) const
{
    return  _impl->getAdditionalInfo(key, value_default);
}

bool ParticipantProxy::getRPCComponentProxy(const std::string& component_name,
    const std::string& component_iid,
    IRPCComponentPtr& proxy_ptr) const
{
    return _impl->getRPCComponentProxy(component_name,
        component_iid,
        proxy_ptr);
}

bool ParticipantProxy::getRPCComponentProxyByIID(const std::string& component_iid,
    IRPCComponentPtr& proxy_ptr) const
{
    return _impl->getRPCComponentProxyByIID(component_iid,
        proxy_ptr);
}

void ParticipantProxy::deregisterLogging()
{
    _impl->deregisterLogging();
}

bool ParticipantProxy::loggingRegistered() const
{
        return _impl->loggingRegistered();
}

ParticipantHealthUpdate ParticipantProxy::getParticipantHealth() const
{
    return _impl->getParticipantHealth();
}

void ParticipantProxy::setHealthListenerRunningStatus(bool running)
{
    return _impl->setHealthListenerRunningStatus(running);
}

bool ParticipantProxy::getHealthListenerRunningStatus() const
{
    return _impl->getHealthListenerRunningStatus();
}

void ParticipantProxy::setNotReachable() const
{
    _impl->setNotReachable();
}

}
