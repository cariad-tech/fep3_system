
/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "fep_system_state_transition_helper.h"

#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <algorithm>
#include <functional>
#include <type_traits>
namespace fep3 {

template <typename ParticipantProxyRange,
          typename TransitionFunction,
          typename ExecPolicy,
          typename ErrorHandling>
bool performTransition(ParticipantProxyRange& proxies_range,
                       TransitionFunction trans_function,
                       ExecPolicy& execution_policy,
                       ErrorHandling error_handling,
                       StateInfo state_info)
{
    bool continue_loop = true;
    while (continue_loop) {
        const auto max_prio = proxies_range.at(0)._prio;
        const auto middle = boost::range::partition(proxies_range, [max_prio](const auto& proxy_meta) {
            return proxy_meta._prio == max_prio;
        });
        auto participants = boost::make_iterator_range(boost::begin(proxies_range), middle);

        /// do transition
        std::vector<TransitionFunctionMetaData> transition_functions;

        std::transform(participants.begin(),
                       participants.end(),
                       std::back_insert_iterator(transition_functions),
                       [&](auto& proxy_meta) {
                           return TransitionFunctionMetaData{
                               [&proxy_meta, trans_function, &state_info]() {
                                   return trans_function(proxy_meta._ref.get());
                               },
                               proxy_meta._ref.get().getName(),
                               state_info,
                               ExecResult{}};
                       });

        std::invoke(execution_policy, transition_functions);
        if (const bool ret = error_handling(transition_functions); !ret) {
            return ret;
        }
        boost::range::erase(proxies_range, participants);
        continue_loop = !boost::empty(proxies_range);
    }
    return true;
}

template <typename ProxyType>
struct PartProxyTransitionMetaData {
    std::reference_wrapper<ProxyType> _ref;
    int32_t _prio;
    std::function<bool(int32_t, int32_t)> _prio_comp_function;

    bool operator<(const PartProxyTransitionMetaData& rhs) const
    {
        auto ret_val = _prio_comp_function(_prio, rhs._prio);
        return ret_val;
    }
};

template <typename PrioFunction, typename ErrorHandling, typename ExecPolicy>
class SystemStateTransition {
public:
    SystemStateTransition(PrioFunction prio_function,
                          SystemStateTransitionPrioSorting sorting,
                          ErrorHandling error_handling,
                          ExecPolicy&& execution_policy)
        : _prio_function(prio_function),
          _sorting(sorting),
          _error_handling(error_handling),
          _execution_policy(std::forward<ExecPolicy>(execution_policy))
    {
    }

    template <typename ParticipantProxyRange, typename TransitionFunction>
    auto execute(ParticipantProxyRange& proxies,
                 TransitionFunction transition_function,
                 StateInfo state_info)
    {
        auto metaData = getTransitionMetaData(proxies);

        if (_sorting != SystemStateTransitionPrioSorting::none) {
            boost::range::sort(metaData);
        }

        return performTransition(
            metaData, transition_function, _execution_policy, _error_handling, state_info);
    }

private:
    template <typename ParticipantProxyRange>
    std::vector<PartProxyTransitionMetaData<typename ParticipantProxyRange::value_type>>
    getTransitionMetaData(ParticipantProxyRange& proxies)
    {
        using ValueType = typename ParticipantProxyRange::value_type;
        std::vector<PartProxyTransitionMetaData<ValueType>> metaData;
        std::transform(
            proxies.begin(), proxies.end(), std::back_insert_iterator(metaData), [&](auto& proxy) {
                return PartProxyTransitionMetaData<ValueType>{
                    std::ref(proxy), _prio_function(proxy), getSortFunction(_sorting)};
            });
        return metaData;
    }

    template <typename ProxyType>
    std::vector<PartProxyTransitionMetaData<ProxyType>> getTransitionMetaData(
        std::vector<std::reference_wrapper<ProxyType>>& proxies)
    {
        std::vector<PartProxyTransitionMetaData<ProxyType>> metaData;
        std::transform(
            proxies.begin(), proxies.end(), std::back_insert_iterator(metaData), [&](auto& proxy) {
                return PartProxyTransitionMetaData<ProxyType>{
                    proxy, _prio_function(proxy.get()), getSortFunction(_sorting)};
            });
        return metaData;
    }

    PrioFunction _prio_function;
    SystemStateTransitionPrioSorting _sorting;
    ErrorHandling _error_handling;
    ExecPolicy _execution_policy;
};
} // namespace fep3
