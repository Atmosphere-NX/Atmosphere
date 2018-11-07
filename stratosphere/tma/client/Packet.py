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
import zlib
import ServiceId
from struct import unpack as up, pack as pk

HEADER_SIZE = 0x28

def crc32(s):
    return zlib.crc32(s) & 0xFFFFFFFF

class Packet():
    def __init__(self):
        self.service = 0
        self.task = 0
        self.cmd = 0
        self.continuation = 0
        self.version = 0
        self.body_len = 0
        self.body = ''
        self.offset = 0
    def load_header(self, header):
        assert len(header) == HEADER_SIZE
        self.service, self.task, self.cmd, self.continuation, self.version, self.body_len, \
        _, self.body_chk, self.hdr_chk = up('<IIHBBI16sII', header)
        if crc32(header[:-4]) != self.hdr_chk:
            raise ValueError('Invalid header checksum in received packet!')
    def load_body(self, body):
        assert len(body) == self.body_len
        if crc32(body) != self.body_chk:
            raise ValueError('Invalid body checksum in received packet!')
        self.body = body   
    def get_data(self):
        assert len(self.body) == self.body_len and self.body_len <= 0xE000
        self.body_chk = crc32(self.body)
        hdr = pk('<IIHBBIIIIII', self.service, self.task, self.cmd, self.continuation, self.version, self.body_len, 0, 0, 0, 0, self.body_chk)
        self.hdr_chk = crc32(hdr)
        hdr += pk('<I', self.hdr_chk)
        return hdr + self.body
    def set_service(self, srv):
        if type(srv) is str:
            self.service = ServiceId.hash(srv)
        else:
            self.service = srv
        return self
    def set_task(self, t):
        self.task = t
        return self
    def set_cmd(self, x):
        self.cmd = x
        return self
    def set_continuation(self, c):
        self.continuation = c
        return self
    def set_version(self, v):
        self.version = v
        return self
    def reset_offset(self):
        self.offset = 0
        return self
    def write_str(self, s):
        self.body += s
        self.body_len += len(s)
        return self
    def write_u8(self, x):
        self.body += pk('<B', x & 0xFF)
        self.body_len += 1
        return self
    def write_u16(self, x):
        self.body += pk('<H', x & 0xFFFF)
        self.body_len += 2
        return self
    def write_u32(self, x):
        self.body += pk('<I', x & 0xFFFFFFFF)
        self.body_len += 4
        return self
    def write_u64(self, x):
        self.body += pk('<Q', x & 0xFFFFFFFFFFFFFFFF)
        self.body_len += 8
        return self
    def read_str(self):
        s = ''
        while self.body[self.offset] != '\x00' and self.offset < self.body_len:
            s += self.body[self.offset]
            self.offset += 1
    def read_u8(self):
        x, = up('<B', self.body[self.offset:self.offset+1])
        self.offset += 1
        return x
    def read_u16(self):
        x, = up('<H', self.body[self.offset:self.offset+2])
        self.offset += 2
        return x
    def read_u32(self):
        x, = up('<I', self.body[self.offset:self.offset+4])
        self.offset += 4
        return x
    def read_u64(self):
        x, = up('<Q', self.body[self.offset:self.offset+8])
        self.offset += 8
        return x
    def read_struct(self, format, sz):
        x = up(format, self.body[self.offset:self.offset+sz])
        self.offset += sz
        return x
