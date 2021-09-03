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

def pad(data, size):
    assert len(data) <= size
    return (data + '\x00' * size)[:size]

def get_overlay(program, i):
    return program[0x2B000 + 0x14000 * i:0x2B000 + 0x14000 * (i+1)]

KIP_NAMES = ['Loader', 'NCM', 'ProcessManager', 'sm', 'boot', 'spl', 'ams_mitm']

def get_kips():
    emummc   = read_file('../../../../emummc/emummc.kip')
    loader   = read_file('../../../../stratosphere/loader/loader.kip')
    ncm      = read_file('../../../../stratosphere/ncm/ncm.kip')
    pm       = read_file('../../../../stratosphere/pm/pm.kip')
    sm       = read_file('../../../../stratosphere/sm/sm.kip')
    boot     = read_file('../../../../stratosphere/boot/boot.kip')
    spl      = read_file('../../../../stratosphere/spl/spl.kip')
    ams_mitm = read_file('../../../../stratosphere/ams_mitm/ams_mitm.kip')
    return (emummc, {
        'Loader' : loader,
        'NCM' : ncm,
        'ProcessManager' : pm,
        'sm' : sm,
        'boot' : boot,
        'spl' : spl,
        'ams_mitm' : ams_mitm,
    })

def write_kip_meta(f, kip, ofs):
    # Write program id
    f.write(kip[0x10:0x18])
    # Write offset, size
    f.write(pk('<II', ofs, len(kip)))
    # Write hash
    f.write(hashlib.sha256(kip).digest())

def write_header(f, all_kips):
    # Unpack kips
    emummc, kips = all_kips
    # Write reserved0 (previously entrypoint) as infinite loop instruction.
    f.write(pk('<I', 0xEAFFFFFE))
    # Write metadata offset = 0x10
    f.write(pk('<I', 0x20))
    # Write flags
    f.write(pk('<I', 0x00000000))
    # Write num_kips
    f.write(pk('<I', len(KIP_NAMES)))
    # Write reserved2
    f.write(b'\xCC' * 0x10)
    # Write magic
    f.write('FSS0')
    # Write total size
    f.write(pk('<I', 0x800000))
    # Write reserved3
    f.write(pk('<I', 0xCCCCCCCC))
    # Write content_header_offset
    f.write(pk('<I', 0x40))
    # Write num_content_headers;
    f.write(pk('<I', 8 + len(KIP_NAMES)))
    # Write supported_hos_version;
    f.write(pk('<I', 0xCCCCCCCC)) # TODO
    # Write release_version;
    f.write(pk('<I', 0xCCCCCCCC)) # TODO
    # Write git_revision;
    f.write(pk('<I', 0xCCCCCCCC)) # TODO
    # Write content metas
    f.write(pk('<IIBBBBI16s', 0x000800, 0x001800,  2, 0, 0, 0, 0xCCCCCCCC, 'warmboot'))
    f.write(pk('<IIBBBBI16s', 0x002000, 0x002000, 12, 0, 0, 0, 0xCCCCCCCC, 'tsec_keygen'))
    f.write(pk('<IIBBBBI16s', 0x004000, 0x01C000, 11, 0, 0, 0, 0xCCCCCCCC, 'exosphere_fatal'))
    f.write(pk('<IIBBBBI16s', 0x048000, 0x00E000,  1, 0, 0, 0, 0xCCCCCCCC, 'exosphere'))
    f.write(pk('<IIBBBBI16s', 0x056000, 0x0AA000, 10, 0, 0, 0, 0xCCCCCCCC, 'mesosphere'))
    f.write(pk('<IIBBBBI16s', 0x7C0000, 0x020000,  0, 0, 0, 0, 0xCCCCCCCC, 'fusee'))
    f.write(pk('<IIBBBBI16s', 0x7E0000, 0x001000,  3, 0, 0, 0, 0xCCCCCCCC, 'rebootstub'))
    f.write(pk('<IIBBBBI16s', 0x100000, len(emummc),  8, 0, 0, 0, 0xCCCCCCCC, 'emummc'))
    ofs = 0x100000 + len(emummc)
    for kip_name in KIP_NAMES:
        kip_data = kips[kip_name]
        f.write(pk('<IIBBBBI16s', ofs, len(kip_data), 6, 0, 0, 0, 0xCCCCCCCC, kip_name))
        ofs += len(kip_data)
    # Pad to kip metas.
    f.write(b'\xCC' * (0x400 - 0x40 - (0x20 * (8 + len(KIP_NAMES)))))
    # Write emummc_meta. */
    write_kip_meta(f, emummc, 0x100000)
    # Write kip metas
    ofs = 0x100000 + len(emummc)
    for kip_name in KIP_NAMES:
        kip_data = kips[kip_name]
        write_kip_meta(f, kip_data, ofs)
        ofs += len(kip_data)
    # Pad to end of header
    f.write(b'\xCC' * (0x800 - (0x400 + (1 + len(KIP_NAMES)) * 0x30)))

def write_kips(f, all_kips):
    # Unpack kips
    emummc, kips = all_kips
    # Write emummc
    f.write(emummc)
    # Write kips
    tot = len(emummc)
    for kip_name in KIP_NAMES:
        kip_data = kips[kip_name]
        f.write(kip_data)
        tot += len(kip_data)
    # Pad to 3 MB
    f.write(b'\xCC' * (0x300000 - tot))

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
    all_kips = get_kips()
    with open('../../program%s.bin' % target, 'rb') as f:
        data = f.read()
    erista_mtc = get_overlay(data, 1)
    mariko_mtc = get_overlay(data, 2)
    erista_hsh = hashlib.sha256(erista_mtc[:-4]).digest()[:4]
    mariko_hsh = hashlib.sha256(mariko_mtc[:-4]).digest()[:4]
    fusee_program = lz4_compress(data[:0x2B000 - 8] + erista_hsh + mariko_hsh + get_overlay(data, 0)[:0x11000])
    with open('../../program%s.lz4' % target, 'wb') as f:
        f.write(fusee_program)
    with open('../../fusee-boogaloo%s.bin' % target, 'wb') as f:
        # Write header
        write_header(f, all_kips)
        # Write warmboot
        f.write(pad(read_file('../../../../exosphere/warmboot%s.bin' % target), 0x1800))
        # Write TSEC Keygen
        f.write(pad(read_file('../../tsec_keygen/tsec_keygen.bin'), 0x2000))
        # Write Mariko Fatal
        f.write(pad(read_file('../../../../exosphere/mariko_fatal%s.bin' % target), 0x1C000))
        # Write Erista MTC
        f.write(erista_mtc[:-4] + erista_hsh)
        # Write Mariko MTC
        f.write(mariko_mtc[:-4] + mariko_hsh)
        # Write exosphere
        f.write(pad(read_file('../../../../exosphere/exosphere%s.bin' % target), 0xE000))
        # Write mesosphere
        f.write(pad(read_file('../../../../mesosphere/mesosphere%s.bin' % target), 0xAA000))
        # Write kips
        write_kips(f, all_kips)
        # Write Splash Screen
        f.write(pad(read_file('../../splash_screen/splash_screen.bin'), 0x3C0000))
        # Write fusee
        f.write(pad(fusee_program, 0x20000))
        # Write rebootstub
        f.write(pad(read_file('../../../../exosphere/program/rebootstub/rebootstub%s.bin' % target), 0x1000))
        # Pad to 8 MB
        f.write(b'\xCC' * (0x800000 - 0x7E1000))


    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
