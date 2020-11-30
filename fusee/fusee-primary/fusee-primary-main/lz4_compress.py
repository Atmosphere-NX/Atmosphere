#!/usr/bin/env python
import sys, lz4
from struct import unpack as up

def lz4_compress(data):
    try:
        import lz4.block as block
    except ImportError:
        block = lz4.LZ4_compress
    return block.compress(data, 'high_compression', store_size=False)

def main(argc, argv):
    if argc != 3:
        print('Usage: %s in out' % argv[0])
        return 1
    with open(argv[1], 'rb') as f:
        data = f.read()
    with open(argv[2], 'wb') as f:
        f.write(lz4_compress(data))
    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
