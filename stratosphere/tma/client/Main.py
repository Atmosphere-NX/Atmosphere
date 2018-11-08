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

def main(argc, argv):
    with UsbConnection(None) as c:
        print 'Waiting for connection...'
        c.wait_connected()
        print 'Connected!'
        while True:
            c.send_packet('AAAAAAAA')
    return 0
    
if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))