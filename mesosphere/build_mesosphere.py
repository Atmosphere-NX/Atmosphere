#!/usr/bin/env python
import sys, os
from struct import pack as pk, unpack as up

def atmosphere_target_firmware(major, minor, micro, rev = 0):
    return (major << 24) | (minor << 16) | (micro << 8) | rev

def align_up(val, algn):
    val += algn - 1
    return val - (val % algn)

def find_rela(kernel, dynamic):
    rela_offset, rela_size = (None, None)
    while True:
        dyn_type, dyn_val = up('<QQ', kernel[dynamic:dynamic+0x10])
        if dyn_type == 0:
            break
        elif dyn_type == 7:
            assert (rela_offset is None)
            rela_offset = dyn_val
        elif dyn_type == 8:
            assert (rela_size is None)
            rela_size = dyn_val
        dynamic += 0x10
    assert (rela_offset is not None)
    assert (rela_size is not None)
    return (rela_offset, rela_size)

def main(argc, argv):
    if argc != 4:
        print('Usage: %s kernel_ldr.bin kernel.bin output.bin' % argv[0])
        return 1
    with open(argv[1], 'rb') as f:
        kernel_ldr = f.read()
    with open(argv[2], 'rb') as f:
        kernel = f.read()
    kernel_metadata_offset = 4
    assert (kernel_metadata_offset <= len(kernel) - 0x40)
    assert (kernel[kernel_metadata_offset:kernel_metadata_offset + 4] == b'MSS0')
    bss_start, bss_end, kernel_end, dynamic = up('<IIII', kernel[kernel_metadata_offset + 0x30:kernel_metadata_offset + 0x40])
    assert (bss_end >= bss_start)
    bss_size = bss_end - bss_start
    assert (bss_end == kernel_end)
    assert (kernel_end <= len(kernel))

    rela_offset, rela_size = find_rela(kernel, dynamic)
    assert (rela_size == len(kernel) - kernel_end)
    assert (bss_start <= rela_offset and rela_offset + rela_size <= bss_end)
    assert (kernel[bss_start:bss_end] == (b'\x00'* bss_size))

    kernel = kernel[:rela_offset] + kernel[bss_end:] + (b'\x00' * (bss_size - (rela_size + (rela_offset - bss_start))))
    assert (kernel_end == len(kernel))

    embedded_ini = b''
    try:
        with open('ini.bin', 'rb') as f:
            embedded_ini = f.read()
    except:
        pass
    embedded_ini_offset = align_up(kernel_end, 0x1000)
    embedded_ini_end = embedded_ini_offset + len(embedded_ini) # TODO: Create and embed an INI, eventually.

    kernel_ldr_offset = align_up(embedded_ini_end, 0x1000) + (0x1000 if len(embedded_ini) == 0 else 0)
    kernel_ldr_end    = kernel_ldr_offset + len(kernel_ldr)
    mesosphere_end    = align_up(kernel_ldr_end, 0x1000)

    with open(argv[3], 'wb') as f:
        f.write(kernel[:kernel_metadata_offset + 4])
        f.write(pk('<QQI', embedded_ini_offset, kernel_ldr_offset, atmosphere_target_firmware(13, 0, 0)))
        f.write(kernel[kernel_metadata_offset + 0x18:])
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
