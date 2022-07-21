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


#pragma once

#include <string>
#include <map>
#include <chrono>
#include "fep_system_types.h"
#include "participant_proxy.h"
#include "base/logging/logging_types.h"
#include "base/health/health_types.h"
#include "logging_types_legacy.h"
#include "event_monitor_intf.h"

///The fep::System default timeout for every fep3::System call that need to connect to a far participant
#define FEP_SYSTEM_DEFAULT_TIMEOUT std::chrono::milliseconds(500)
///The fep::System state transition timeout for every fep3::System call that need to change the states of the participants
#define FEP_SYSTEM_TRANSITION_TIMEOUT std::chrono::milliseconds(10000)
///The fep::discoverSystem default timeout
#define FEP_SYSTEM_DISCOVER_TIMEOUT std::chrono::milliseconds(1000)
///The fep::ParticipantProxy default timeout for every fep::ParticipantProxy call that need to connect the participant
#define PARTICIPANT_DEFAULT_TIMEOUT std::chrono::milliseconds(1000)

namespace fep3
{
    /**
     * @brief The aggregated system state as participant state.
     *
     */
    using SystemAggregatedState = fep3::rpc::ParticipantState;
    /**
     * @brief System state
     * The aggregated state is always the lowest state of the participants.
     * if the system state is homogeneous,
     * then every participant in the system have the same participant state.
     *
     * Each participant where the participant state is \p undefined is not considered except
     * every participant is \p undefined.
     */
    struct SystemState
    {
        /**
         * @brief CTOR
         *
         */
        SystemState() = default;
        /**
         * @brief DTOR
         *
         */
        ~SystemState() = default;
        /**
         * @brief Move DTOR
         *
         */
        SystemState(SystemState&&) = default;
        /**
         * @brief Copy DTOR
         *
         */
        SystemState(const SystemState&) = default;
        /**
         * @brief move assignment
         *
         * @return SystemState&
         */
        SystemState& operator=(SystemState&&) = default;
        /**
         * @brief copy assignment
         *
         * @return SystemState&
         */
        SystemState& operator=(const SystemState&) = default;
        /**
         * @brief CTOR to set values immediately.
         *
         * @param[in] homogeneous if the system state is homogeneous,
         *                    then every participant in the system have the same participant state
         * @param[in] state the aggregated state is always the lowest state of the participants
         */
        SystemState(bool homogeneous, const SystemAggregatedState& state) : _homogeneous(homogeneous), _state(state) {}
        /**
         * @brief CTOR to set a homogenous state.
         *
         * @param[in] state the aggregated state is always the lowest state of the participants
         */
        SystemState(const SystemAggregatedState& state) : _homogeneous(true), _state(state) {}
        /**
         * @brief homogenous value
         *
         */
        bool _homogeneous;
        /**
         * @brief the aggregated state is always the lowest state of the participants
         *
         */
        SystemAggregatedState _state;
    };

    /**
     * @brief FEP System class is a collection of fep3::ParticipantProxy.
     *
     * You may create a fep::System by using the CTOR fep3::System()
     * @code
     *
     * //this functionality may be used if you know your system
     * //construct a system that will be connected to the default discover address
     * fep::System my_system("my_system");
     * my_system.add("my_participant_1");
     * //now your system contains one participant
     * my_system.add("my_participant_2");
     * //now your system contains two participants
     *
     * @endcode
     *
     * @see @ref fep3::discoverSystem
     */
    class FEP3_SYSTEM_EXPORT System
    {
    public:
        ///System state
        using State = SystemState;
        ///Definition of the Aggregated system state
        using AggregatedState = SystemAggregatedState;

    public:
        /**
         * @brief Construct a new System object
         *
         * @remark at the moment of FEP 2 there is no system affiliation implemented within the participants.
         *         for fep3 the system name will be considered. If empty the system name is really not set!
         */
        System();
        /**
         * @brief Construct a new System object
         *
         * @param[in]  system_name the name of the system to construct
         * @remark at the moment of FEP 2 there is no system affiliation implemented within the participants.
         */
        System(const std::string& system_name);
        /**
         * @brief Construct a new System object
         *
         * @param[in]  system_name the name of the system to construct
         * @param[in]  system_discovery_url the url for discovery of the system
         * @remark at the moment of FEP 2 there is no system affiliation implemented within the participants.
         */
        System(const std::string& system_name, const std::string& system_discovery_url);
        /**
         * @brief Copy Construct a new System object
         *
         * @param[in] other the other system object to copy from
         * @remark we cannot copy the registered event monitor!
         *         This is also a very expensive operation; use move instead if possible!
         */
        System(const System& other);
        /**
         * @brief Move construct a new System object
         *
         * @param[in] other the other system object to move from
         */
        System(System&& other);
        /**
         * @brief Copy assignment
         *
         * @param[in] other the other system object to copy from
         * @remark we cannot copy the registered event monitor!
         *         This is also a very expensive operation; use move instead if possible!
         * @return copied system
         */
        System& operator=(const System& other);
        /**
         * @brief Move assignment
         *
         * @param[in] other the other system object to move from
         * @return moved system
         */
        System& operator=(System&& other);
        /**
         * @brief Destroy the System object
         *
         */
        virtual ~System();

