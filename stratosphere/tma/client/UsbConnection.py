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
from UsbInterface import UsbInterface
from threading import Thread, Condition
from collections import deque
import time
import ServiceId
from Packet import Packet

class UsbConnection(UsbInterface):
    # Auto connect thread func.
    def auto_connect(connection):
        while not connection.is_connected():
            try:
                connection.connect(UsbInterface())
            except ValueError as e:
                continue
    def recv_thread(connection):
        while connection.is_connected():
            try:
                connection.recv_packet()
            except Exception as e:
                print 'An exception occurred:'
                print 'Type: '+e.__class__.__name__
                print 'Msg: '+str(e)
                connection.disconnect()
                connection.send_packet(None)
    def send_thread(connection):
        while connection.is_connected():
            try:
                next_packet = connection.get_next_send_packet()
                if next_packet is not None:
                    connection.intf.send_packet(next_packet)
                else:
                    connection.disconnect()
            except Exception as e:
                print 'An exception occurred:'
                print 'Type: '+e.__class__.__name__
                print 'Msg: '+str(e)
                connection.disconnect()
    def __init__(self, manager):
        self.manager = manager
        self.connected = False
        self.intf = None
        self.conn_lock, self.send_lock = Condition(), Condition()
        self.send_queue = deque()
    def __enter__(self):
        self.conn_thrd = Thread(target=UsbConnection.auto_connect, args=(self,))
        self.conn_thrd.daemon = True
        self.conn_thrd.start()
        return self
    def __exit__(self, type, value, traceback):
        self.disconnect()
        time.sleep(1)
        print 'Closing!'
        time.sleep(1)
    def wait_connected(self):
        self.conn_lock.acquire()
        if not self.is_connected():
            self.conn_lock.wait()
        self.conn_lock.release()
    def is_connected(self):
        return self.connected
    def connect(self, intf):
        # This indicates we have a connection.
        self.conn_lock.acquire()
        assert not self.connected
        self.intf = intf
        
        try:
            # Perform Query + Connection handshake
            print 'Performing handshake...'
            self.intf.send_packet(Packet().set_service(ServiceId.USB_QUERY_TARGET))
            query_resp = self.intf.read_packet()
            print 'Found Switch, Protocol version 0x%x' % query_resp.read_u32()
            
            self.intf.send_packet(Packet().set_service(ServiceId.USB_SEND_HOST_INFO).write_u32(0).write_u32(0))
            
            self.intf.send_packet(Packet().set_service(ServiceId.USB_CONNECT))
            resp = self.intf.read_packet()
            
            # Spawn threads
            self.recv_thrd = Thread(target=UsbConnection.recv_thread, args=(self,))
            self.send_thrd = Thread(target=UsbConnection.send_thread, args=(self,))
            self.recv_thrd.daemon = True
            self.send_thrd.daemon = True
            self.recv_thrd.start()
            self.send_thrd.start()
            self.connected = True
        finally:
            # Finish connection.
            self.conn_lock.notify()
            self.conn_lock.release()
    def disconnect(self):
        self.conn_lock.acquire()
        if self.connected:
            self.connected = False
            self.intf.send_packet(Packet().set_service(ServiceId.USB_DISCONNECT))
        self.conn_lock.release()
    def recv_packet(self):
        packet = self.intf.read_packet()
        assert type(packet) is Packet
        dat = packet.read_u64()
        print('Got Packet: %08x' % dat)
    def send_packet(self, packet):
        assert type(packet) is Packet
        self.send_lock.acquire()
        if len(self.send_queue) == 0x40:
            self.send_lock.wait()
        self.send_queue.append(packet)
        if len(self.send_queue) == 1:
            self.send_lock.notify()
        self.send_lock.release()
    def get_next_send_packet(self):
        self.send_lock.acquire()
        if len(self.send_queue) == 0:
            self.send_lock.wait()
        packet = self.send_queue.popleft()
        if len(self.send_queue) == 0x3F:
            self.send_lock.notify()
        self.send_lock.release()
        return packet

