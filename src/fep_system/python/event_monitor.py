# Copyright 2023 CARIAD SE. 
#
# This Source Code Form is subject to the terms of the Mozilla
# Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at https://mozilla.org/MPL/2.0/.

try:
    # used if installed in site_packages
    from . import fep3_system 
except ImportError:
    # used from conan package, e.g. folder [conan_package]/lib is in the python path
    import fep3_system

# class EventMonitor to use for further inheritance
# to add an own function 'onLog' for using Event Monitor
#    def onLog(self, log_time, severity_level, participant_name, logger_name, message):
class EventMonitor(fep3_system.IEventMonitor):
    def __init__(self, system):
        fep3_system.IEventMonitor.__init__(self)
        # register monitoring here to guarantee unregistering in destructor
        system.registerMonitoring(self)
        self._system = system
    def __del__(self):
        self._system.unregisterMonitoring(self)
