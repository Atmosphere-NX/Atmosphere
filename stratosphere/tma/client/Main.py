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
        c.intf.send_packet(Packet().set_service(ServiceId.SETTINGS_SERVICE).set_task(0x01000000).set_cmd(0).write_str('lm').write_str('sd_card_log_output_directory'))
        resp = c.intf.read_packet()
        success = resp.read_u8() != 0
        print 'Succeeded: %s' % str(success)
        if success:
            size = resp.read_u32()
            value = resp.body[resp.offset:resp.offset+size]
            print 'Value Size: 0x%x' % size
            print 'Value: %s' % repr(value)
            
    return 0
    
if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))