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

# nxo64.py: IDA loader and library for reading nso/nro files

import gzip, math, os, re, struct, sys

from io import BytesIO
from cStringIO import StringIO

import lz4.block

uncompress = lz4.block.decompress


def get_file_size(f):
    filesize = 0
    try:
        filesize = f.size()
    except:
        ptell = f.tell()
        f.seek(0, 2)
        filesize = f.tell()
        f.seek(ptell)
    return filesize

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
            return self.read_to_end()
        else:
            out = self._f.read(arg)
            if isinstance(arg, (int,long)) and len(out) != arg:
                pass #print 'warning: read of %d bytes got %d bytes' % (arg, len(out))
            return out

    def read_to_end(self):
        return self.read(self.size()-self.tell())

    def size(self):
        return get_file_size(self._f)

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

    def skip(self, dist):
        self.seek(self.tell()+dist)

    def close(self):
        self._f.close()

    def tell(self):
        return self._f.tell()


(DT_NULL, DT_NEEDED, DT_PLTRELSZ, DT_PLTGOT, DT_HASH, DT_STRTAB, DT_SYMTAB, DT_RELA, DT_RELASZ,
 DT_RELAENT, DT_STRSZ, DT_SYMENT, DT_INIT, DT_FINI, DT_SONAME, DT_RPATH, DT_SYMBOLIC, DT_REL,
 DT_RELSZ, DT_RELENT, DT_PLTREL, DT_DEBUG, DT_TEXTREL, DT_JMPREL, DT_BIND_NOW, DT_INIT_ARRAY,
 DT_FINI_ARRAY, DT_INIT_ARRAYSZ, DT_FINI_ARRAYSZ, DT_RUNPATH, DT_FLAGS) = xrange(31)

DT_RELRSZ, DT_RELR, DT_RELRENT = 0x23, 0x24, 0x25

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

