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

class UsbConnection(UsbInterface):
    # Auto connect thread func.
    def auto_connect(connection):
        while not connection.is_connected():
            try:
                connection.connect(UsbInterface())
            except ValueError as e:
                continue
    def recv_thread(connection):
        if connection.is_connected():
            try:
                # If we've previously been connected, PyUSB will read garbage...
                connection.recv_packet()
            except ValueError:
                pass
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
        self.connected = True
        self.conn_lock.notify()
        self.conn_lock.release()
        self.recv_thrd = Thread(target=UsbConnection.recv_thread, args=(self,))
        self.send_thrd = Thread(target=UsbConnection.send_thread, args=(self,))
        self.recv_thrd.daemon = True
        self.send_thrd.daemon = True
        self.recv_thrd.start()
        self.send_thrd.start()
    def disconnect(self):
        self.conn_lock.acquire()
        if self.connected:
            self.connected = False
        self.conn_lock.release()
    def recv_packet(self):
        hdr, body = self.intf.read_packet()
        print('Got Packet: %s' % body.encode('hex'))
    def send_packet(self, packet):
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

