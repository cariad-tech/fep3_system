# Copyright 2023 CARIAD SE. 
#
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

import time, datetime
import fep3_system, event_monitor, collections
from threading import Event
from pathlib import Path

class TestMonitor(event_monitor.EventMonitor):
    def __init__(self, system, notification):
        event_monitor.EventMonitor.__init__(self, system)
        # implementation for this application
        self._notification = notification
        self._messages = collections.defaultdict(list)
    def onLog(self, log_time, severity_level, participant_name, logger_name, message):
        self._notification.set()
        self._messages[severity_level].append(str(log_time) + ": " + logger_name + " --- " + participant_name + " --- " + message)
    def getMsg(self):
        return self._messages

def test_system_monitoring():
    systems = fep3_system.discoverAllSystems(2, datetime.timedelta(seconds=60))
    notify = Event()
    monitor = TestMonitor(systems[0], notify)
    assert len(monitor.getMsg()) == 0           # expect an empty dictionary
    assert len(systems) == 1
    assert systems[0].getSystemName().startswith("system_under_test") == True

    systems[0].setSystemState(fep3_system.getSystemAggregatedStateFromString('loaded'))
    state = systems[0].getSystemState()
    assert fep3_system.systemAggregatedStateToString(state._state) == 'loaded'
    assert state._homogeneous == True

    systems[0].setSystemState(fep3_system.getSystemAggregatedStateFromString('initialized'))
    state = systems[0].getSystemState()
    assert fep3_system.systemAggregatedStateToString(state._state) == 'initialized'
    assert state._homogeneous == True

    states = systems[0].getParticipantStates()
    assert len(states) == 2

    notify.wait(10.0)                           # wait to collect log messages

    assert len(monitor.getMsg()) > 0            # expect entries in at least one key
    del monitor

def test_system_csv_logging():
    systems = fep3_system.discoverAllSystems(2, datetime.timedelta(seconds=60))
    assert len(systems) == 1
    assert systems[0].getSystemName().startswith("system_under_test") == True

    log_file_path = Path(__file__).parent / "csv_log_file.txt"
    csv_monitor = fep3_system.LoggingSinkCsv(log_file_path.absolute().as_posix())
    systems[0].registerSystemMonitoring(csv_monitor)
    systems[0].setSystemSeverityLevel(fep3_system.LoggerSeverity.debug)

    systems[0].setSystemState(fep3_system.getSystemAggregatedStateFromString('loaded'))
    state = systems[0].getSystemState()
    assert fep3_system.systemAggregatedStateToString(state._state) == 'loaded'
    assert state._homogeneous == True

    systems[0].setSystemState(fep3_system.getSystemAggregatedStateFromString('unloaded'))
    state = systems[0].getSystemState()
    assert fep3_system.systemAggregatedStateToString(state._state) == 'unloaded'
    assert state._homogeneous == True

    systems[0].unregisterSystemMonitoring(csv_monitor)

    assert log_file_path.is_file() == True