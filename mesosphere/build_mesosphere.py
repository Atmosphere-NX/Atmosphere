#!/usr/bin/env python
import sys, os
from struct import pack as pk, unpack as up

def atmosphere_target_firmware(major, minor, micro, rev = 0):
    return (major << 24) | (minor << 16) | (micro << 8) | rev

def align_up(val, algn):
    val += algn - 1
    return val - (val % algn)

def main(argc, argv):
    if argc < 4:
        print('Usage: %s kernel_ldr.bin kernel.bin output.bin [initial_process.kip ...]' % argv[0])
        return 1
    with open(argv[1], 'rb') as f:
        kernel_ldr = f.read()
    with open(argv[2], 'rb') as f:
        kernel = f.read()
    kernel_metadata_offset = 4
    assert (kernel_metadata_offset <= len(kernel) - 0x40)
    assert (kernel[kernel_metadata_offset:kernel_metadata_offset + 4] == b'MSS0')

    bss_start, bss_end, kernel_end = up('<III', kernel[kernel_metadata_offset + 0x30:kernel_metadata_offset + 0x3C])
    assert (bss_end >= bss_start)
    assert (bss_end == kernel_end)

    assert (len(kernel) <= kernel_end)
    if len(kernel) < kernel_end:
        kernel += b'\x00' * (kernel_end - len(kernel))
    assert (kernel_end == len(kernel))

    embedded_kips = b''
    num_kips = 0
    for kip_file in argv[4:]:
        try:
            with open(kip_file, 'rb') as f:
                data = f.read()
                if data.startswith(b'KIP1'):
                    embedded_kips += data
                    num_kips += 1
        except:
            pass
    if num_kips > 0:
        embedded_ini_header = pk('<4sIII', b'INI1', len(embedded_kips) + 0x10, num_kips, 0)
    else:
        embedded_ini_header = b''
    embedded_ini_offset = align_up(kernel_end, 0x1000)
    embedded_ini_end = embedded_ini_offset + len(embedded_ini_header) + len(embedded_kips)

    kernel_ldr_offset = align_up(embedded_ini_end, 0x1000) + (0x1000 if len(embedded_ini_header) == 0 else 0)
    kernel_ldr_end    = kernel_ldr_offset + len(kernel_ldr)
    mesosphere_end    = align_up(kernel_ldr_end, 0x1000)

    with open(argv[3], 'wb') as f:
        f.write(kernel[:kernel_metadata_offset + 4])
        f.write(pk('<QQI', embedded_ini_offset, kernel_ldr_offset, atmosphere_target_firmware(13, 0, 0)))
        f.write(kernel[kernel_metadata_offset + 0x18:])
        f.seek(embedded_ini_offset)
        f.write(embedded_ini_header)
        f.write(embedded_kips)
        f.seek(embedded_ini_end)
        f.seek(kernel_ldr_offset)
        f.write(kernel_ldr)
        f.seek(mesosphere_end)
        f.write(b'\x00'*0x1000)
    return 0


if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
