#!/usr/bin/env python
import sys, lz4, hashlib
from struct import unpack as up, pack as pk

def lz4_compress(data):
    try:
        import lz4.block as block
    except ImportError:
        block = lz4.LZ4_compress
    return block.compress(data, 'high_compression', store_size=False)

def read_file(fn):
    with open(fn, 'rb') as f:
        return f.read()

def get_overlay(program, i):
    return program[0x2B000 + 0x14000 * i:0x2B000 + 0x14000 * (i+1)]

def main(argc, argv):
    if argc != 3:
        print('Usage: %s in out' % argv[0])
        return 1
    data = read_file(argv[1])
    erista_mtc = get_overlay(data, 1)
    mariko_mtc = get_overlay(data, 2)
    erista_hsh = hashlib.sha256(erista_mtc[:-4]).digest()[:4]
    mariko_hsh = hashlib.sha256(mariko_mtc[:-4]).digest()[:4]
    fusee_program = lz4_compress(data[:0x2B000 - 8] + erista_hsh + mariko_hsh + get_overlay(data, 0)[:0x11000])
    with open(argv[2], 'wb') as f:
        f.write(fusee_program)

    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