R_FAKE_RELR = -1

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
        self._sections = []

    def add_segment(self, start, size, name, kind):
        r = Range(start, size)
        for i in self.segments:
            assert not r.overlaps(i.range)
        self.segments.append(Segment(r, name, kind))

    def add_section(self, name, start, end=None, size=None):
        assert end is None or size is None
        if size is None:
            size = end-start
        if size > 0:
            #assert size > 0
            r = Range(start, size)
            self._sections.append((r, name))

    def _add_sections_to_segments(self):
        for r, name in self._sections:
            for i in self.segments:
                if i.range.includes(r):
                    i.add_section(Section(r, name))
                    break
            else:
                assert False, 'no containing segment for %r' % (name,)

    def flatten(self):
        self._add_sections_to_segments()
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
    def __init__(self, f, segment_data=None):
        self.binfile = f

        # read MOD
        self.modoff = f.read_from('I', 4)

        f.seek(self.modoff)
        if f.read('4s') != 'MOD0':
            raise NxoException('invalid MOD0 magic')

        self.dynamicoff = self.modoff + f.read('i')
        self.bssoff     = self.modoff + f.read('i')
        self.bssend     = self.modoff + f.read('i')
        self.unwindoff  = self.modoff + f.read('i')
        self.unwindend  = self.modoff + f.read('i')
        self.moduleoff  = self.modoff + f.read('i')


        builder = SegmentBuilder()

        # read dynamic
        self.armv7 = (f.read_from('Q', self.dynamicoff) > 0xFFFFFFFF or f.read_from('Q', self.dynamicoff+0x10) > 0xFFFFFFFF)
        self.offsize = 4 if self.armv7 else 8

        f.seek(self.dynamicoff)
        self.dynamic = dynamic = {}
        for i in MULTIPLE_DTS:
            dynamic[i] = []
        for i in xrange((f.size() - self.dynamicoff) / 0x10):
            tag, val = f.read('II' if self.armv7 else 'QQ')
            if tag == DT_NULL:
                break
            if tag in MULTIPLE_DTS:
                dynamic[tag].append(val)
            else:
                dynamic[tag] = val
        self.dynamicsize = f.tell() - self.dynamicoff
        builder.add_section('.dynamic', self.dynamicoff, end=self.dynamicoff + self.dynamicsize)
        builder.add_section('.eh_frame_hdr', self.unwindoff, end=self.unwindend)

        # read .dynstr
        if DT_STRTAB in dynamic and DT_STRSZ in dynamic:
            f.seek(dynamic[DT_STRTAB])
            self.dynstr = f.read(dynamic[DT_STRSZ])
        else:
            self.dynstr = '\0'
            print 'warning: no dynstr'

        for startkey, szkey, name in [
            (DT_STRTAB, DT_STRSZ, '.dynstr'),
            (DT_INIT_ARRAY, DT_INIT_ARRAYSZ, '.init_array'),
            (DT_FINI_ARRAY, DT_FINI_ARRAYSZ, '.fini_array'),
            (DT_RELA, DT_RELASZ, '.rela.dyn'),
            (DT_REL, DT_RELSZ, '.rel.dyn'),
            (DT_RELR, DT_RELRSZ, '.relr.dyn'),
            (DT_JMPREL, DT_PLTRELSZ, ('.rel.plt' if self.armv7 else '.rela.plt')),
        ]:
            if startkey in dynamic and szkey in dynamic:
                builder.add_section(name, dynamic[startkey], size=dynamic[szkey])

        # TODO
        #build_id = content.find('\x04\x00\x00\x00\x14\x00\x00\x00\x03\x00\x00\x00GNU\x00')
        #if build_id >= 0:
        #    builder.add_section('.note.gnu.build-id', build_id, size=0x24)
        #else:
        #    build_id = content.index('\x04\x00\x00\x00\x10\x00\x00\x00\x03\x00\x00\x00GNU\x00')
        #    if build_id >= 0:
        #        builder.add_section('.note.gnu.build-id', build_id, size=0x20)

        if DT_HASH in dynamic:
            hash_start = dynamic[DT_HASH]
            f.seek(hash_start)
            nbucket, nchain = f.read('II')
            f.skip(nbucket * 4)
            f.skip(nchain * 4)
            hash_end = f.tell()
            builder.add_section('.hash', hash_start, end=hash_end)

        if DT_GNU_HASH in dynamic:
            gnuhash_start = dynamic[DT_GNU_HASH]
            f.seek(gnuhash_start)
            nbuckets, symoffset, bloom_size, bloom_shift = f.read('IIII')
            f.skip(bloom_size * self.offsize)
            buckets = [f.read('I') for i in range(nbuckets)]

            max_symix = max(buckets) if buckets else 0
            if max_symix >= symoffset:
                f.skip((max_symix - symoffset) * 4)
                while (f.read('I') & 1) == 0:
                    pass
            gnuhash_end = f.tell()
            builder.add_section('.gnu.hash', gnuhash_start, end=gnuhash_end)

        self.needed = [self.get_dynstr(i) for i in self.dynamic[DT_NEEDED]]

        # load .dynsym
        self.symbols = symbols = []
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
        if DT_REL in dynamic:
            locations |= self.process_relocations(f, symbols, dynamic[DT_REL], dynamic[DT_RELSZ])

        if DT_RELA in dynamic:
            locations |= self.process_relocations(f, symbols, dynamic[DT_RELA], dynamic[DT_RELASZ])

        if DT_RELR in dynamic:
            locations |= self.process_relocations_relr(f, dynamic[DT_RELR], dynamic[DT_RELRSZ])

        if segment_data is None:
            # infer segment info
            rloc_guess = (dynamic[DT_REL if DT_REL in dynamic else DT_RELA] & ~0xFFF)
            dloc_guess = (min(i for i in locations if i != 0) & ~0xFFF)
            dloc_guess2 = None
            modoff = f.read_from('I', 4)
            if self.modoff != 8:
                search_start = (self.modoff + 0xFFF) & ~0xFFF
                for i in range(search_start, f.size(), 0x1000):
                    count = 0
                    for j in range(4, 0x1000, 4):
                        if f.read_from('I', i - j) != 0:
                            break
                        count += 1
                    if count > 6:
                        dloc_guess2 = i
                        break
            if dloc_guess2 is not None and dloc_guess2 < dloc_guess:
                dloc_guess = dloc_guess2

            if segment_data:
                tloc, tsize, rloc, rsize, dloc, dsize = segment_data
                assert rloc_guess == rloc
                assert dloc_guess == dloc

            self.textoff = 0
            self.textsize = rloc_guess
            self.rodataoff = rloc_guess
            self.rodatasize = dloc_guess - rloc_guess
            self.dataoff = dloc_guess
        else:
            tloc, tsize, rloc, rsize, dloc, dsize = segment_data
            self.textoff = tloc
            self.textsize = tsize
            self.rodataoff = rloc
            self.rodatasize = rsize
            self.dataoff = dloc

        self.datasize = self.bssoff - self.dataoff
        self.bsssize = self.bssend - self.bssoff

        plt_got_end = None
        if DT_JMPREL in dynamic:
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
                if len(self.plt_entries) > 0:
                    builder.add_section('.plt', min(self.plt_entries)[0], end=max(self.plt_entries)[0] + 0x10)

        # try to find the ".got" which should follow the ".got.plt"
        good = False
        got_start = (plt_got_end if plt_got_end is not None else self.dynamicoff + self.dynamicsize)
        got_end = self.offsize + got_start
        while (got_end in locations or (plt_got_end is None and got_end < dynamic[DT_INIT_ARRAY])) and (DT_INIT_ARRAY not in dynamic or got_end < dynamic[DT_INIT_ARRAY] or dynamic[DT_INIT_ARRAY] < got_start):
            good = True
            got_end += self.offsize
            #print 'got_start got_end %X %X %s %X' % (got_start, got_end, str(got_end in locations), dynamic[DT_INIT_ARRAY])

        if good:
            self.got_start = got_start
            self.got_end   = got_end
            builder.add_section('.got', self.got_start, end=self.got_end)

        self.eh_table = []
        if not self.armv7:
            f.seek(self.unwindoff)
            version, eh_frame_ptr_enc, fde_count_enc, table_enc = f.read('BBBB')
            if not any(i == 0xff for i in (eh_frame_ptr_enc, fde_count_enc, table_enc)): # DW_EH_PE_omit
                #assert eh_frame_ptr_enc == 0x1B # DW_EH_PE_pcrel | DW_EH_PE_sdata4
                #assert fde_count_enc == 0x03    # DW_EH_PE_absptr | DW_EH_PE_udata4
                #assert table_enc == 0x3B        # DW_EH_PE_datarel | DW_EH_PE_sdata4
                if eh_frame_ptr_enc == 0x1B and fde_count_enc == 0x03 and table_enc == 0x3B:
                    base_offset = f.tell()
                    eh_frame = base_offset + f.read('i')

                    fde_count = f.read('I')
                    #assert 8 * fde_count == self.unwindend - f.tell()
                    if 8 * fde_count == self.unwindend - f.tell():
                        for i in range(fde_count):
                            pc = self.unwindoff + f.read('i')
                            entry = self.unwindoff + f.read('i')
                            self.eh_table.append((pc, entry))

                    # TODO: we miss the last one, but better than nothing
                    last_entry = sorted(self.eh_table, key=lambda x: x[1])[-1][1]
                    builder.add_section('.eh_frame', eh_frame, end=last_entry)


        for off,sz,name,kind in [
            (self.textoff, self.textsize, '.text', 'CODE'),
            (self.rodataoff, self.rodatasize, '.rodata', 'CONST'),
            (self.dataoff, self.datasize, '.data', 'DATA'),
            (self.bssoff, self.bsssize, '.bss', 'BSS'),
        ]:
            builder.add_segment(off, sz, name, kind)

        self.sections = []
        for start, end, name, kind in builder.flatten():
            self.sections.append((start, end, name, kind))

        self._addr_to_name = None
        self._plt_lookup = None

    @property
    def addr_to_name(self):
        if self._addr_to_name is None:
            d = {}
            for sym in self.symbols:
                if sym.shndx:
                    d[sym.value] = sym.name
            self._addr_to_name = d
        return self._addr_to_name

    @property
    def plt_lookup(self):
        if self._plt_lookup is None:
            # generate lookups for .plt call redirection
            got_value_lookup = {}
            for offset, r_type, sym, addend in self.relocations:
                if r_type in (R_AARCH64_GLOB_DAT, R_AARCH64_JUMP_SLOT, R_AARCH64_ABS64) and addend == 0 and sym.shndx:
                    got_value_lookup[offset] = sym.value

            self._plt_lookup = {}
            for func, target in self.plt_entries:
                if target in got_value_lookup:
                    self._plt_lookup[func] = got_value_lookup[target]

        return self._plt_lookup

    def process_relocations(self, f, symbols, offset, size):
        locations = set()
        f.seek(offset)
        relocsize = 8 if self.armv7 else 0x18
        for i in xrange(size / relocsize):
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

    def process_relocations_relr(self, f, offset, size):
        locations = set()
        f.seek(offset)
        relocsize = 8
        for i in xrange(size / relocsize):
            entry = f.read('Q')
            if entry & 1:
                entry >>= 1
                i = 0
                while i < (relocsize * 8) - 1:
                    if entry & (1 << i):
                        locations.add(where + i * relocsize)
                        self.relocations.append((where + i * relocsize, R_FAKE_RELR, None, 0))
                    i += 1
                where += relocsize * ((relocsize * 8) - 1)
            else:
                # Where
                where = entry
                locations.add(where)
                self.relocations.append((where, R_FAKE_RELR, None, 0))
                where += relocsize
        return locations

    def get_dynstr(self, o):
        return self.dynstr[o:self.dynstr.index('\0', o)]

    def get_path_or_name(self):
        path = None
        for off, end, name, class_ in self.sections:
            if name == '.rodata' and end-off < 0x1000 and end-off > 8:
                id_ = self.binfile.read_from(end-off, off)
                length = struct.unpack_from('<I', id_, 4)[0]
                if length + 8 <= len(id_):
                    id_ = id_[8:length + 8]
                    return id_

        self.binfile.seek(self.rodataoff)
        as_string = self.binfile.read(self.rodatasize)
        if path is None:
            strs = re.findall(r'[a-z]:[\\/][ -~]{5,}\.n[rs]s', as_string, flags=re.IGNORECASE)
            if strs:
                return strs[-1]

        return None

    def get_name(self):
        name = self.get_path_or_name()
        if name is not None:
            name = name.split('/')[-1].split('\\')[-1]
            if name.lower().endswith(('.nss', '.nrs')):
                name = name[:-4]
        return name

