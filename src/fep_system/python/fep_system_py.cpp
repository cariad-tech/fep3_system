/**
 * Copyright 2023 CARIAD SE.
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#if defined(_MSC_VER)
#pragma warning(disable: 4522) // warning C4522: multiple assignment operators specified
#endif

#include <fep_system.h>
#include <rpc_services/rpc_passthrough/rpc_passthrough_intf.h>
#include <pybind11/chrono.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <fep_system/logging_sink_csv.h>

namespace fep3
{

namespace py = pybind11;

// helper class to define new types of IEventMonitor within python
class PyEventMonitor : public IEventMonitor {
public:
    /* Inherit the constructor */
    using IEventMonitor::IEventMonitor;

    /* Trampoline (need one for each virtual function) */
    void onLog(std::chrono::milliseconds log_time,
        arya::LoggerSeverity severity_level,
        const std::string& participant_name,
        const std::string& logger_name,
        const std::string& message) override {
        py::gil_scoped_acquire acquire;
        PYBIND11_OVERRIDE_PURE(
            void,           /* Return type */
            IEventMonitor,  /* Parent class */
            onLog,          /* Name of function in C++ (must match Python name) */
            log_time, severity_level, participant_name, logger_name, message    /* Argument(s) */
        );
    }
};

// helper class to define an instance of rpc::RPCClient<rpc::experimental::IRPCPassthrough>
class PyRPCComponentPtr : public IRPCComponentPtr {
public:
    /* Inherit the constructors */
    using IRPCComponentPtr::IRPCComponentPtr;

