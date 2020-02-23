#!/usr/bin/env python
import sys, os
from struct import pack as pk, unpack as up

def align_up(val, algn):
    val += algn - 1
    return val - (val % algn)


def main(argc, argv):
    if argc != 1:
        print('Usage: %s' % argv[0])
        return 1
    with open('kernel_ldr/kernel_ldr.bin', 'rb') as f:
        kernel_ldr = f.read()
    with open('kernel/kernel.bin', 'rb') as f:
        kernel = f.read()
    kernel_metadata_offset = 4
    assert (kernel_metadata_offset <= len(kernel) - 0x40)
    assert (kernel[kernel_metadata_offset:kernel_metadata_offset + 4] == b'MSS0')
    kernel_end = up('<I', kernel[kernel_metadata_offset + 0x34:kernel_metadata_offset + 0x38])[0]
    assert (kernel_end >= len(kernel))

    embedded_ini = b''
    try:
        with open('ini.bin', 'rb') as f:
            embedded_ini = f.read()
    except:
        pass
    embedded_ini_offset = align_up(kernel_end, 0x1000) + 0x1000
    embedded_ini_end = embedded_ini_offset + len(embedded_ini) # TODO: Create and embed an INI, eventually.

    kernel_ldr_offset = align_up(embedded_ini_end, 0x1000) + 0x1000
    kernel_ldr_end    = kernel_ldr_offset + len(kernel_ldr)
    mesosphere_end    = align_up(kernel_ldr_end, 0x1000)

    with open('mesosphere.bin', 'wb') as f:
        f.write(kernel[:kernel_metadata_offset + 4])
        f.write(pk('<QQ', embedded_ini_offset, kernel_ldr_offset))
        f.write(kernel[kernel_metadata_offset + 0x14:])
        f.seek(embedded_ini_offset)
        f.write(embedded_ini)
        f.seek(embedded_ini_end)
        f.seek(kernel_ldr_offset)
        f.write(kernel_ldr)
        f.seek(mesosphere_end)
        f.write(b'\x00'*0x1000)
    return 0


if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
