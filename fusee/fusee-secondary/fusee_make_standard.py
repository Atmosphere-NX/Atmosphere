#!/usr/bin/env python
import sys, os
from struct import pack as pk, unpack as up

def make_standard(exp):
    std = exp[:]
    _, metadata_offset, is_exp = up('<III', exp[:12])
    assert is_exp == 1

    # Patch the experimental flag to zero.
    std = std[:8] + pk('<I', 0) + std[12:]

    # Locate the mesosphere content header, patch to be experimental.
    magic, size, code_ofs, content_ofs, num_contents, ver, sup_ver, rev = up('<IIIIIIII', exp[metadata_offset:metadata_offset + 0x20])
    for i in range(num_contents):
        start, size, cnt_type, flag0, flag1, flag2, pad = up('<IIBBBBI', exp[content_ofs + 0x20 * i:content_ofs + 0x20 * i + 0x10])
        if cnt_type == 10: # CONTENT_TYPE_KRN
            assert exp[content_ofs + 0x20 * i + 0x10:content_ofs + 0x20 * i + 0x10 + len(b'mesosphere') + 1] == (b'mesosphere\x00')
            assert flag0 == 0 and flag1 == 0 and flag2 == 0
            std = std[:content_ofs + 0x20 * i] + pk('<IIBBBBI', start, size, cnt_type, flag0 | 0x1, flag1, flag2, pad) + std[content_ofs + 0x20 * i + 0x10:]

    return std


def main(argc, argv):
    if argc != 3:
        print('Usage: %s input output' % argv[0])
        return 1
    with open(argv[1], 'rb') as f:
        experimental = f.read()
    with open(argv[2], 'wb') as f:
        f.write(make_standard(experimental))
    return 0


if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
