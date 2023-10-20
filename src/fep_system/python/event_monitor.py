#
# Copyright @ 2021 VW Group. All rights reserved.
#
#     This Source Code Form is subject to the terms of the Mozilla
#     Public License, v. 2.0. If a copy of the MPL was not distributed
#     with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
#
# If it is not possible or desirable to put the notice in a particular file, then
# You may include the notice in a location (such as a LICENSE file in a
# relevant directory) where a recipient would be likely to look for such a notice.
#
# You may add additional accurate notices of copyright ownership.

try:
    import fep3_system
except ImportError:
    from . import fep3_system

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
