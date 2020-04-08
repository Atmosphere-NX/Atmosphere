# Copyright 2017 Reswitched Team
#
# Permission to use, copy, modify, and/or distribute this software for any purpose with or
# without fee is hereby granted, provided that the above copyright notice and this permission
# notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
# SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
# THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
# DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
# OR PERFORMANCE OF THIS SOFTWARE.

# nxo64.py: IDA loader (and library for reading nso/nro files)

from __future__ import print_function

import gzip, math, os, re, struct, sys
from struct import unpack as up, pack as pk

from io import BytesIO

import lz4.block

uncompress = lz4.block.decompress

if sys.version_info[0] == 3:
    iter_range = range
    int_types = (int,)
    ascii_string = lambda b: b.decode('ascii')
    bytes_to_list = lambda b: list(b)
    list_to_bytes = lambda l: bytes(l)
else:
    iter_range = xrange
    int_types = (int, long)
    ascii_string = lambda b: str(b)
    bytes_to_list = lambda b: map(ord, b)
    list_to_bytes = lambda l: ''.join(map(chr, l))

def kip1_blz_decompress(compressed):
    compressed_size, init_index, uncompressed_addl_size = struct.unpack('<III', compressed[-0xC:])
    decompressed = compressed[:] + b'\x00' * uncompressed_addl_size
    decompressed_size = len(decompressed)
    if len(compressed) != compressed_size:
        assert len(compressed) > compressed_size
        compressed = compressed[len(compressed) - compressed_size:]
    if not (compressed_size + uncompressed_addl_size):
        return b''
    compressed = bytes_to_list(compressed)
    decompressed = bytes_to_list(decompressed)
    index = compressed_size - init_index
    outindex = decompressed_size
    while outindex > 0:
        index -= 1
        control = compressed[index]
        for i in iter_range(8):
            if control & 0x80:
                if index < 2:
                    raise ValueError('Compression out of bounds!')
                index -= 2
                segmentoffset = compressed[index] | (compressed[index+1] << 8)
                segmentsize = ((segmentoffset >> 12) & 0xF) + 3
                segmentoffset &= 0x0FFF
                segmentoffset += 2
                if outindex < segmentsize:
                    raise ValueError('Compression out of bounds!')
                for j in iter_range(segmentsize):
                    if outindex + segmentoffset >= decompressed_size:
                        raise ValueError('Compression out of bounds!')
                    data = decompressed[outindex+segmentoffset]
                    outindex -= 1
                    decompressed[outindex] = data
            else:
                if outindex < 1:
                    raise ValueError('Compression out of bounds!')
                outindex -= 1
                index -= 1
                decompressed[outindex] = compressed[index]
            control <<= 1
            control &= 0xFF
            if not outindex:
                break
    return list_to_bytes(decompressed)

class BinFile(object):
    def __init__(self, li):
        self._f = li

    def read(self, arg):
        if isinstance(arg, str):
            fmt = '<' + arg
            size = struct.calcsize(fmt)
            raw = self._f.read(size)
            out = struct.unpack(fmt, raw)
            if len(out) == 1:
                return out[0]
            return out
        elif arg is None:
            return self._f.read()
        else:
            out = self._f.read(arg)
            return out

    def read_from(self, arg, offset):
        old = self.tell()
        try:
            self.seek(offset)
            out = self.read(arg)
        finally:
            self.seek(old)
        return out

    def seek(self, off):
        self._f.seek(off)

    def close(self):
        self._f.close()

    def tell(self):
        return self._f.tell()


(DT_NULL, DT_NEEDED, DT_PLTRELSZ, DT_PLTGOT, DT_HASH, DT_STRTAB, DT_SYMTAB, DT_RELA, DT_RELASZ,
 DT_RELAENT, DT_STRSZ, DT_SYMENT, DT_INIT, DT_FINI, DT_SONAME, DT_RPATH, DT_SYMBOLIC, DT_REL,
 DT_RELSZ, DT_RELENT, DT_PLTREL, DT_DEBUG, DT_TEXTREL, DT_JMPREL, DT_BIND_NOW, DT_INIT_ARRAY,
 DT_FINI_ARRAY, DT_INIT_ARRAYSZ, DT_FINI_ARRAYSZ, DT_RUNPATH, DT_FLAGS) = iter_range(31)
