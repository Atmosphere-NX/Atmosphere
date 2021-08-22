#!/usr/bin/env python
import sys, lz4, os
from struct import unpack as up

def lz4_compress(data):
    try:
        import lz4.block as block
    except ImportError:
        block = lz4.LZ4_compress
    return block.compress(data, 'high_compression', store_size=False)

def read_file(fn):
    with open(fn, 'rb') as f:
        return f.read()

def write_file(fn, data):
    with open(fn, 'wb') as f:
        f.write(data)

def main(argc, argv):
    if argc != 1:
        print('Usage: %s' % argv[0])
        return 1
    params = {
        'erista' : {},
        'mariko' : {},
    }
    for fn in os.listdir('sdram_params/bin'):
        assert fn.startswith('sdram_params_') and fn.endswith('.bin')
        (_sdram, _params, soc, _id_bin) = tuple(fn.split('_'))
        param_id = int(_id_bin[:-len('.bin')])
        assert soc in params.keys()
        compressed = lz4_compress(read_file(os.path.join('sdram_params/bin', fn)))
        write_file(os.path.join('sdram_params/lz', fn.replace('.bin', '.lz4')), compressed)
        params[soc][param_id] = compressed
    with open('source/fusee_sdram_params.inc', 'w') as f:
        f.write('%s\n' % "/*")
        f.write('%s\n' % " * Copyright (c) 2018-2020 Atmosph\xc3re-NX")
        f.write('%s\n' % " *")
        f.write('%s\n' % " * This program is free software; you can redistribute it and/or modify it")
        f.write('%s\n' % " * under the terms and conditions of the GNU General Public License,")
        f.write('%s\n' % " * version 2, as published by the Free Software Foundation.")
        f.write('%s\n' % " *")
        f.write('%s\n' % " * This program is distributed in the hope it will be useful, but WITHOUT")
        f.write('%s\n' % " * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or")
        f.write('%s\n' % " * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for")
        f.write('%s\n' % " * more details.")
        f.write('%s\n' % " *")
        f.write('%s\n' % " * You should have received a copy of the GNU General Public License")
        f.write('%s\n' % " * along with this program.  If not, see <http://www.gnu.org/licenses/>.")
        f.write('%s\n' % " */")
        f.write('\n')
        for soc in ('Erista', 'Mariko'):
            for param_id in sorted(params[soc.lower()].keys()):
                compressed = params[soc.lower()][param_id]
                f.write('%s\n' % ('constexpr inline const u8 SdramParams%s%d[0x%03X] = {' % (soc, param_id, len(compressed))))
                while compressed:
                    block = compressed[:0x10]
                    compressed = compressed[0x10:]
                    f.write('    %s\n' % (', '.join('0x%02X' % ord(c) for c in block) + ','))
                f.write('};\n\n')
    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
