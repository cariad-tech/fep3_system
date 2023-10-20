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
#include "healthiness_types.h"
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
     * @brief A map of participant name and participant state.
     *
     */
    using ParticipantStates = std::map<std::string, fep3::rpc::arya::IRPCParticipantStateMachine::State>;

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

        /**
        * @brief Enum for execution policy in init and stat state transitions.
        */
        enum class InitStartExecutionPolicy { sequential, parallel };

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
         * Heterogeneous system states are supported as well as iterating multiple states at once.
         * When transitioning a heterogeneous system lowest states will be increased first
         * and highest states will be decreased last.
         * For example a system consisting of participant p1 in state unloaded and p2 in state running
         * which shall be transitioned to state initialized will transition as follows:
         * p1 -> loaded
         * p1 -> initialized
         * p2 -> initialized
         *
         * @param[in] state the aggregated state to set
         * @param[in] timeout the timeout used for each state change
         * @throw runtime_error if at least one participant is unreachable or the state to be set is invalid
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
        * @brief set a property of a participant to a specified value
        *
        * @param[in] participant_name name of the participant
        * @param[in] property_path    path and name of the property
        * @param[in] property_value   value of the property to set
        * @throw runtime_error        if participant is not found
        */
        void setParticipantProperty(const std::string& participant_name,
                                    const std::string& property_path,
                                    const std::string& property_value);

        /**
        * @brief get the value of a property of a participant
        *
        * @param[in] participant_name name of the participant
        * @param[in] property_path    path and name of the property
        * @return std::string         the value of the requested property
        * @throw runtime_error        if participant is not found
        */
        std::string getParticipantProperty(const std::string& participant_name,
                                           const std::string& property_path) const;

        /**
        * @brief get the state of a participant
        *
        * @param[in] participant_name   name of the participant
        * @return SystemAggregatedState the state of the requested participant
        * @throw runtime_error          if participant is not found
        */
        SystemAggregatedState getParticipantState(const std::string& participant_name) const;

        /**
        * @brief get the state of all participants
        *
        * @param[in] timeout timeout for waiting on the response of every participant
        * @return PartStates the states of all participants of system
        */
        ParticipantStates getParticipantStates(std::chrono::milliseconds timeout = FEP_SYSTEM_DEFAULT_TIMEOUT) const;

        /**
        * @brief set the state of a participant
        * CAUTION: Does not gurantee that the parcitipant will properly work in the target state
        *          See documentation of fep system for more details
        *
        * @param[in] participant_name   name of the participant
        * @param[in] participant_state  state of the participant to set
        * @throw runtime_error          if participant is not found
        */
        void setParticipantState(const std::string& participant_name, const SystemAggregatedState participant_state) const;

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
        * On Failure an Incident will be send with a detailed description
        * @param[in] event_listener The listener
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

        /**
         * @brief Set the execution policy for initialization and start state transition.
         *
         * @param[in] policy policy to use during init and start state transition.
         * @param[in] thread_count number of threads used to initialize the participants.
         * @throw runtime_error throws if thread count is 0
         */
        void setInitAndStartPolicy(InitStartExecutionPolicy policy, uint8_t thread_count);

        /**
         * @brief Returns the currennt execution policy for initialization and start state transition.
         *
         * @return std::pair<InitStartExecutionPolicy, uint8_t> First pair element is the policy
         *         and second is the thread count.
         */
        std::pair<InitStartExecutionPolicy, uint8_t> getInitAndStartPolicy();

        /**
         * @brief Returns the participants health.
         *
         * @return a map containing the participant name and health
         * @throw runtime_error throws if health listener is deactivated (using  @ref fep3::System::setHealthListenerRunningStatus) 
         */
        std::map<std::string, ParticipantHealth> getParticipantsHealth();

        /**
         * @brief Sets the time interval after the last notify alive discovery message the participant
         *  will be considered as alive. Default value is 20 seconds.
         *
         * @param[in] liveliness_timeout The time interval in ns.
         */
        void setLivelinessTimeout(std::chrono::nanoseconds liveliness_timeout);

        /**
         * @brief Returns the time interval after the last notify alive discovery message the participant
         *  will be considered as alive.
         *
         * @return std::chrono::nanoseconds the liveliness timeout.
         */
        std::chrono::nanoseconds getLivelinessTimeout() const;

        /**
         * Activates or deactivates the health listener. Per default the health listener is activated.
         * Deactivation will reduce the load of the system, since the polling of each participants health
         * with RPC calls is deactivated.
         * In case the listener is deactivated, polling the participants health with
         * @ref fep3::System::getParticipantsHealth, will result in exception.
         *
         * @param[in] running Set to true to activate the health listener and false to deactivate.
         */
        void setHealthListenerRunningStatus(bool running);

        /**
         * Returns the running state of the participants' health listeners.
         *
         * @return std::pair<bool, bool>, the first value of the pair indicates if all the
         * participants health listeners have the same running state. The second value indicates the
         * participants health listeners' running state (in case the first value is not true, the second value
         * is undefined).
         */
        std::pair<bool, bool> getHealthListenerRunningStatus() const;

        /**
         * @brief Set the heartbeat interval of one participant
         *
         * @param[in] participants participant names in a vector
         * @param[in] interval_ms heartbeat interval in milliseconds
         * @throw runtime_error throws if call to one of the discovered participants is not successful 
         */
        void setHeartbeatInterval(const std::vector<std::string>& participants, const std::chrono::milliseconds interval_ms);

        /**
         * @brief Get the Heartbeat interval of one participant
         *
         * @param[in] participant participant name
         * @return std::chrono::milliseconds the liveliness timeout.
         * @throw runtime_error throws if call to the participant is not successful
         */
        std::chrono::milliseconds getHeartbeatInterval(const std::string& participant) const;

        /// @cond no_doc
        protected:
            struct Implementation;
            std::unique_ptr<Implementation> _impl;
        /// @endcond no_doc
    };

    /**
     * @fn System discoverSystem(std::string name, std::chrono::milliseconds timeout)
     *
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
     * @fn System discoverSystem(std::string name,
     *   std::vector<std::string> participant_names,
     *   std::chrono::milliseconds timeout)
     *
     * discoverSystem discovers all participants which are added to the system named by @p name.
     * The default url will be taken which is provided by fep participant library. The discovery will return
     * once at least the participants by @p participant_names are discovered or the discovery time
     * defined by @p timeout expired.
     *
     * It will use the default service bus discovery provided by the fep participant library
     *
     * @param[in] name name of the system which is discovered
     * @param[in] participant_names names of the participants to be discovered
     * @param[in] timeout (ms) total time that discovery can take; has to be positive
     * @return Discovered system
     * @throw runtime_error throws if one of the discovered participants is not available or not all
     * participants defined in @p participant_names are discovered at the end of the timeout.
     */
    System FEP3_SYSTEM_EXPORT discoverSystem(std::string name,
        std::vector<std::string> participant_names,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

    /**
     * @fn System discoverSystem(std::string name,
     *   uint32_t participant_count,
     *   std::chrono::milliseconds timeout)
     *
     * discoverSystem discovers all participants which are added to the system named by @p name.
     * The default url will be taken which is provided by fep participant library. The discovery will return
     * once at least the number of participants set by @p participant_count are discovered or the discovery time
     * defined by @p timeout expired.
     *
     * It will use the default service bus discovery provided by the fep participant library
     *
     * @param[in] name name of the system which is discovered
     * @param[in] participant_count the number of participants that have to discovered for the discovery to stop.
     * @param[in] timeout (ms) total time that discovery can take; has to be positive
     * @return Discovered system
     * @throw runtime_error throws if one of the discovered participants is not available or the number of
     * participants defined in @p participant_count is not discovered at the end of the timeout.
     */
    System FEP3_SYSTEM_EXPORT discoverSystem(std::string name,
        uint32_t participant_count,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

    /**
     * @fn System discoverSystemByURL(std::string name,
     *   std::string discover_url,
     *   std::chrono::milliseconds timeout)
     *
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
     * @fn System discoverSystemByURL(std::string name,
     *   std::string discover_url,
     *   std::vector<std::string> participant_names,
     *   std::chrono::milliseconds timeout)
     *
     * discoverSystemByURL discovers all participants that are added to the system named by @p name,
     * which are discoverable on the given \p discover_url. The discovery will return
     * once at least the participants given by @p participant_names are discovered or the discovery time
     * defined by @p timeout expired.
     *
     * It will use the default service bus discovery provided by the fep participant library
     *
     * @param name name of the system which is discovered
     * @param[in]   discover_url   url where the systems can be discovered
     * @param participant_names names of the participants to be discovered
     * @param timeout (ms) total time that discovery can take; has to be positive
     * @return Discovered system
     * @throw runtime_error throws if one of the discovered participants is not available or not all
     * participants defined in @p participant_names are discovered at the end of the timeout.
     */
    System FEP3_SYSTEM_EXPORT discoverSystemByURL(std::string name,
        std::string discover_url,
        std::vector<std::string> participant_names,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

    /**
     * @fn System discoverSystemByURL(std::string name,
     *   std::string discover_url,
     *   uint32_t participant_count,
     *   std::chrono::milliseconds timeout)
     * 
     * discoverSystemByURL discovers all participants that are added to the system named by @p name,
     * which are discoverable on the given \p discover_url. The discovery will return
     * once at least the number of participants given by @p participant_count are discovered or the discovery time
     * defined by @p timeout expired.
     *
     * It will use the default service bus discovery provided by the fep participant library
     *
     * @param name name of the system which is discovered
     * @param[in]   discover_url   url where the systems can be discovered
     * @param[in] participant_count the number of participants that have to discovered for the discovery to stop.
     * @param[in] timeout (ms) total time that discovery can take; has to be positive
     * @return Discovered system
     * @throw runtime_error throws if one of the discovered participants is not available or the number of
     * participants defined in @p participant_count is not discovered at the end of the timeout.
     */
    System FEP3_SYSTEM_EXPORT discoverSystemByURL(std::string name,
        std::string discover_url,
        uint32_t participant_count,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

    /**
     * @fn System discoverAllSystems(
     *   std::chrono::milliseconds timeout)
     *
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
     * @fn System discoverAllSystems(
     *  uint32_t participant_count,
     *   std::chrono::milliseconds timeout)
     *
     * discoverAllSystems discovers all participants at all systems on the default discovery url.
     * The default url will be taken which is provided by fep participant library. The discovery will return
     * once at least the participants given by @p participant_names are discovered or the discovery time
     * defined by @p timeout expired.
     *
     * It will use the default service bus discovery provided by the fep participant library
     * @param[in] participant_count the number of participants that have to discovered for the discovery to stop.
     * @param[in]   timeout   (ms) timeout for remote request; has to be positive
     * @return vector of discovered systems
     * @throw runtime_error throws if one of the discovered participants is not available
     * (for filtering of standalone participants) or the number of
     * participants defined in @p participant_count is not discovered at the end of the timeout.
     */
    std::vector<System> FEP3_SYSTEM_EXPORT discoverAllSystems(
        uint32_t participant_count,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

    /**
    * Loads the service bus plugin.
    */
    FEP3_SYSTEM_EXPORT void preloadServiceBusPlugin();

    /**
     * @fn System discoverAllSystemsByURL(
     *   std::string discover_url,
     *   std::chrono::milliseconds timeout)
     *
     * discoverAllSystemsByURL discovers all participants at all systems
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

    /**
     * @fn System discoverAllSystemsByURL(
     *   std::string discover_url,
     *   uint32_t participant_count,
     *   std::chrono::milliseconds timeout)
     *
     * discoverAllSystemsByURL discovers all participants at all systems
     * which are discoverable on the given \p discover_url. The discovery will return
     * once at least the number of participants given by @p participant_count are discovered or the discovery time
     * defined by @p timeout expired.
     *
     * It will use the default service bus discovery provided by the fep participant library.
     *
     * @param[in]   discover_url   url where the systems can be discovered
     * @param[in] participant_count the number of participants that have to discovered for the discovery to stop.
     * @param[in]   timeout   (ms) timeout for remote request; has to be positive
     * @return vector of discovered systems
     * @throw runtime_error throws if one of the discovered participants is not available
     * (for filtering of standalone participants)  or the number of
     * participants defined in @p participant_count is not discovered at the end of the timeout.
     */
    std::vector<System> FEP3_SYSTEM_EXPORT discoverAllSystemsByURL(std::string discover_url,
        uint32_t participant_count,
        std::chrono::milliseconds timeout = FEP_SYSTEM_DISCOVER_TIMEOUT);

    /**
     * @fn SystemAggregatedState getSystemAggregatedStateFromString(const std::string& state_string)
     * Converts a std::string to ::SystemAggregatedState
     *
     * @param[in] state_string string to be converted
     * @return the std::string converted to the enum, returns fep3::SystemAggregatedState::undefined if conversion failed.
     */
    SystemAggregatedState FEP3_SYSTEM_EXPORT getSystemAggregatedStateFromString(const std::string& state_string);

    /**
     * @fn std::string systemAggregatedStateToString(const fep3::System::AggregatedState state)
     * Converts a ::SystemAggregatedState to std::string
     *
     * @param[in] state
     * @return The enum converted to std::string, "NOT RESOLVABLE" if conversion failed.
     */
    std::string FEP3_SYSTEM_EXPORT systemAggregatedStateToString(const fep3::System::AggregatedState state);

}