class NsoFile(NxoFileBase):
    def __init__(self, fileobj):
        f = BinFile(fileobj)

        if f.read_from('4s', 0) != 'NSO0':
            raise NxoException('Invalid NSO magic')

        flags = f.read_from('I', 0xC)

        toff, tloc, tsize = f.read_from('III', 0x10)
        roff, rloc, rsize = f.read_from('III', 0x20)
        doff, dloc, dsize = f.read_from('III', 0x30)

        tfilesize, rfilesize, dfilesize = f.read_from('III', 0x60)
        bsssize = f.read_from('I', 0x3C)

        text = uncompress(f.read_from(tfilesize, toff), uncompressed_size=tsize) if flags & 1 else f.read_from(tfilesize, toff)
        ro   = uncompress(f.read_from(rfilesize, roff), uncompressed_size=rsize) if flags & 2 else f.read_from(rfilesize, roff)
        data = uncompress(f.read_from(dfilesize, doff), uncompressed_size=dsize) if flags & 4 else f.read_from(dfilesize, doff)

        full = text
        if rloc >= len(full):
            full += '\0' * (rloc - len(full))
        else:
            print 'truncating?'
            full = full[:rloc]
        full += ro
        if dloc >= len(full):
            full += '\0' * (dloc - len(full))
        else:
            print 'truncating?'
            full = full[:dloc]
        full += data
        self.full = full

        super(NsoFile, self).__init__(BinFile(StringIO(full)), (tloc, tsize, rloc, rsize, dloc, dsize))


