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
import usb
import Packet

class UsbInterface():
    def __init__(self):
        self.dev = usb.core.find(idVendor=0x057e, idProduct=0x3000)
        if self.dev is None:
            raise ValueError('Device not found')

        self.dev.set_configuration()
        self.cfg = self.dev.get_active_configuration()
        self.intf = usb.util.find_descriptor(self.cfg, bInterfaceClass=0xff, bInterfaceSubClass=0xff, bInterfaceProtocol=0xfc)
        assert self.intf is not None
        
        self.ep_in = usb.util.find_descriptor(
            self.intf,
            custom_match = \
            lambda e: \
                usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_IN)
        assert self.ep_in is not None

        self.ep_out = usb.util.find_descriptor(
            self.intf,
            custom_match = \
            lambda e: \
                usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_OUT)
        assert self.ep_out is not None
    def close(self):
        usb.util.dispose_resources(self.dev)
    def blocking_read(self, size):
        return ''.join(chr(x) for x in self.ep_in.read(size, 0xFFFFFFFFFFFFFFFF))
    def blocking_write(self, data):
        self.ep_out.write(data, 0xFFFFFFFFFFFFFFFF)
    def read_packet(self):
        packet = Packet.Packet()
        hdr = self.blocking_read(Packet.HEADER_SIZE)
        packet.load_header(hdr)
        if packet.body_len:
            packet.load_body(self.blocking_read(packet.body_len))
        return packet
    def send_packet(self, packet):
        data = packet.get_data()
        self.blocking_write(data[:Packet.HEADER_SIZE])
        if (len(data) > Packet.HEADER_SIZE):
            self.blocking_write(data[Packet.HEADER_SIZE:])


