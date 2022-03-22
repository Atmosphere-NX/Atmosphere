#!/usr/bin/env python
import sys, lz4
from struct import unpack as up

def lz4_compress(data):
    try:
        import lz4.block as block
    except ImportError:
        block = lz4.LZ4_compress
    return block.compress(data, 'high_compression', store_size=False)

def split_binary(data):
    A, B, START, BOOT_CODE_START, BOOT_CODE_END, PROGRAM_START, C, D = up('<QQQQQQQQ', data[:0x40])
    assert A == 0xAAAAAAAAAAAAAAAA
    assert B == 0xBBBBBBBBBBBBBBBB
    assert C == 0xCCCCCCCCCCCCCCCC
    assert D == 0xDDDDDDDDDDDDDDDD
    data = data[0x40:]

    #print ('%X %X %X %X' % (START, BOOT_CODE_START, BOOT_CODE_END, PROGRAM_START))
    boot_code = data[BOOT_CODE_START - START:BOOT_CODE_END - BOOT_CODE_START]
    program   = data[PROGRAM_START   - START:]
    return [('boot_code%s.lz4', lz4_compress(boot_code)), ('program%s.lz4', lz4_compress(program))]

def main(argc, argv):
    if argc != 4:
        print('Usage: %s in suffix outdir' % argv[0])
        return 1
    with open(argv[1], 'rb') as f:
        data = f.read()
    assert len(data) >= 0x40
    for (fn, fdata) in split_binary(data):
        with open('%s/%s' % (argv[3], fn % argv[2]), 'wb') as f:
            f.write(fdata)
    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