class NroFile(NxoFileBase):
    def __init__(self, fileobj):
        f = BinFile(fileobj)

        if f.read_from('4s', 0x10) != 'NRO0':
            raise NxoException('Invalid NRO magic')

        f.seek(0x20)

        tloc, tsize = f.read('II')
        rloc, rsize = f.read('II')
        dloc, dsize = f.read('II')

        # copy f, since all other formats allow the original file to be closed
        f.seek(0)
        full = f.read(f.size())

        filecopy = BinFile(StringIO(full))
        super(NroFile, self).__init__(filecopy, (tloc, tsize, rloc, rsize, dloc, dsize))

def kip1_blz_decompress(compressed):
    compressed_size, init_index, uncompressed_addl_size = struct.unpack('<III', compressed[-0xC:])
    decompressed = compressed[:] + '\x00' * uncompressed_addl_size
    decompressed_size = len(decompressed)
    if not (compressed_size + uncompressed_addl_size):
        return ''
    decompressed = map(ord, decompressed)
    cmp_start = len(compressed) - compressed_size
    cmp_ofs   = compressed_size - init_index
    out_ofs   = compressed_size + uncompressed_addl_size
    while out_ofs > 0:
        cmp_ofs -= 1
        control = decompressed[cmp_start + cmp_ofs]
        for i in xrange(8):
            if control & 0x80:
                if cmp_ofs < 2 - cmp_start:
                    raise ValueError('Compression out of bounds!')
                cmp_ofs -= 2
                segmentoffset = decompressed[cmp_start + cmp_ofs] | (decompressed[cmp_start + cmp_ofs + 1] << 8)
                segmentsize = ((segmentoffset >> 12) & 0xF) + 3
                segmentoffset &= 0x0FFF
                segmentoffset += 2
                if out_ofs < segmentsize - cmp_start:
                    raise ValueError('Compression out of bounds!')
                for j in xrange(segmentsize):
                    if out_ofs + segmentoffset >= decompressed_size:
                        raise ValueError('Compression out of bounds!')
                    data = decompressed[cmp_start + out_ofs + segmentoffset]
                    out_ofs -= 1
                    decompressed[cmp_start + out_ofs] = data
            else:
                if out_ofs < 1 - cmp_start:
                    raise ValueError('Compression out of bounds!')
                out_ofs -= 1
                cmp_ofs -= 1
                decompressed[cmp_start + out_ofs] = decompressed[cmp_start + cmp_ofs]
            control <<= 1
            control &= 0xFF
            if not out_ofs:
                break
    return ''.join(map(chr, decompressed))