DT_GNU_HASH = 0x6ffffef5
DT_VERSYM = 0x6ffffff0
DT_RELACOUNT = 0x6ffffff9
DT_RELCOUNT = 0x6ffffffa
DT_FLAGS_1 = 0x6ffffffb
DT_VERDEF = 0x6ffffffc
DT_VERDEFNUM = 0x6ffffffd

STT_NOTYPE = 0
STT_OBJECT = 1
STT_FUNC = 2
STT_SECTION = 3

STB_LOCAL = 0
STB_GLOBAL = 1
STB_WEAK = 2

R_ARM_ABS32 = 2
R_ARM_TLS_DESC = 13
R_ARM_GLOB_DAT = 21
R_ARM_JUMP_SLOT = 22
R_ARM_RELATIVE = 23

R_AARCH64_ABS64 = 257
R_AARCH64_GLOB_DAT = 1025
R_AARCH64_JUMP_SLOT = 1026
R_AARCH64_RELATIVE = 1027
R_AARCH64_TLSDESC = 1031

MULTIPLE_DTS = set([DT_NEEDED])


class Range(object):
    def __init__(self, start, size):
        self.start = start
        self.size = size
        self.end = start+size
        self._inclend = start+size-1

    def overlaps(self, other):
        return self.start <= other._inclend and other.start <= self._inclend

    def includes(self, other):
        return other.start >= self.start and other._inclend <= self._inclend

    def __repr__(self):
        return 'Range(0x%X -> 0x%X)' % (self.start, self.end)


class Segment(object):
    def __init__(self, r, name, kind):
        self.range = r
        self.name = name
        self.kind = kind
        self.sections = []

    def add_section(self, s):
        for i in self.sections:
            assert not i.range.overlaps(s.range), '%r overlaps %r' % (s, i)
        self.sections.append(s)


class Section(object):
    def __init__(self, r, name):
        self.range = r
        self.name = name

    def __repr__(self):
        return 'Section(%r, %r)' % (self.range, self.name)


def suffixed_name(name, suffix):
    if suffix == 0:
        return name
    return '%s.%d' % (name, suffix)


class SegmentBuilder(object):
    def __init__(self):
        self.segments = []

    def add_segment(self, start, size, name, kind):
        r = Range(start, size)
        for i in self.segments:
            assert not r.overlaps(i.range)
        self.segments.append(Segment(r, name, kind))

    def add_section(self, name, start, end=None, size=None):
        assert end is None or size is None
        if size == 0:
            return
        if size is None:
            size = end-start
        assert size > 0
        r = Range(start, size)
        for i in self.segments:
            if i.range.includes(r):
                i.add_section(Section(r, name))
                return
        assert False, "no containing segment for %r" % (name,)

    def flatten(self):
        self.segments.sort(key=lambda s: s.range.start)
        parts = []
        for segment in self.segments:
            suffix = 0
            segment.sections.sort(key=lambda s: s.range.start)
            pos = segment.range.start
            for section in segment.sections:
                if pos < section.range.start:
                    parts.append((pos, section.range.start, suffixed_name(segment.name, suffix), segment.kind))
                    suffix += 1
                    pos = section.range.start
                parts.append((section.range.start, section.range.end, section.name, segment.kind))
                pos = section.range.end
            if pos < segment.range.end:
                parts.append((pos, segment.range.end, suffixed_name(segment.name, suffix), segment.kind))
                suffix += 1
                pos = segment.range.end
        return parts


