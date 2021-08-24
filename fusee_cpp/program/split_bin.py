#!/usr/bin/env python
import sys, lz4
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

def pad(data, size):
    return (data + '\x00' * size)[:size]

def get_overlay(program, i):
    return program[0x2B000 + 0x14000 * i:0x2B000 + 0x14000 * (i+1)]

def main(argc, argv):
    if argc == 2:
        target = argv[1]
        if len(target) == 0:
            target = ''
    elif argc == 1:
        target = ''
    else:
        print('Usage: %s target' % argv[0])
        return 1
    with open('../../program%s.bin' % target, 'rb') as f:
        data = f.read()
    with open('../../program%s.lz4' % target, 'wb') as f:
        f.write(lz4_compress(data[:0x2B000] + get_overlay(data, 0)[:0x11000]))
    with open('../../fusee-boogaloo%s.bin' % target, 'wb') as f:
        # TODO: Write header
        f.write('\xCC'*0x800)
        # TODO: Write warmboot
        f.write('\xCC'*0x1800)
        # Write TSEC Keygen
        f.write(pad(read_file('../../tsec_keygen/tsec_keygen.bin'), 0x2000))
        # TODO: Write Mariko Fatal
        f.write('\xCC'*0x1C000)
        # Write Erista MTC
        f.write(get_overlay(data, 1))
        # Write Mariko MTC
        f.write(get_overlay(data, 2))
        # TODO: Write exosphere
        f.write('\xCC'*0x10000)
        # TODO: Write mesosphere
        f.write('\xCC'*0xA8000)
        # TODO: Write kips
        f.write('\xCC'*0x300000)
        # Write Splash Screen
        f.write(pad(read_file('../../splash_screen/splash_screen.bin'), 0x3C0000))

    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
