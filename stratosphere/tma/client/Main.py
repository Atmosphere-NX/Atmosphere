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
from UsbConnection import UsbConnection
import sys, time
from Packet import Packet
import ServiceId

def main(argc, argv):
    with UsbConnection(None) as c:
        print 'Waiting for connection...'
        c.wait_connected()
        print 'Connected!'
        print 'Reading atmosphere/BCT.ini...'
        c.intf.send_packet(Packet().set_service(ServiceId.TARGETIO_SERVICE).set_task(0x01000000).set_cmd(2).write_str('atmosphere/BCT.ini').write_u64(0x109).write_u64(0))
        resp = c.intf.read_packet()
        res_packet = c.intf.read_packet()
        read_res, size_read = resp.read_u32(), resp.read_u32()
        print 'Final Result: 0x%x' % res_packet.read_u32()
        print 'Size Read: 0x%x' % size_read
        print 'Data:\n%s' % resp.body[resp.offset:]
        
    return 0
    
if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))