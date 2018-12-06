# Copyright (c) 2018 Atmosphere-NX
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

def hash(s):
    h = ord(s[0]) & 0xFFFFFFFF
    for c in s:
        h = ((1000003 * h) ^ ord(c)) & 0xFFFFFFFF
    h ^= len(s)
    return h
    
USB_QUERY_TARGET = hash("USBQueryTarget")
USB_SEND_HOST_INFO = hash("USBSendHostInfo")
USB_CONNECT = hash("USBConnect")
USB_DISCONNECT = hash("USBDisconnect")

ATMOSPHERE_TEST_SERVICE = hash("AtmosphereTestService")
SETTINGS_SERVICE = hash("SettingsService")
TARGETIO_SERVICE = hash("TIOService")
    