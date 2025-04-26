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

def get_param_size(soc):
    return {
        'erista' : 0x1340,
        'mariko' : 0x10CC,
    }[soc]

def main(argc, argv):
    if argc != 1:
        print('Usage: %s' % argv[0])
        return 1
    params = {
        'erista' : {},
        'mariko' : {},
    }
    compressed_params = {
        'erista' : {},
        'mariko' : {},
    }
    for board in os.listdir('mtc_tables/bin'):
        if board.startswith('T210b01Sdev'):
            soc = 'mariko'
            count = 3
        else:
            assert board.startswith('T210Sdev')
            soc = 'erista'
            count = 10
        assert os.listdir('mtc_tables/bin/%s' % board) == ['%d.bin' % i for i in xrange(count)]
        params[soc][board] = []
        compressed_params[soc][board] = []
        for i in xrange(count):
            uncompressed = read_file(os.path.join('mtc_tables/bin/%s' % board, '%d.bin' % i))
            assert len(uncompressed) == get_param_size(soc)
            compressed = lz4_compress(uncompressed)
            params[soc][board].append(uncompressed)
            compressed_params[soc][board].append(compressed)
            try:
                os.makedirs('mtc_tables/lz/%s' % board)
            except:
                pass
            write_file(os.path.join('mtc_tables/lz/%s' % board, '%d.lz4' % i), compressed)
        try:
            os.makedirs('mtc_tables/combined/%s' % board)
        except:
            pass
        data_1600 = params[soc][board][-1]
        data_800 = params[soc][board][-4] if soc == 'erista' else ''
        data_204  = params[soc][board][0] if soc == 'mariko' else params[soc][board][3]
        assert up('<I', data_1600[0x40:0x44])[0] == 1600000
        assert up('<I', data_204[0x40:0x44])[0] == 204000
        if soc == 'erista':
            assert up('<I', data_800[0x40:0x44])[0] == 800000
        if soc == 'mariko':
            data = lz4_compress(data_204 + data_1600)
        else:
            data = data_204 + data_800 + data_1600
        write_file(os.path.join('mtc_tables/combined/%s' % board, 'table.bin'), data)
    for soc in ('erista', 'mariko'):
        with open('source/mtc/fusee_mtc_tables_%s.inc' % soc, 'w') as f:
            f.write('%s\n' % "/*")
            f.write('%s\n' % " * Copyright (c) Atmosph\xc3\xa8re-NX")
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
            for board in compressed_params[soc].keys():
                f.write('%s\n' % ('constexpr const u8 %s[] = {' % (board)))
                f.write('%s\n' % ('    #embed "../../mtc_tables/combined/%s/table.bin"' % (board)))
                f.write('};\n\n')
    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