class KipFile(NxoFileBase):
    def __init__(self, fileobj):
        f = BinFile(fileobj)

        if f.read_from('4s', 0) != 'KIP1':
            raise NxoException('Invalid KIP magic')

        flags = f.read_from('b', 0x1F)

        tloc, tsize, tfilesize = f.read_from('III', 0x20)
        rloc, rsize, rfilesize = f.read_from('III', 0x30)
        dloc, dsize, dfilesize = f.read_from('III', 0x40)

        toff = 0x100
        roff = toff + tfilesize
        doff = roff + rfilesize

        bsssize = f.read_from('I', 0x18)


        text = kip1_blz_decompress(str(f.read_from(tfilesize, toff))) if flags & 1 else f.read_from(tfilesize, toff)
        ro   = kip1_blz_decompress(str(f.read_from(rfilesize, roff))) if flags & 2 else f.read_from(rfilesize, roff)
        data = kip1_blz_decompress(str(f.read_from(dfilesize, doff))) if flags & 4 else f.read_from(dfilesize, doff)

        full = text
        if rloc >= len(full):
            full += '\0' * (rloc - len(full))
        else:
            print 'truncating?'
            full = full[:rloc]
        full += ro
        if dloc >= len(full):
            full += '\0' * (dloc - len(full))
        else:
            print 'truncating?'
            full = full[:dloc]
        full += data

        super(KipFile, self).__init__(BinFile(StringIO(full)), (tloc, tsize, rloc, rsize, dloc, dsize))