    /* Trampoline (need one for each virtual function) */
    bool reset(const std::shared_ptr<rpc::arya::IRPCServiceClient>& other) override {
        py::gil_scoped_acquire acquire;
        PYBIND11_OVERRIDE_PURE(
            bool,               /* Return type */
            IRPCComponentPtr,   /* Parent class */
            reset,              /* Name of function in C++ (must match Python name) */
            other               /* Argument(s) */
        );
    }
};
PYBIND11_MODULE(fep3_system, m) {

    // class to get ParticipantInfo (return type for RPCComponent<rpc::IRPCParticipantInfo>::getInterface -> rpc::arya::ParticipantInfoProxy)
    py::class_<rpc::IRPCParticipantInfo, std::unique_ptr<rpc::IRPCParticipantInfo, py::nodelete>>(m, "IRPCParticipantInfo")
        .def("getRPCComponents", &rpc::IRPCParticipantInfo::getRPCComponents, py::call_guard<py::gil_scoped_release>())
        .def("getRPCComponentIIDs", &rpc::IRPCParticipantInfo::getRPCComponentIIDs,
            py::arg("object_name"), py::call_guard<py::gil_scoped_release>())
        .def("getRPCComponentInterfaceDefinition", &rpc::IRPCParticipantInfo::getRPCComponentInterfaceDefinition,
            py::arg("object_name"), py::arg("intf_name"), py::call_guard<py::gil_scoped_release>());

    // class to get Properties (return type for rpc::IRPCConfiguration::getProperties -> base::Properties<IProperties>)
    py::class_<IProperties, std::shared_ptr<IProperties>>(m, "IProperties")
        .def("getPropertyNames", &IProperties::getPropertyNames, py::call_guard<py::gil_scoped_release>())
        .def("getProperty", &IProperties::getProperty, py::call_guard<py::gil_scoped_release>())
        .def("getPropertyType", &IProperties::getPropertyType, py::call_guard<py::gil_scoped_release>());

    // class to get Configuration (return type for RPCComponent<rpc::IRPCConfiguration>::getInterface -> rpc::arya::ConfigurationProxy)
    py::class_<rpc::IRPCConfiguration, std::unique_ptr<rpc::IRPCConfiguration, py::nodelete>>(m, "IRPCConfiguration")
        .def("getProperties", py::overload_cast<const std::string&>(&rpc::arya::IRPCConfiguration::getProperties),
            py::arg("property_path"), py::call_guard<py::gil_scoped_release>());

    // class IRPCPassthrough and its member functions
    py::module_ experimental = m.def_submodule("experimental");
    py::class_<rpc::experimental::IRPCPassthrough, std::unique_ptr<rpc::experimental::IRPCPassthrough, py::nodelete>>(experimental, "IRPCPassthrough")
        // string is immutable in python, so use a lambda to return response
        .def("call", [](rpc::experimental::IRPCPassthrough& self, const std::string& request)
            {py::gil_scoped_release release; std::string response; bool result=self.call(request, response); return std::make_tuple(result, response);});

    // argument for getRPCComponentProxy
    // todo: IRPCComponentPtr provides a non public destructor, with the specified holder type memory leaks might occur
    py::class_<IRPCComponentPtr, PyRPCComponentPtr, std::unique_ptr<IRPCComponentPtr, py::nodelete>>(m, "IRPCComponentPtr")
        .def(py::init<>())
        .def("reset", &IRPCComponentPtr::reset, py::call_guard<py::gil_scoped_release>());

    // return types for getRPCComponentProxy
    // IRPCParticipantInfo
    py::class_<rpc::RPCClient<rpc::IRPCParticipantInfo>, IRPCComponentPtr>(m, "RPCClient_IRPCParticipantInfo")
        .def(py::init<>())
        .def("getInterface", &rpc::RPCClient<rpc::IRPCParticipantInfo>::getInterface, py::return_value_policy::reference, py::call_guard<py::gil_scoped_release>());
    // IRPCConfiguration
    py::class_<rpc::RPCClient<rpc::IRPCConfiguration>, IRPCComponentPtr>(m, "RPCClient_IRPCConfiguration")
        .def(py::init<>())
        .def("getInterface", &rpc::RPCClient<rpc::IRPCConfiguration>::getInterface, py::return_value_policy::reference, py::call_guard<py::gil_scoped_release>());
    // IRPCPassthrough
    py::class_<rpc::RPCClient<rpc::experimental::IRPCPassthrough>, IRPCComponentPtr>(m, "RPCClient_IRPCPassthrough")
        .def(py::init<>())
        .def("getInterface", &rpc::RPCClient<rpc::experimental::IRPCPassthrough>::getInterface, py::return_value_policy::reference, py::call_guard<py::gil_scoped_release>());

    // class ParticipantProxy and its member functions
    py::class_<ParticipantProxy>(m, "ParticipantProxy")
        .def("setInitPriority", &ParticipantProxy::setInitPriority,
            py::arg("priority"), py::call_guard<py::gil_scoped_release>())
        .def("getInitPriority", &ParticipantProxy::getInitPriority, py::call_guard<py::gil_scoped_release>())
        .def("setStartPriority", &ParticipantProxy::setStartPriority,
            py::arg("priority"), py::call_guard<py::gil_scoped_release>())
        .def("getStartPriority", &ParticipantProxy::getStartPriority, py::call_guard<py::gil_scoped_release>())
        .def("getRPCComponentProxy", [](const ParticipantProxy& self, const std::string& component_name, const std::string& component_iid, IRPCComponentPtr& proxy_ptr)
            {py::call_guard<py::gil_scoped_release>(); return self.getRPCComponentProxy(component_name, component_iid, proxy_ptr);})
        .def("getName", &ParticipantProxy::getName, py::call_guard<py::gil_scoped_release>());

    // substructs of ParticipantHealth
    py::enum_<ParticipantRunningState>(m, "ParticipantRunningState")                                // for running_state in ParticipantHealth
        .value("offline", ParticipantRunningState::offline)
        .value("online", ParticipantRunningState::online);

    py::class_<JobHealthiness::ExecuteResult>(m, "ExecuteResult")                                   // for last_error in ExecuteError
        .def_readwrite("error_code", &JobHealthiness::ExecuteResult::error_code)
        .def_readwrite("error_description", &JobHealthiness::ExecuteResult::error_description)
        .def_readwrite("line", &JobHealthiness::ExecuteResult::line)
        .def_readwrite("file", &JobHealthiness::ExecuteResult::file)
        .def_readwrite("function", &JobHealthiness::ExecuteResult::function);
    py::class_<JobHealthiness::ExecuteError>(m, "ExecuteError")                                     // for execute_errors in JobHealthiness
        .def_readwrite("error_count", &JobHealthiness::ExecuteError::error_count)
        .def_readwrite("simulation_time", &JobHealthiness::ExecuteError::simulation_time)
        .def_readwrite("last_error", &JobHealthiness::ExecuteError::last_error);
    py::class_<JobHealthiness::ClockTriggeredJobInfo>(m, "ClockTriggeredJobInfo")                   // for job_info in JobHealthiness
        .def_readonly("cycle_time", &JobHealthiness::ClockTriggeredJobInfo::cycle_time);
    py::class_<JobHealthiness::DataTriggeredJobInfo>(m, "DataTriggeredJobInfo")                     // for job_info in JobHealthiness
        .def_readonly("trigger_signals", &JobHealthiness::DataTriggeredJobInfo::trigger_signals);
    py::class_<JobHealthiness>(m, "JobHealthiness")                                                 // for jobs_healthiness in ParticipantHealth
        .def_readonly("job_name", &JobHealthiness::job_name)
        .def_readonly("job_info", &JobHealthiness::job_info)
        .def_readwrite("simulation_time", &JobHealthiness::simulation_time)
        .def_readwrite("executeError", &JobHealthiness::execute_error)
        .def_readwrite("executeDataInError", &JobHealthiness::execute_data_in_error)
        .def_readwrite("executeDataOutError", &JobHealthiness::execute_data_out_error);

    // used types of class System
    py::enum_<SystemAggregatedState>(m, "SystemAggregatedState")                    // for argument of setSystemState, setParticipantState
        .value("undefined", SystemAggregatedState::undefined)
        .value("unreachable", SystemAggregatedState::unreachable)
        .value("unloaded", SystemAggregatedState::unloaded)
        .value("loaded", SystemAggregatedState::loaded)
        .value("initialized", SystemAggregatedState::initialized)
        .value("paused", SystemAggregatedState::paused)
        .value("running", SystemAggregatedState::running);
    py::class_<SystemState>(m, "SystemState")                                       // for returnvalue of getSystemState
        .def_readwrite("_state", &SystemState::_state)
        .def_readwrite("_homogeneous", &SystemState::_homogeneous);
    py::class_<ParticipantHealth>(m, "ParticipantHealth")                           // for returnvalue of getParticipantsHealth
        .def_readwrite("running_state", &ParticipantHealth::running_state)
        .def_readonly("jobs_healthiness", &ParticipantHealth::jobs_healthiness);
    py::enum_<LoggerSeverity>(m, "LoggerSeverity")                                  // for function onLog in IEventMonitor
        .value("off", LoggerSeverity::off)
        .value("fatal", LoggerSeverity::fatal)
        .value("error", LoggerSeverity::error)
        .value("warning", LoggerSeverity::warning)
        .value("info", LoggerSeverity::info)
        .value("debug", LoggerSeverity::debug);
    py::class_<IEventMonitor, PyEventMonitor>(m, "IEventMonitor")                   // for register- and unregisterMonitoring
        .def(py::init<>())
        .def("onLog", &IEventMonitor::onLog);
    py::class_<LoggingSinkCsv, IEventMonitor>(m, "LoggingSinkCsv")                  // for system csv logging
        .def(py::init<const std::string &>())
        .def("onLog", &IEventMonitor::onLog);

    // class System and its member functions
    py::class_<System> (m, "System")
    .def("setSystemState", &System::setSystemState,
        py::arg("state"), py::arg("timeout_ms") = FEP_SYSTEM_DISCOVER_TIMEOUT, py::call_guard<py::gil_scoped_release>())
    .def("getSystemState", &System::getSystemState,
        py::arg("timeout_ms") = FEP_SYSTEM_DISCOVER_TIMEOUT, py::call_guard<py::gil_scoped_release>())
    .def("getParticipants", &System::getParticipants, py::call_guard<py::gil_scoped_release>())
    .def("getParticipant", &System::getParticipant,
        py::arg("participant_name"), py::call_guard<py::gil_scoped_release>())
    .def("setParticipantState", &System::setParticipantState,
        py::arg("participant_name"), py::arg("participant_state"), py::call_guard<py::gil_scoped_release>())
    .def("getParticipantState", &System::getParticipantState,
        py::arg("participant_name"), py::call_guard<py::gil_scoped_release>())
    .def("getParticipantStates", &System::getParticipantStates,
        py::arg("timeout_ms") = FEP_SYSTEM_DISCOVER_TIMEOUT, py::call_guard<py::gil_scoped_release>())
    .def("setParticipantProperty", &System::setParticipantProperty,
        py::arg("participant_name"), py::arg("property_path"), py::arg("property_value"), py::call_guard<py::gil_scoped_release>())
    .def("getParticipantsHealth", &System::getParticipantsHealth, py::call_guard<py::gil_scoped_release>())
    .def("setLivelinessTimeout", &System::setLivelinessTimeout,
        py::arg("liveliness_timeout_ns"), py::call_guard<py::gil_scoped_release>())
    .def("getLivelinessTimeout", &System::getLivelinessTimeout)
    .def("setHealthListenerRunningStatus", &System::setHealthListenerRunningStatus,
        py::arg("running"), py::call_guard<py::gil_scoped_release>())
    .def("getHealthListenerRunningStatus", &System::getHealthListenerRunningStatus)
    .def("setHeartbeatInterval", &System::setHeartbeatInterval,
        py::arg("participants"), py::arg("interval_ms"), py::call_guard<py::gil_scoped_release>())
    .def("getHeartbeatInterval", &System::getHeartbeatInterval,
        py::arg("participants"), py::call_guard<py::gil_scoped_release>())
    .def("registerMonitoring", &System::registerMonitoring,
        py::arg("event_listener"), py::call_guard<py::gil_scoped_release>())
    .def("unregisterMonitoring", &System::unregisterMonitoring,
        py::arg("event_listener"), py::call_guard<py::gil_scoped_release>())
    .def("setSeverityLevel", &System::setSeverityLevel,
        py::arg("severity_level"), py::call_guard<py::gil_scoped_release>())
    .def("registerSystemMonitoring", &System::registerSystemMonitoring,
        py::arg("event_listener"), py::call_guard<py::gil_scoped_release>())
    .def("unregisterSystemMonitoring", &System::unregisterSystemMonitoring,
        py::arg("event_listener"), py::call_guard<py::gil_scoped_release>())
    .def("setSystemSeverityLevel", &System::setSystemSeverityLevel,
        py::arg("severity_level"), py::call_guard<py::gil_scoped_release>())
    .def("configureTiming3ClockSyncOnlyInterpolation", &System::configureTiming3ClockSyncOnlyInterpolation,
        py::arg("master_element_id"), py::arg("slave_sync_cycle_time_ms"), py::call_guard<py::gil_scoped_release>())
    .def("configureTiming3DiscreteSteps", &System::configureTiming3DiscreteSteps,
        py::arg("master_element_id"), py::arg("master_time_stepsize_ns"), py::arg("master_time_factor"), py::call_guard<py::gil_scoped_release>())
    .def("configureTiming3NoMaster", &System::configureTiming3NoMaster, py::call_guard<py::gil_scoped_release>())
    .def("getCurrentTimingMasters", &System::getCurrentTimingMasters, py::call_guard<py::gil_scoped_release>())
    .def("getSystemName", &System::getSystemName, py::call_guard<py::gil_scoped_release>());

    // public functions of module fep_system
    m.def("discoverAllSystems",
        py::overload_cast<std::chrono::milliseconds>(&discoverAllSystems),
        py::arg("timeout_ms") = FEP_SYSTEM_DISCOVER_TIMEOUT, py::call_guard<py::gil_scoped_release>());

    m.def("discoverAllSystems",
        py::overload_cast<uint32_t, std::chrono::milliseconds>(&discoverAllSystems),
        py::arg("participant_count"), py::arg("timeout_ms") = FEP_SYSTEM_DISCOVER_TIMEOUT, py::call_guard<py::gil_scoped_release>());

    m.def("discoverSystem",
        py::overload_cast<std::string, std::chrono::milliseconds>(&discoverSystem),
        py::arg("name"), py::arg("timeout_ms") = FEP_SYSTEM_DISCOVER_TIMEOUT, py::call_guard<py::gil_scoped_release>());

    m.def("discoverSystem",
        py::overload_cast<std::string, uint32_t, std::chrono::milliseconds>(&discoverSystem),
        py::arg("name"), py::arg("participant_count"), py::arg("timeout_ms") = FEP_SYSTEM_DISCOVER_TIMEOUT, py::call_guard<py::gil_scoped_release>());

    /*m.def("discoverSystem",
        py::overload_cast<std::string, std::vector<std::string>, std::chrono::milliseconds>(&discoverSystem),
        py::arg("name"), py::arg("participant_names"), py::arg("timeout_ms") = FEP_SYSTEM_DISCOVER_TIMEOUT, py::call_guard<py::gil_scoped_release>());*/

    m.def("systemAggregatedStateToString",
        &systemAggregatedStateToString, py::arg("state"), py::call_guard<py::gil_scoped_release>());
    m.def("getSystemAggregatedStateFromString",
        &getSystemAggregatedStateFromString, py::arg("state_string"), py::call_guard<py::gil_scoped_release>());
    m.def("getString", &arya::getString, py::arg("severity"), py::call_guard<py::gil_scoped_release>());
}
}   // namespace fep3