class ElfSym(object):
    def __init__(self, name, info, other, shndx, value, size):
        self.name = name
        self.shndx = shndx
        self.value = value
        self.size = size

        self.vis = other & 3
        self.type = info & 0xF
        self.bind = info >> 4

    def __repr__(self):
        return 'Sym(name=%r, shndx=0x%X, value=0x%X, size=0x%X, vis=%r, type=%r, bind=%r)' % (
            self.name, self.shndx, self.value, self.size, self.vis, self.type, self.bind)


class NxoFileBase(object):
    # segment = (content, file offset, vaddr, vsize)
    def __init__(self, text, ro, data, bsssize):
        self.text = text
        self.ro = ro
        self.data = data
        self.bsssize = bsssize
        self.textoff = text[2]
        self.textsize = text[3]
        self.rodataoff = ro[2]
        self.rodatasize = ro[3]
        self.dataoff = data[2]
        flatsize = data[2] + data[3]

        full = text[0]
        if ro[2] >= len(full):
            full += b'\x00' * (ro[2] - len(full))
        else:
            print('truncating .text?')
            full = full[:ro[2]]
        full += ro[0]
        if data[2] > len(full):
            full += b'\x00' * (data[2] - len(full))
        else:
            print('truncating .rodata?')
        full += data[0]
        f = BinFile(BytesIO(full))

        self.binfile = f

        # read MOD
        self.modoff = f.read_from('I', 4)

        f.seek(self.modoff)
        if f.read('4s') != b'MOD0':
            raise NxoException('invalid MOD0 magic')

        self.dynamicoff = self.modoff + f.read('i')
        self.bssoff     = self.modoff + f.read('i')
        self.bssend     = self.modoff + f.read('i')
        self.unwindoff  = self.modoff + f.read('i')
        self.unwindend  = self.modoff + f.read('i')
        self.moduleoff  = self.modoff + f.read('i')


        self.datasize = self.bssoff - self.dataoff
        self.bsssize = self.bssend - self.bssoff

        self.isLibnx = False
        if f.read('4s') == 'LNY0':
            self.isLibnx = True
            self.libnx_got_start    = self.modoff + f.read('i')
            self.libnx_got_end      = self.modoff + f.read('i')

        self.segment_builder = builder = SegmentBuilder()
        for off,sz,name,kind in [
            (self.textoff, self.textsize, ".text", "CODE"),
            (self.rodataoff, self.rodatasize, ".rodata", "CONST"),
            (self.dataoff, self.datasize, ".data", "DATA"),
            (self.bssoff, self.bsssize, ".bss", "BSS"),
        ]:
            builder.add_segment(off, sz, name, kind)

        # read dynamic
        self.armv7 = (f.read_from('Q', self.dynamicoff) > 0xFFFFFFFF or f.read_from('Q', self.dynamicoff+0x10) > 0xFFFFFFFF)
        self.offsize = 4 if self.armv7 else 8

        f.seek(self.dynamicoff)
        self.dynamic = dynamic = {}
        for i in MULTIPLE_DTS:
            dynamic[i] = []
        for i in iter_range((flatsize - self.dynamicoff) // 0x10):
            tag, val = f.read('II' if self.armv7 else 'QQ')
            if tag == DT_NULL:
                break
            if tag in MULTIPLE_DTS:
                dynamic[tag].append(val)
            else:
                dynamic[tag] = val
        builder.add_section('.dynamic', self.dynamicoff, end=f.tell())

        # read .dynstr
        if DT_STRTAB in dynamic and DT_STRSZ in dynamic:
            f.seek(dynamic[DT_STRTAB])
            self.dynstr = f.read(dynamic[DT_STRSZ])
        else:
            self.dynstr = b'\x00'
            print('warning: no dynstr')

        for startkey, szkey, name in [
            (DT_STRTAB, DT_STRSZ, '.dynstr'),
            (DT_INIT_ARRAY, DT_INIT_ARRAYSZ, '.init_array'),
            (DT_FINI_ARRAY, DT_FINI_ARRAYSZ, '.fini_array'),
            (DT_RELA, DT_RELASZ, '.rela.dyn'),
            (DT_REL, DT_RELSZ, '.rel.dyn'),
            (DT_JMPREL, DT_PLTRELSZ, ('.rel.plt' if self.armv7 else '.rela.plt')),
        ]:
            if startkey in dynamic and szkey in dynamic:
                builder.add_section(name, dynamic[startkey], size=dynamic[szkey])

        self.needed = [self.get_dynstr(i) for i in self.dynamic[DT_NEEDED]]

        # load .dynsym
        self.symbols = symbols = []
        if DT_SYMTAB in dynamic and DT_STRTAB in dynamic:
            f.seek(dynamic[DT_SYMTAB])
            while True:
                if dynamic[DT_SYMTAB] < dynamic[DT_STRTAB] and f.tell() >= dynamic[DT_STRTAB]:
                    break
                if self.armv7:
                    st_name, st_value, st_size, st_info, st_other, st_shndx = f.read('IIIBBH')
                else:
                    st_name, st_info, st_other, st_shndx, st_value, st_size = f.read('IBBHQQ')
                if st_name > len(self.dynstr):
                    break
                symbols.append(ElfSym(self.get_dynstr(st_name), st_info, st_other, st_shndx, st_value, st_size))
            builder.add_section('.dynsym', dynamic[DT_SYMTAB], end=f.tell())

        self.plt_entries = []
        self.relocations = []
        locations = set()
        plt_got_end = None
        if DT_REL in dynamic and DT_RELSZ in dynamic:
            locations |= self.process_relocations(f, symbols, dynamic[DT_REL], dynamic[DT_RELSZ])

        if DT_RELA in dynamic and DT_RELASZ in dynamic:
            locations |= self.process_relocations(f, symbols, dynamic[DT_RELA], dynamic[DT_RELASZ])

        if DT_JMPREL in dynamic and DT_PLTRELSZ in dynamic:
            pltlocations = self.process_relocations(f, symbols, dynamic[DT_JMPREL], dynamic[DT_PLTRELSZ])
            locations |= pltlocations

            plt_got_start = min(pltlocations)
            plt_got_end = max(pltlocations) + self.offsize
            if DT_PLTGOT in dynamic:
                builder.add_section('.got.plt', dynamic[DT_PLTGOT], end=plt_got_end)

            if not self.armv7:
                f.seek(0)
                text = f.read(self.textsize)
                last = 12
                while True:
                    pos = text.find(struct.pack('<I', 0xD61F0220), last)
                    if pos == -1: break
                    last = pos+1
                    if (pos % 4) != 0: continue
                    off = pos - 12
                    a, b, c, d = struct.unpack_from('<IIII', text, off)
                    if d == 0xD61F0220 and (a & 0x9f00001f) == 0x90000010 and (b & 0xffe003ff) == 0xf9400211:
                        base = off & ~0xFFF
                        immhi = (a >> 5) & 0x7ffff
                        immlo = (a >> 29) & 3
                        paddr = base + ((immlo << 12) | (immhi << 14))
                        poff = ((b >> 10) & 0xfff) << 3
                        target = paddr + poff
                        if plt_got_start <= target < plt_got_end:
                            self.plt_entries.append((off, target))
                builder.add_section('.plt', min(self.plt_entries)[0], end=max(self.plt_entries)[0] + 0x10)

            # try to find the ".got" which should follow the ".got.plt"
            if not self.isLibnx:
                if plt_got_end is not None:
                    good = False
                    got_end = plt_got_end + self.offsize
                    while got_end in locations and (DT_INIT_ARRAY not in dynamic or got_end < dynamic[DT_INIT_ARRAY]):
                        good = True
                        got_end += self.offsize

                    if good:
                        builder.add_section('.got', plt_got_end, end=got_end)
        if self.isLibnx:
            builder.add_section('.got', self.libnx_got_start, end=self.libnx_got_end)

        self.sections = []
        for start, end, name, kind in builder.flatten():
            self.sections.append((start, end, name, kind))


    def process_relocations(self, f, symbols, offset, size):
        locations = set()
        f.seek(offset)
        relocsize = 8 if self.armv7 else 0x18
        for i in iter_range(size // relocsize):
            # NOTE: currently assumes all armv7 relocs have no addends,
            # and all 64-bit ones do.
            if self.armv7:
                offset, info = f.read('II')
                addend = None
                r_type = info & 0xff
                r_sym = info >> 8
            else:
                offset, info, addend = f.read('QQq')
                r_type = info & 0xffffffff
                r_sym = info >> 32

            sym = symbols[r_sym] if r_sym != 0 else None

            if r_type != R_AARCH64_TLSDESC and r_type != R_ARM_TLS_DESC:
                locations.add(offset)
            self.relocations.append((offset, r_type, sym, addend))
        return locations

    def get_dynstr(self, o):
        return ascii_string(self.dynstr[o:self.dynstr.index(b'\x00', o)])


class NsoFile(NxoFileBase):
    def __init__(self, fileobj):
        f = BinFile(fileobj)

        if f.read_from('4s', 0) != b'NSO0':
            raise NxoException('Invalid NSO magic')

        flags = f.read_from('I', 0xC)

        toff, tloc, tsize = f.read_from('III', 0x10)
        roff, rloc, rsize = f.read_from('III', 0x20)
        doff, dloc, dsize = f.read_from('III', 0x30)

        tfilesize, rfilesize, dfilesize = f.read_from('III', 0x60)
        bsssize = f.read_from('I', 0x3C)

        #print('load text: ')
        text = (uncompress(f.read_from(tfilesize, toff), uncompressed_size=tsize), None, tloc, tsize) if flags & 1 else (f.read_from(tfilesize, toff), toff, tloc, tsize)
        ro   = (uncompress(f.read_from(rfilesize, roff), uncompressed_size=rsize), None, rloc, rsize) if flags & 2 else (f.read_from(rfilesize, roff), roff, rloc, rsize)
        data = (uncompress(f.read_from(dfilesize, doff), uncompressed_size=dsize), None, dloc, dsize) if flags & 4 else (f.read_from(dfilesize, doff), doff, dloc, dsize)

        super(NsoFile, self).__init__(text, ro, data, bsssize)


class NroFile(NxoFileBase):
    def __init__(self, fileobj):
        f = BinFile(fileobj)

        if f.read_from('4s', 0x10) != b'NRO0':
            raise NxoException('Invalid NRO magic')

        f.seek(0x20)

        tloc, tsize = f.read('II')
        rloc, rsize = f.read('II')
        dloc, dsize = f.read('II')
        bsssize = f.read_from('I', 0x28)

        text = (f.read_from(tsize, tloc), tloc, tloc, tsize)
        ro   = (f.read_from(rsize, rloc), rloc, rloc, rsize)
        data = (f.read_from(dsize, dloc), dloc, dloc, dsize)

        super(NroFile, self).__init__(text, ro, data, bsssize)

class KipFile(NxoFileBase):
    def __init__(self, fileobj):
        f = BinFile(fileobj)

        if f.read_from('4s', 0) != b'KIP1':
            raise NxoException('Invalid KIP magic')

        flags = f.read_from('b', 0x1F)

        tloc, tsize, tfilesize = f.read_from('III', 0x20)
        rloc, rsize, rfilesize = f.read_from('III', 0x30)
        dloc, dsize, dfilesize = f.read_from('III', 0x40)

        toff = 0x100
        roff = toff + tfilesize
        doff = roff + rfilesize

        bsssize = f.read_from('I', 0x54)
        print('bss size 0x%x' % bsssize)

        print('load segments')
        text = (kip1_blz_decompress(f.read_from(tfilesize, toff)), None, tloc, tsize) if flags & 1 else (f.read_from(tfilesize, toff), toff, tloc, tsize)
        ro   = (kip1_blz_decompress(f.read_from(rfilesize, roff)), None, rloc, rsize) if flags & 2 else (f.read_from(rfilesize, roff), roff, rloc, rsize)
        data = (kip1_blz_decompress(f.read_from(dfilesize, doff)), None, dloc, dsize) if flags & 4 else (f.read_from(dfilesize, doff), doff, dloc, dsize)

        super(KipFile, self).__init__(text, ro, data, bsssize)


class NxoException(Exception):
    pass


def load_nxo(fileobj):
    fileobj.seek(0)
    header = fileobj.read(0x14)

    if header[:4] == b'NSO0':
        return NsoFile(fileobj)
    elif header[0x10:0x14] == b'NRO0':
        return NroFile(fileobj)
    elif header[:4] == b'KIP1':
        return KipFile(fileobj)
    else:
        raise NxoException("not an NRO or NSO or KIP file")


try:
    import idaapi
    import idc
except ImportError:
    pass
else:
    # IDA specific code
    def accept_file(li, n):
        print('accept_file')
        if not isinstance(n, int_types) or n == 0:
            li.seek(0)
            if li.read(4) == b'NSO0':
                return 'nxo.py: Switch binary (NSO)'
            li.seek(0)
            if li.read(4) == b'KIP1':
                return 'nxo.py: Switch binary (KIP)'
            li.seek(0x10)
            if li.read(4) == b'NRO0':
                return 'nxo.py: Switch binary (NRO)'
        return 0

    def ida_make_offset(f, ea):
        if f.armv7:
            idaapi.create_data(ea, idc.FF_DWORD, 4, idaapi.BADADDR)
        else:
            idaapi.create_data(ea, idc.FF_QWORD, 8, idaapi.BADADDR)
        idc.op_plain_offset(ea, 0, 0)

    def find_bl_targets(text_start, text_end):
        targets = set()
        for pc in range(text_start, text_end, 4):
            d = idc.get_wide_dword(pc)
            if (d & 0xfc000000) == 0x94000000:
                imm = d & 0x3ffffff
                if imm & 0x2000000:
                    imm |= ~0x1ffffff
                if 0 <= imm <= 2:
                    continue
                target = pc + imm * 4
                if target >= text_start and target < text_end:
                    targets.add(target)
        return targets

    def load_file(li, neflags, format):
        idaapi.set_processor_type("arm", idaapi.SETPROC_LOADER_NON_FATAL|idaapi.SETPROC_LOADER)
        f = load_nxo(li)
        if f.armv7:
            idc.set_inf_attr(idc.INF_LFLAGS, idc.get_inf_attr(idc.INF_LFLAGS) | idc.LFLG_PC_FLAT)
        else:
            idc.set_inf_attr(idc.INF_LFLAGS, idc.get_inf_attr(idc.INF_LFLAGS) | idc.LFLG_64BIT)

        idc.set_inf_attr(idc.INF_DEMNAMES, idaapi.DEMNAM_GCC3)
        idaapi.set_compiler_id(idaapi.COMP_GNU)
        idaapi.add_til('gnulnx_arm' if f.armv7 else 'gnulnx_arm64', 1)

        loadbase = 0x60000000 if f.armv7 else 0x7100000000

        f.binfile.seek(0)
        as_string = f.binfile.read(f.bssoff)
        idaapi.mem2base(as_string, loadbase)
        if f.text[1] != None:
            li.file2base(f.text[1], loadbase + f.text[2], loadbase + f.text[2] + f.text[3], True)
        if f.ro[1] != None:
            li.file2base(f.ro[1], loadbase + f.ro[2], loadbase + f.ro[2] + f.ro[3], True)
        if f.data[1] != None:
            li.file2base(f.data[1], loadbase + f.data[2], loadbase + f.data[2] + f.data[3], True)

        for start, end, name, kind in f.sections:
            if name.startswith('.got'):
                kind = 'CONST'
            idaapi.add_segm(0, loadbase+start, loadbase+end, name, kind)
            segm = idaapi.get_segm_by_name(name)
            if kind == 'CONST':
                segm.perm = idaapi.SEGPERM_READ
            elif kind == 'CODE':
                segm.perm = idaapi.SEGPERM_READ | idaapi.SEGPERM_EXEC
            elif kind == 'DATA':
                segm.perm = idaapi.SEGPERM_READ | idaapi.SEGPERM_WRITE
            elif kind == 'BSS':
                segm.perm = idaapi.SEGPERM_READ | idaapi.SEGPERM_WRITE
            idaapi.update_segm(segm)
            idaapi.set_segm_addressing(segm, 1 if f.armv7 else 2)

        # do imports
        # TODO: can we make imports show up in "Imports" window?
        undef_count = 0
        for s in f.symbols:
            if not s.shndx and s.name:
                undef_count += 1
        last_ea = max(loadbase + end for start, end, name, kind in f.sections)
        undef_entry_size = 8
        undef_ea = ((last_ea + 0xFFF) & ~0xFFF) + undef_entry_size # plus 8 so we don't end up on the "end" symbol
        idaapi.add_segm(0, undef_ea, undef_ea+undef_count*undef_entry_size, "UNDEF", "XTRN")
        segm = idaapi.get_segm_by_name("UNDEF")
        segm.type = idaapi.SEG_XTRN
        idaapi.update_segm(segm)
        for i,s in enumerate(f.symbols):
            if not s.shndx and s.name:
                idaapi.create_data(undef_ea, idc.FF_QWORD, 8, idaapi.BADADDR)
                idaapi.force_name(undef_ea, s.name)
                s.resolved = undef_ea
                undef_ea += undef_entry_size
            elif i != 0:
                assert s.shndx
                s.resolved = loadbase + s.value
                if s.name:
                    if s.type == STT_FUNC:
                        print(hex(s.resolved), s.name)
                        idaapi.add_entry(s.resolved, s.resolved, s.name, 0)
                    else:
                        idaapi.force_name(s.resolved, s.name)

            else:
                # NULL symbol
                s.resolved = 0

        funcs = set()
        for s in f.symbols:
            if s.name and s.shndx and s.value:
                if s.type == STT_FUNC:
                    funcs.add(loadbase+s.value)

        got_name_lookup = {}
        for offset, r_type, sym, addend in f.relocations:
            target = offset + loadbase
            if r_type in (R_ARM_GLOB_DAT, R_ARM_JUMP_SLOT, R_ARM_ABS32):
                if not sym:
                    print('error: relocation at %X failed' % target)
                else:
                    idaapi.put_dword(target, sym.resolved)
            elif r_type == R_ARM_RELATIVE:
                idaapi.put_dword(target, idaapi.get_dword(target) + loadbase)
            elif r_type in (R_AARCH64_GLOB_DAT, R_AARCH64_JUMP_SLOT, R_AARCH64_ABS64):
                idaapi.put_qword(target, sym.resolved + addend)
                if addend == 0:
                    got_name_lookup[offset] = sym.name
            elif r_type == R_AARCH64_RELATIVE:
                idaapi.put_qword(target, loadbase + addend)
                if addend < f.textsize:
                    funcs.add(loadbase + addend)
            else:
                print('TODO r_type %d' % (r_type,))
            ida_make_offset(f, target)

        for func, target in f.plt_entries:
            if target in got_name_lookup:
                addr = loadbase + func
                funcs.add(addr)
                idaapi.force_name(addr, got_name_lookup[target])

        funcs |= find_bl_targets(loadbase, loadbase+f.textsize)

        for addr in sorted(funcs, reverse=True):
            idc.AutoMark(addr, idc.AU_CODE)
            idc.AutoMark(addr, idc.AU_PROC)

        return 1