class MemoryDumpFile(NxoFileBase):
    def __init__(self, fileobj):
        f = BinFile(fileobj)

        if f.read_from('4s', f.read_from('I', 4)) != 'MOD0':
            raise NxoException('Invalid MOD0 magic')

        f.seek(0)
        full = f.read(f.size())

        filecopy = BinFile(StringIO(full))
        super(MemoryDumpFile, self).__init__(filecopy)

class NxoException(Exception):
    pass

def looks_like_memory_dump(fileobj):
    fileobj.seek(0)
    header = fileobj.read(8)
    if len(header) < 8:
        return False
    modoff = struct.unpack_from('<I', header, 4)[0]
    if modoff + 0x1c < get_file_size(fileobj):
        fileobj.seek(modoff)
        if fileobj.read(4) == 'MOD0':
            return True
    return False


def load_nxo(fileobj):
    fileobj.seek(0)
    header = fileobj.read(0x14)

    if header[:4] == 'NSO0':
        return NsoFile(fileobj)
    elif header[:4] == 'KIP1':
        return KipFile(fileobj)
    elif header[0x10:0x14] == 'NRO0':
        return NroFile(fileobj)
    elif looks_like_memory_dump(fileobj):
        return MemoryDumpFile(fileobj)

    raise NxoException('not an NRO or NSO file')


try:
    import idaapi
    import idc
except ImportError:
    pass
