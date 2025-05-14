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
        'erista' : 0x768,
        'mariko' : 0x838,
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
    for fn in os.listdir('sdram_params/bin'):
        assert fn.startswith('sdram_params_') and fn.endswith('.bin')
        (_sdram, _params, soc, _id_bin) = tuple(fn.split('_'))
        param_id = int(_id_bin[:-len('.bin')])
        assert soc in params.keys()
        uncompressed = read_file(os.path.join('sdram_params/bin', fn))
        assert len(uncompressed) == get_param_size(soc)
        params[soc][param_id] = uncompressed
    for soc in ('erista', 'mariko'):
        empty_params = '\x00' * get_param_size(soc)
        for param_id in xrange(0, max(params[soc].keys()) + 4, 2):
            param_id_l = param_id
            param_id_h = param_id + 1
            if param_id_l in params[soc] or param_id_h in params[soc]:
                print soc, param_id_l, param_id_h
                param_l = params[soc][param_id_l] if param_id_l in params[soc] else empty_params
                param_h = params[soc][param_id_h] if param_id_h in params[soc] else empty_params
                compressed = lz4_compress(param_l + param_h)
                compressed_params[soc][(param_id_l, param_id_h)] = compressed
                write_file(os.path.join('sdram_params/lz', 'sdram_params_%s_%d_%d.lz4' % (soc, param_id_l, param_id_h)), compressed)
    with open('source/sdram/fusee_sdram_params.inc', 'w') as f:
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
        for soc in ('Erista', 'Mariko'):
            for (param_id_l, param_id_h) in sorted(compressed_params[soc.lower()].keys(), key=lambda (l, h): l):
                compressed = compressed_params[soc.lower()][(param_id_l, param_id_h)]
                f.write('%s\n' % ('constexpr inline const u8 SdramParams%s%d_%d[0x%03X] = {' % (soc, param_id_l, param_id_h, len(compressed))))
                f.write('%s\n' % ('    #embed "../../sdram_params/lz/sdram_params_%s_%d_%d.lz4"' % (soc.lower(), param_id_l, param_id_h)))
                f.write('};\n\n')
                f.write('%s\n' % ('constexpr inline const u8 * const SdramParams%s%d = SdramParams%s%d_%d;' % (soc, param_id_l, soc, param_id_l, param_id_h)))
                f.write('%s\n' % ('constexpr inline const size_t SdramParamsSize%s%d = sizeof(SdramParams%s%d_%d);' % (soc, param_id_l, soc, param_id_l, param_id_h)))
                f.write('\n')
                f.write('%s\n' % ('constexpr inline const u8 * const SdramParams%s%d = SdramParams%s%d_%d;' % (soc, param_id_h, soc, param_id_l, param_id_h)))
                f.write('%s\n' % ('constexpr inline const size_t SdramParamsSize%s%d = sizeof(SdramParams%s%d_%d);' % (soc, param_id_h, soc, param_id_l, param_id_h)))
                f.write('\n')
    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