        /**
         * @brief Sets the system state
         * depending on the current state this function will increase or decrease
         * the system state to the given AggregatedState.
         * Only homogenous system states are valid as starting point.
         *
         * @param[in] state the aggregated state to set
         * @param[in] timeout the timeout used for each state change
         * @throw runtime_error if a homogenous state cannot be reached or the starting state is not homogenous
         *
         */
        void setSystemState(System::AggregatedState state,
                            std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;

        /**
         * @brief sends a load event to every participant
         *
         * @param[in] timeout timeout for waiting on the response of every participant
         * @throw throws a logical_error if a participant declined the state change (i.e. it is in the wrong state)
         */
        void load(std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;
        /**
         * @brief sends an unload event to every participant
         *
         * @param[in] timeout timeout for waiting on the response of every participant
         * @throw throws a logical_error if a participant declined the state change (i.e. it is in the wrong state)
         */
        void unload(std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;
        /**
         * @brief sends a initialize event to every participant
         *
         * @param[in] timeout timeout for waiting on the response of every participant
         * @throw throws a logical_error if a participant declined the state change (i.e. it is in the wrong state)
         */
        void initialize(std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;
        /**
         * @brief sends a deinitialize event to every participant
         *
         * @param[in] timeout timeout for waiting on the response of every participant
         * @throw throws a logical_error if a participant declined the state change (i.e. it is in the wrong state)
         */
        void deinitialize(std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;

        /**
         * @brief sends a start event to every participant
         *
         * @param[in] timeout timeout for waiting on the response of every participant
         * @throw throws a logical_error if a participant declined the state change (i.e. it is in the wrong state)
         */
        void start(std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;
        /**
         * @brief sends a pause event to every participant
         *
         * @param[in] timeout timeout for waiting on the response of every participant
         * @throw throws a logical_error if a participant declined the state change (i.e. it is in the wrong state)
         */
        void pause(std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;
        /**
         * @brief sends a stop event to every participant
         *
         * @param[in] timeout timeout for waiting on the response of every participant
         * @throw throws a logical_error if a participant declined the state change (i.e. it is in the wrong state)
         */
        void stop(std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;
        /**
         * @brief sends a shutdown event to every participant
         *
         * @param[in] timeout timeout for waiting on the response of every participant
         * @throw throws a logical_error if a participant declined the state change (i.e. it is in the wrong state)
         */
        void shutdown(std::chrono::milliseconds timeout = FEP_SYSTEM_TRANSITION_TIMEOUT) const;

        /**
        * @c getParticipant returns the participant object
        * @param[in] participant_name name of the participant to retrieve
        * @return the participant proxy if found
        * @throw runtime_error if participant is not found
        */
        ParticipantProxy getParticipant(const std::string& participant_name) const;

        /**
        * @c getParticipants delivers a vector of all current participants that are part of the system
        *
        * @remark mind that the participant still belongs to the system
        * @return a vector of all participant proxies.
        */
        std::vector<ParticipantProxy> getParticipants() const;

       /**
        * @c adds the participant to the system
        * @param[in]   participant_name  name of the participant
        * @param[in]   participant_url url of the participant
        *
        */
        void add(const std::string& participant_name,
                 const std::string& participant_url = std::string());

        /**
        * @c adds the list of participants to the system
        * @param[in]   participants         list of participant names
        *
        */
        void add(const std::vector<std::string>& participants);

        /**
        * @c adds the list of participants to the system
        * @param[in]   participants         map of participant names and url pairs
        *
        */
        void add(const std::multimap<std::string, std::string>& participants);

        /**
        * @c adds the list of participants to the system in an asynchronous execution.
        *    Function returnes once the participants are added to the system.
        * @param[in]   participants         map of participant names and url pairs
        *
        */
        void addAsync(const std::multimap<std::string, std::string>& participants);

        /**
        * @c adds the list of participants to the system in an asynchronous execution
        *         Function returnes once the participants are added to the system.
        * @param[in]   participants         map of participant names and url pairs
        * @param[in]   pool_size            size of the thread pool to be used for the parallel execution.
        *
        */
        void addAsync(const std::multimap<std::string, std::string>& participants, uint8_t pool_size);

        /**
        * @c removes the participant from the system
        * @param[in]   participant_name         participant name
        * @remark existing references to participant proxy instances are disconnected from this system object
        *
        */
        void remove(const std::string& participant_name);

        /**
        * @c removes the list of participants from the system
        * @param[in]   participants         list of participant names
        * @remark existing references to participant proxy instances are disconnected from this system object
        */
        void remove(const std::vector<std::string>& participants);

        /**
        * @c clearSystem removes all current participants from the system
        * @remark existing references to participant proxy instances are disconnected from this system object
        */
        void clear();

        /**
        * Configures the timing. The general function is for custom timing types only.
        *
        * @param[in]  master_clock_name      The name of the clock of the timing master.
        * @param[in]  slave_clock_name       The name of the sync clock of the timing clients.
        * @param[in]  scheduler              The name of the scheduler.
        * @param[in]  master_element_id      The element id (participant name) of the timing master.
        * @param[in]  master_time_stepsize   The time in ns between discrete time steps. Won't be set if an empty string gets passed.
        * @param[in]  master_time_factor     Multiplication factor of the simulation speed. Won't be set if an empty string gets passed.
        * @param[in]  slave_sync_cycle_time  The time in ns between synchronizations. Won't be set if an empty string gets passed.
        *
        * @throws std::runtime_error if one of the timing properties cannot be set
        *
        * @note master_clock_name, master_time_stepsize, master_time_factor and slave_sync_cycle_time will only
        *       be set if the master_element_id is a non-empty string.
        *
        */
        void configureTiming(const std::string& master_clock_name, const std::string& slave_clock_name,
                             const std::string& scheduler, const std::string& master_element_id, const std::string& master_time_stepsize,
                             const std::string& master_time_factor, const std::string& slave_sync_cycle_time) const;

        // Native FEP 3 configurations
        /**
         * Configures the timing by resetting timing master.
         * Each particiapnt will work in system time by itself
         *
         * @throws std::runtime_error if one of the timing properties cannot be set
         *
         */
        void configureTiming3NoMaster() const;
        /**
         * Configures the timing by setting the participant with the master element name as master.
         *
         * @param[in]  master_element_id      The element id (participant name) of the timing master.
         * @param[in]  slave_sync_cycle_time  The time in ns between synchronizations. Won't be set if an empty string gets passed.
         *
         * @throws std::runtime_error if one of the timing properties cannot be set
         *
         * @note slave_sync_cycle_time will only
         *       be set if the master_element_id is a non-empty string.
         *
         */
        void configureTiming3ClockSyncOnlyInterpolation(const std::string& master_element_id, const std::string& slave_sync_cycle_time) const;
        /**
         * Configures the timing by setting the participant with the master element name as master.
         *
         * @param[in]  master_element_id      The element id (participant name) of the timing master.
         * @param[in]  slave_sync_cycle_time  The time in ns between synchronizations. Won't be set if an empty string gets passed.
         *
         * @throws std::runtime_error if one of the timing properties cannot be set
         *
         * @note slave_sync_cycle_time will only
         *       be set if the master_element_id is a non-empty string.
         *
         */
        void configureTiming3ClockSyncOnlyDiscrete(const std::string& master_element_id, const std::string& slave_sync_cycle_time) const;
        /**
         * Configures the timing by setting the participant with the master element name as master,
         * the master clock is a discrete clock.
         *
         * @param[in]  master_element_id      The element id (participant name) of the timing master.
         * @param[in]  master_time_stepsize   The time in ns between discrete time steps. Won't be set if an empty string gets passed.
         * @param[in]  master_time_factor     Multiplication factor of the simulation speed. Won't be set if an empty string gets passed.
         *
         * @throws std::runtime_error if one of the timing properties cannot be set
         *
         * @note master_time_stepsize and master_time_factor will only
         *       be set if the master_element_id is a non-empty string.
         *
         */
        void configureTiming3DiscreteSteps(const std::string& master_element_id, const std::string& master_time_stepsize, const std::string& master_time_factor) const;
        /**
         * Configures the timing by setting the participant with the master element name as master,
         * the master clock is a discrete clock.
         * This is the same call than configureTiming3DiscreteSteps with time factor 0.0
         *
         *
         * @param[in]  master_element_id      The element id (participant name) of the timing master.
         * @param[in]  master_time_stepsize   The time in ns between discrete time steps. Won't be set if an empty string gets passed.
         *
         * @throws std::runtime_error if one of the timing properties cannot be set
         *
         * @note master_time_stepsize will only
         *       be set if the master_element_id is a non-empty string.
         *
         */
        void configureTiming3AFAP(const std::string& master_element_id, const std::string& master_time_stepsize) const;

        /**
         * retrieves the current set timing masters set on each participant.
         * @return the list of the timing masters within the system.
         */
        std::vector<std::string> getCurrentTimingMasters() const;

        /**
         * retrieves the timing related properties on each participant.
         * @return participant name-property list map of timing related properties.
         */
        std::map<std::string, std::unique_ptr<IProperties>> getTimingProperties() const;

        /**
        * @c getSystemState determines the aggregated state of all participants in a system.
        * To find the aggregated state of the system, the states of all participants are sorted
        * according to their weight (see below). The lowest state in the list is chosen.
        * The relative weights are:
        *         Shutdown < Error < Startup < Idle < Initializing < Ready < Running.
        *
        * @note Even if the timeout parameter is lower than 500 ms, the call will
        *       still take at least 500 ms if no participant list is specified
        * @note This method is _not_ thread safe. Do not call concurrently inside the same participant!
        *
        * @param[in]   timeout      (ms) time how long this method waits maximally for other
        *                           participants to respond
        *
        * @return State
        * @remark On Failure a IEventMonitor::onLog will be send with a detailed description
        */
        State getSystemState(std::chrono::milliseconds timeout = FEP_SYSTEM_DEFAULT_TIMEOUT) const;

        /**
         * @brief Get the System Name object
         *
         * @return std::string the name given at construction time
         */
        std::string getSystemName() const;

        /**
         * @brief Get the System Discovery Url object
         *
         * @return std::string the name given at construction time
         */
        std::string getSystemUrl() const;

        /**
        * Register monitoring listener for state and name changed notifications of the whole system
        *
        * @param[in] event_listener The listener
        * @retval true/false        Everything went fine/Something went wrong.
        * On Failure an Incident will be send with a detailed description
        */
        void registerMonitoring(IEventMonitor& event_listener);

        /**
         * Unregister monitoring listener for state and name changed and logging notifications
         *
         * @param[in] event_listener The listener
         */
        void unregisterMonitoring(IEventMonitor& event_listener);

        /**
         * @brief Set the System Severity Level for all participants
         *
         * @param[in] severity_level minimum severity level
         */
        void setSeverityLevel(LoggerSeverity severity_level);

        /// @cond no_doc
        protected:
            struct Implementation;
            std::unique_ptr<Implementation> _impl;
        /// @endcond no_doc
    };

namespace experimental
{
    /**
     * @brief The aggregated system health state as participant health state.
     *
     */
    using SystemAggregatedHealthState = experimental::HealthState;
    /**
     * @brief System health state
     * The aggregated health state is always the highest state of the participants.
     * If the system health state is homogeneous,
     * then every participant in the system has the same participant health state.
     */
    struct SystemHealthState
    {
        /**
         * @brief CTOR
         *
         */
        SystemHealthState() = default;
        /**
         * @brief DTOR
         *
         */
        ~SystemHealthState() = default;
        /**
         * @brief Move DTOR
         *
         */
        SystemHealthState(SystemHealthState&&) = default;
        /**
         * @brief Copy DTOR
         *
         */
        SystemHealthState(const SystemHealthState&) = default;
        /**
         * @brief move assignment
         *
         * @return SystemHealthState
         */
        SystemHealthState& operator=(SystemHealthState&&) = default;
        /**
         * @brief copy assignment
         *
         * @return SystemHealthState
         */
        SystemHealthState& operator=(const SystemHealthState&) = default;
        /**
         * @brief CTOR to set values immediately.
         *
         * @param[in] homogeneous if the system health state is homogeneous,
         *                    then every participant in the system has the same participant health state
         * @param[in] system_health_state the aggregated health state is always the highest health state of the participants
         */
        SystemHealthState(bool homogeneous, const SystemAggregatedHealthState& system_health_state) : _homogeneous(homogeneous), _system_health_state(system_health_state) {}
        /**
         * @brief CTOR to set a homogenous health state.
         *
         * @param[in] system_health_state the aggregated health state is always the highest health state of the participants
         */
        SystemHealthState(const SystemAggregatedHealthState& system_health_state) : _homogeneous(true), _system_health_state(system_health_state) {}
        /**
         * @brief homogenous value
         *
         */
        bool _homogeneous;
        /**
         * @brief the aggregated health state is always the highest health state of the participants
         *
         */
        SystemAggregatedHealthState _system_health_state;
    };

    /**
     * @brief fep3::experimental::System is a System class which derives from fep3::System
     * and contains additional experimental functionality. This functionality is not stable yet
     * and likely to change in the future.
     */
    class FEP3_SYSTEM_EXPORT System : public fep3::System
    {
    public:
        /**
         * @brief CTOR
         *
         */
        System();

        /**
         * @brief CTOR
         *
         * @param[in]  system_name the name of the system to construct
         */
        System(const std::string& system_name);

        /**
         * @brief CTOR
         *
         * @param[in]  system_name the name of the system to construct
         * @param[in]  system_discovery_url the url for discovery of the system
         */
        System(const std::string& system_name, const std::string& system_discovery_url);

        /**
         * @brief DTOR
         *
         */
        virtual ~System();

        /**
         * @brief Get the health state of a specific participant.
         *
         * @param participant_name the name of the participant to retrieve the health state for
         * @return HealthState the participant health state retrieved
         */
        experimental::HealthState getHealth(const std::string& participant_name) const;

        /**
         * @brief Get the system health state.
         * The system health state is equal to the worst participant health state of the system.
         *
         * @return SystemHealthState the system health state retrieved
         */
        experimental::SystemHealthState getSystemHealth() const;

        /**
         * Reset the health state of a specific participant to ok.
         *
         * @param participant_name the name of the participant to set the health state for
         * @param message message indicating the reason for the health state change
         * @return Result indicating whether the rpc call succeeded or failed
         * @retval ERR_NOERROR in case of successful rpc call
         */
        a_util::result::Result resetHealth(const std::string& participant_name, const std::string& message);
        /// @cond no_doc
        private:
            struct Implementation;
            std::unique_ptr<Implementation> _impl;
        /// @endcond no_doc
    };
} // namespace experimental

    /**
     * discoverSystem discovers all participants which are added to the system named by @p name.
     * The default url will be taken which is provided by fep participant library.
     *
     * It will use the default service bus discovery provided by the fep participant library
     *
     * @param[in]   name      name of the system which is discovered
     * @param[in]   timeout   (ms) timeout for remote request; has to be positive
     * @return Discovered system
     * @throw runtime_error throws if one of the discovered participants is not available
     */
    System FEP3_SYSTEM_EXPORT discoverSystem(std::string name,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

    /**
     * discoverSystemByURL discovers all participants which are added to the system named by @p name
     * which are discoverable on the given \p discover_url .
     *
     * It will use the default service bus discovery provided by the fep participant library.
     *
     * @param[in]   name           name of the system which is discovered
     * @param[in]   discover_url   url where the systems can be discovered
     * @param[in]   timeout       (ms) timeout for remote request; has to be positive
     * @return Discovered system
     * @throw runtime_error throws if one of the discovered participants is not available
     */
    System FEP3_SYSTEM_EXPORT discoverSystemByURL(std::string name,
        std::string discover_url,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);
    /**
     * discoverAllSystems discovers all participants at all systems on the default discovery url.
     * The default url will be taken which is provided by fep participant library.
     *
     * It will use the default service bus discovery provided by the fep participant library
     *
     * @param[in]   timeout   (ms) timeout for remote request; has to be positive
     * @return vector of discovered systems
     * @throw runtime_error throws if one of the discovered participants is not available
     *                      (for filtering of standalone participants)
     */
    std::vector<System> FEP3_SYSTEM_EXPORT discoverAllSystems(std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

    /**
    * Loads the service bus plugin.
    */
    void FEP3_SYSTEM_EXPORT preloadServiceBusPlugin();

    /**
     * discoverAllSystemsByURL discovers all participants at all system
     * which are discoverable on the given \p discover_url
     *
     * It will use the default service bus discovery provided by the fep participant library.
     *
     * @param[in]   discover_url   url where the systems can be discovered
     * @param[in]   timeout   (ms) timeout for remote request; has to be positive
     * @return vector of discovered systems
     * @throw runtime_error throws if one of the discovered participants is not available
     *                      (for filtering of standalone participants)
     */
    std::vector<System> FEP3_SYSTEM_EXPORT discoverAllSystemsByURL(std::string discover_url,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

}