else:
    # IDA specific code
    NRO_FORMAT = 'Switch binary (NRO)'
    NSO_FORMAT = 'Switch binary (NSO)'
    KIP_FORMAT = 'Switch binary (KIP)'
    RAW_FORMAT = 'Switch raw binary memory dump'
    SDK_FORMAT = 'Switch SDK binary (NSO, with arm64 .plt call rewriting hack)'
    RAW_SDK_FORMAT = 'Switch SDK (raw memory dump, with arm64 .plt call rewriting hack)'
    EXEFS_FORMAT = 'Switch exefs (multiple files, for use with Mephisto)'
    EXEFS_LOW_FORMAT = 'Switch exefs with 31-bit addressing (multiple files, for use with Mephisto)'

    OPT_BYPASS_PLT = 'BYPASS_PLT'
    OPT_EXEFS_LOAD = 'EXEFS_LOAD'
    OPT_LOAD_31_BIT = 'LOAD_31_BIT'

    FORMAT_OPTIONS = {
        NRO_FORMAT: [],
        NSO_FORMAT: [],
        KIP_FORMAT: [],
        RAW_FORMAT: [],
        SDK_FORMAT: [OPT_BYPASS_PLT],
        RAW_SDK_FORMAT: [OPT_BYPASS_PLT],
        EXEFS_FORMAT: [OPT_EXEFS_LOAD],
        EXEFS_LOW_FORMAT: [OPT_EXEFS_LOAD, OPT_LOAD_31_BIT],
    }

    PRIMARY_EXEFS_NAMES = ['main', 'rtld', 'sdk']
    LOAD_EXEFS_NAMES = PRIMARY_EXEFS_NAMES + ['subsdk%d' % i for i in range(10)]
    ALL_EXEFS_NAMES = LOAD_EXEFS_NAMES + ['main.npdm']

    def get_load_formats(li, path):
        li.seek(0)
        if li.read(4) == 'NSO0':
            if 'sdk' in os.path.basename(path):
                yield { 'format': SDK_FORMAT, 'options': idaapi.ACCEPT_FIRST }
                yield { 'format': NSO_FORMAT }
            else:
                yield { 'format': NSO_FORMAT, 'options': idaapi.ACCEPT_FIRST }
                yield { 'format': SDK_FORMAT }

        li.seek(0)
        if li.read(4) == 'KIP1':
            yield { 'format': KIP_FORMAT, 'options': idaapi.ACCEPT_FIRST }

        li.seek(0x10)
        if li.read(4) == 'NRO0':
            yield { 'format': NRO_FORMAT, 'options': idaapi.ACCEPT_FIRST }
        elif looks_like_memory_dump(li):
            yield { 'format': RAW_FORMAT, 'options': idaapi.ACCEPT_FIRST }
            yield { 'format': RAW_SDK_FORMAT }

        if os.path.basename(path) in ALL_EXEFS_NAMES:
            dirname = os.path.dirname(path)
            if all(os.path.exists(os.path.join(dirname, i)) for i in PRIMARY_EXEFS_NAMES):
                yield { 'format': EXEFS_FORMAT }
                yield { 'format': EXEFS_LOW_FORMAT }



    # this is mostly just to work with the IDA API
    accept_formats_list = None
    accept_formats_index = 0

    def accept_file(li, path):
        global accept_formats_list
        global accept_formats_index

        if accept_formats_list is None:
            accept_formats_list = list(get_load_formats(li, path))
            accept_formats_index = 0

        if accept_formats_index >= len(accept_formats_list):
            accept_formats_list = None
            accept_formats_index = 0
            return 0

        ret = accept_formats_list[accept_formats_index]
        if not isinstance(ret, dict):
            ret = { 'format': ret }
        if 'options' not in ret:
            ret['options'] = 1
        if 'processor' not in ret:
            ret['processor'] = 'arm'
        ret['options'] |= 1 | idaapi.ACCEPT_CONTINUE

        accept_formats_index += 1

        return ret

    def ida_make_offset(f, ea):
        if f.armv7:
            idc.MakeDword(ea)
        else:
            idc.MakeQword(ea)
        idc.OpOff(ea, 0, 0)

    def find_bl_targets(text_start, text_end):
        targets = set()
        for pco in xrange(0, text_end - text_start, 4):
            pc = text_start + pco
            d = Dword(pc)
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

    def load_file(li, neflags, fmt):
        idaapi.set_processor_type('arm', SETPROC_ALL|SETPROC_FATAL)

        options = FORMAT_OPTIONS[fmt]

        if OPT_EXEFS_LOAD in options:
            ret = load_as_exefs(li, options)
        else:
            ret = load_one_file(li, options, 0)

        eh_parse = idaapi.find_plugin('eh_parse', True)
        if eh_parse:
            print 'eh_parse ->', idaapi.run_plugin(eh_parse, 0)
        else:
            print 'warning: eh_parse missing'

        return ret

    def load_as_exefs(li, options):
        dirname = os.path.dirname(idc.get_input_file_path())
        binaries = LOAD_EXEFS_NAMES
        binaries = [os.path.join(dirname, i) for i in binaries]
        binaries = [i for i in binaries if os.path.exists(i)]
        for idx, fname in enumerate(binaries):
            with open(fname, 'rb') as f:
                if not load_one_file(f, options, idx, os.path.basename(fname)):
                    return False
        return True

    def load_one_file(li, options, idx, basename=None):
        bypass_plt = OPT_BYPASS_PLT in options

        f = load_nxo(li)

        if idx == 0:
            if f.armv7:
                idc.SetShortPrm(idc.INF_LFLAGS, idc.GetShortPrm(idc.INF_LFLAGS) | idc.LFLG_PC_FLAT)
            else:
                idc.SetShortPrm(idc.INF_LFLAGS, idc.GetShortPrm(idc.INF_LFLAGS) | idc.LFLG_64BIT)

            idc.SetCharPrm(idc.INF_DEMNAMES, idaapi.DEMNAM_GCC3)
            idaapi.set_compiler_id(idaapi.COMP_GNU)
            idaapi.add_til2('gnulnx_arm' if f.armv7 else 'gnulnx_arm64', 1)
            # don't create tails
            idc.set_inf_attr(idc.INF_AF, idc.get_inf_attr(idc.INF_AF) & ~idc.AF_FTAIL)

        if OPT_LOAD_31_BIT in options:
            loadbase = 0x8000000
            step = 0x1000000
        elif f.armv7:
            loadbase = 0x60000000
            step = 0x10000000
        else:
            loadbase = 0x7100000000
            step = 0x100000000
        loadbase += idx * step

        f.binfile.seek(0)
        as_string = f.binfile.read(f.bssoff)
        idaapi.mem2base(as_string, loadbase)

        seg_prefix = basename if basename is not None else ''
        for start, end, name, kind in f.sections:
            if name.startswith('.got'):
                kind = 'CONST'
            idaapi.add_segm(0, loadbase+start, loadbase+end, seg_prefix+name, kind)
            segm = idaapi.get_segm_by_name(seg_prefix+name)
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

        undef_seg = basename + '.UNDEF' if basename is not None else 'UNDEF'
        idaapi.add_segm(0, undef_ea, undef_ea+undef_count*undef_entry_size, undef_seg, 'XTRN')
        segm = idaapi.get_segm_by_name(undef_seg)
        segm.type = idaapi.SEG_XTRN
        idaapi.update_segm(segm)
        for i,s in enumerate(f.symbols):
            if not s.shndx and s.name:
                idc.MakeQword(undef_ea)
                idaapi.do_name_anyway(undef_ea, s.name)
                s.resolved = undef_ea
                undef_ea += undef_entry_size
            elif i != 0:
                assert s.shndx
                s.resolved = loadbase + s.value
                if s.name:
                    if s.type == STT_FUNC:
                        idaapi.add_entry(s.resolved, s.resolved, s.name, 0)
                    else:
                        idaapi.do_name_anyway(s.resolved, s.name)

            else:
                # NULL symbol
                s.resolved = 0

        funcs = set()
        for s in f.symbols:
            if s.name and s.shndx and s.value:
                if s.type == STT_FUNC:
                    funcs.add(loadbase+s.value)
                    symend = loadbase+s.value+s.size
                    if Dword(symend) != 0:
                        funcs.add(symend)

        got_name_lookup = {}
        for offset, r_type, sym, addend in f.relocations:
            target = offset + loadbase
            if r_type in (R_ARM_GLOB_DAT, R_ARM_JUMP_SLOT, R_ARM_ABS32):
                if not sym:
                    print 'error: relocation at %X failed' % target
                else:
                    idaapi.put_long(target, sym.resolved)
            elif r_type == R_ARM_RELATIVE:
                idaapi.put_long(target, idaapi.get_long(target) + loadbase)
            elif r_type in (R_AARCH64_GLOB_DAT, R_AARCH64_JUMP_SLOT, R_AARCH64_ABS64):
                idaapi.put_qword(target, sym.resolved + addend)
                if addend == 0:
                    got_name_lookup[offset] = sym.name
            elif r_type == R_AARCH64_RELATIVE:
                idaapi.put_qword(target, loadbase + addend)
                if addend < f.textsize:
                    funcs.add(loadbase + addend)
            elif r_type == R_FAKE_RELR:
                assert not f.armv7 # TODO
                addend = idaapi.get_qword(target)
                idaapi.put_qword(target, addend + loadbase)
                if addend < f.textsize:
                    funcs.add(loadbase + addend)
            else:
                print 'TODO r_type %d' % (r_type,)
            ida_make_offset(f, target)

        for func, target in f.plt_entries:
            if target in got_name_lookup:
                addr = loadbase + func
                funcs.add(addr)
                idaapi.do_name_anyway(addr, got_name_lookup[target])

        if not f.armv7:
            funcs |= find_bl_targets(loadbase, loadbase+f.textsize)

            if bypass_plt:
                plt_lookup = f.plt_lookup
                for pco in xrange(0, f.textsize, 4):
                    pc = loadbase + pco
                    d = Dword(pc)
                    if (d & 0x7c000000) == (0x94000000 & 0x7c000000):
                        imm = d & 0x3ffffff
                        if imm & 0x2000000:
                            imm |= ~0x1ffffff
                        if 0 <= imm <= 2:
                            continue
                        target = (pc + imm * 4) - loadbase
                        if target in plt_lookup:
                            new_target = plt_lookup[target] + loadbase
                            new_instr = (d & ~0x3ffffff) | (((new_target - pc) / 4) & 0x3ffffff)
                            idaapi.put_long(pc, new_instr)

            for pco in xrange(0, f.textsize, 4):
                pc = loadbase + pco
                d = Dword(pc)
                if d == 0x14000001:
                    funcs.add(pc + 4)

        for pc, _ in f.eh_table:
            funcs.add(loadbase + pc)

        for addr in sorted(funcs, reverse=True):
            idaapi.auto_make_proc(addr)

        return 1
