#!/usr/bin/env python
import sys, lz4, hashlib, os
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
    return (data + b'\x00' * size)[:size]

def get_overlay(program, i):
    return program[0x2B000 + 0x14000 * i:0x2B000 + 0x14000 * (i+1)]

KIP_NAMES = [b'Loader', b'NCM', b'ProcessManager', b'sm', b'boot', b'spl', b'ams_mitm']

def get_kips(ams_dir, build_out_dir):
    emummc   = read_file(os.path.join(ams_dir, 'emummc/emummc_unpacked.kip'))
    loader   = read_file(os.path.join(ams_dir, 'stratosphere/loader/%s/loader.kip' % build_out_dir))
    ncm      = read_file(os.path.join(ams_dir, 'stratosphere/ncm/%s/ncm.kip' % build_out_dir))
    pm       = read_file(os.path.join(ams_dir, 'stratosphere/pm/%s/pm.kip' % build_out_dir))
    sm       = read_file(os.path.join(ams_dir, 'stratosphere/sm/%s/sm.kip' % build_out_dir))
    boot     = read_file(os.path.join(ams_dir, 'stratosphere/boot/%s/boot.kip' % build_out_dir))
    spl      = read_file(os.path.join(ams_dir, 'stratosphere/spl/%s/spl.kip' % build_out_dir))
    ams_mitm = read_file(os.path.join(ams_dir, 'stratosphere/ams_mitm/%s/ams_mitm.kip' % build_out_dir))
    return (emummc, {
        b'Loader' : loader,
        b'NCM' : ncm,
        b'ProcessManager' : pm,
        b'sm' : sm,
        b'boot' : boot,
        b'spl' : spl,
        b'ams_mitm' : ams_mitm,
    })

def write_kip_meta(f, kip, ofs):
    # Write program id
    f.write(kip[0x10:0x18])
    # Write offset, size
    f.write(pk('<II', ofs - 0x100000, len(kip)))
    # Write hash
    f.write(hashlib.sha256(kip).digest())

def write_header(f, all_kips, wb_size, tk_size, xf_size, ex_size, ms_size, fs_size, rb_size, git_revision, major, minor, micro, relstep, s_major, s_minor, s_micro, s_relstep):
    # Unpack kips
    emummc, kips = all_kips
    # Write magic as PK31 magic.
    f.write(b'PK31')
    # Write metadata offset = 0x10
    f.write(pk('<I', 0x20))
    # Write flags
    f.write(pk('<I', 0x00000000))
    # Write meso_size
    f.write(pk('<I', ms_size))
    # Write num_kips
    f.write(pk('<I', len(KIP_NAMES)))
    # Write reserved1
    f.write(b'\xCC' * 0xC)
    # Write legacy magic
    f.write(b'FSS0')
    # Write total size
    f.write(pk('<I', 0x800000))
    # Write reserved2
    f.write(pk('<I', 0xCCCCCCCC))
    # Write content_header_offset
    f.write(pk('<I', 0x40))
    # Write num_content_headers;
    f.write(pk('<I', 8 + len(KIP_NAMES)))
    # Write supported_hos_version;
    f.write(pk('<BBBB', s_relstep, s_micro, s_minor, s_major))
    # Write release_version;
    f.write(pk('<BBBB', relstep, micro, minor, major))
    # Write git_revision;
    f.write(pk('<I', git_revision))
    # Write content metas
    f.write(pk('<IIBBBBI16s', 0x000800, wb_size,  2, 0, 0, 0, 0xCCCCCCCC, b'warmboot'))
    f.write(pk('<IIBBBBI16s', 0x002000, tk_size, 12, 0, 0, 0, 0xCCCCCCCC, b'tsec_keygen'))
    f.write(pk('<IIBBBBI16s', 0x004000, xf_size, 11, 0, 0, 0, 0xCCCCCCCC, b'exosphere_fatal'))
    f.write(pk('<IIBBBBI16s', 0x048000, ex_size,  1, 0, 0, 0, 0xCCCCCCCC, b'exosphere'))
    f.write(pk('<IIBBBBI16s', 0x056000, ms_size, 10, 0, 0, 0, 0xCCCCCCCC, b'mesosphere'))
    f.write(pk('<IIBBBBI16s', 0x7C0000, fs_size,  0, 0, 0, 0, 0xCCCCCCCC, b'fusee'))
    f.write(pk('<IIBBBBI16s', 0x7E0000, rb_size,  3, 0, 0, 0, 0xCCCCCCCC, b'rebootstub'))
    f.write(pk('<IIBBBBI16s', 0x100000, len(emummc),  8, 0, 0, 0, 0xCCCCCCCC, b'emummc'))
    ofs = (0x100000 + len(emummc) + 0xF) & ~0xF
    for kip_name in KIP_NAMES:
        kip_data = kips[kip_name]
        f.write(pk('<IIBBBBI16s', ofs, len(kip_data), 6, 0, 0, 0, 0xCCCCCCCC, kip_name))
        ofs += len(kip_data)
        ofs += 0xF
        ofs &= ~0xF
    # Pad to kip metas.
    f.write(b'\xCC' * (0x400 - 0x40 - (0x20 * (8 + len(KIP_NAMES)))))
    # Write emummc_meta. */
    write_kip_meta(f, emummc, 0x100000)
    # Write kip metas
    ofs = (0x100000 + len(emummc) + 0xF) & ~0xF
    for kip_name in KIP_NAMES:
        kip_data = kips[kip_name]
        write_kip_meta(f, kip_data, ofs)
        ofs += len(kip_data)
        ofs += 0xF
        ofs &= ~0xF
    # Pad to end of header
    f.write(b'\xCC' * (0x800 - (0x400 + (1 + len(KIP_NAMES)) * 0x30)))

def write_kips(f, all_kips):
    # Unpack kips
    emummc, kips = all_kips
    # Write emummc
    f.write(emummc)
    # Write kips
    tot = len(emummc)
    if (tot & 0xF):
        f.write(b'\xCC' * (0x10 - (tot & 0xF)))
        tot += 0xF
        tot &= ~0xF
    for kip_name in KIP_NAMES:
        kip_data = kips[kip_name]
        f.write(kip_data)
        tot += len(kip_data)
        if (tot & 0xF):
            f.write(b'\xCC' * (0x10 - (tot & 0xF)))
            tot += 0xF
            tot &= ~0xF
    # Pad to 3 MB
    f.write(b'\xCC' * (0x300000 - tot))

def main(argc, argv):
    if argc != 13:
        print('Usage: %s ams_dir build_out_dir build_boot_out_dir revision major minor micro relstep s_major s_minor s_micro s_relstep' % argv[0])
        return 1
    # Parse arguments
    ams_dir            = argv[1]
    build_out_dir      = argv[2]
    build_boot_out_dir = argv[3]
    revision           = int(argv[4][:8], 16)
    major              = int(argv[5])
    minor              = int(argv[6])
    micro              = int(argv[7])
    relstep            = int(argv[8])
    s_major            = int(argv[9])
    s_minor            = int(argv[10])
    s_micro            = int(argv[11])
    s_relstep          = int(argv[12])
    # Read/parse fusee
    fusee_program = read_file(os.path.join(ams_dir, 'fusee/program/%s/program.bin' % build_boot_out_dir))
    fusee_bin     = read_file(os.path.join(ams_dir, 'fusee/%s/fusee.bin' % build_boot_out_dir))
    erista_mtc = get_overlay(fusee_program, 1)
    mariko_mtc = get_overlay(fusee_program, 2)
    erista_hsh = hashlib.sha256(erista_mtc[:-4]).digest()[:4]
    mariko_hsh = hashlib.sha256(mariko_mtc[:-4]).digest()[:4]
    # Read other files
    exosphere    = read_file(os.path.join(ams_dir, 'exosphere/%s/exosphere.bin' % build_out_dir))
    warmboot     = read_file(os.path.join(ams_dir, 'exosphere/warmboot/%s/warmboot.bin' % build_boot_out_dir))
    mariko_fatal = read_file(os.path.join(ams_dir, 'exosphere/mariko_fatal/%s/mariko_fatal.bin' % build_out_dir))
    rebootstub   = read_file(os.path.join(ams_dir, 'exosphere/program/rebootstub/%s/rebootstub.bin' % build_boot_out_dir))
    mesosphere   = read_file(os.path.join(ams_dir, 'mesosphere/%s/mesosphere.bin' % build_out_dir))
    all_kips     = get_kips(ams_dir, build_out_dir)
    tsec_keygen  = read_file(os.path.join(ams_dir, 'fusee/program/tsec_keygen/tsec_keygen.bin'))
    splash_bin   = read_file(os.path.join(ams_dir, 'img/splash.bin'))
    assert len(splash_bin) == 0x3C0000
    with open(os.path.join(ams_dir, 'fusee/%s/package3' % build_boot_out_dir), 'wb') as f:
        # Write header
        write_header(f, all_kips, len(warmboot), len(tsec_keygen), len(mariko_fatal), len(exosphere), len(mesosphere), len(fusee_bin), len(rebootstub), revision, major, minor, micro, relstep, s_major, s_minor, s_micro, s_relstep)
        # Write warmboot
        f.write(pad(warmboot, 0x1800))
        # Write TSEC Keygen
        f.write(pad(tsec_keygen, 0x2000))
        # Write Mariko Fatal
        f.write(pad(mariko_fatal, 0x1C000))
        # Write Erista MTC
        f.write(erista_mtc[:-4] + erista_hsh)
        # Write Mariko MTC
        f.write(mariko_mtc[:-4] + mariko_hsh)
        # Write exosphere
        f.write(pad(exosphere, 0xE000))
        # Write mesosphere
        f.write(pad(mesosphere, 0xAA000))
        # Write kips
        write_kips(f, all_kips)
        # Write Splash Screen
        f.write(splash_bin)
        # Write fusee
        f.write(pad(fusee_bin, 0x20000))
        # Write rebootstub
        f.write(pad(rebootstub, 0x1000))
        # Pad to 8 MB
        f.write(b'\xCC' * (0x800000 - 0x7E1000))


    return 0

if __name__ == '__main__':
    sys.exit(main(len(sys.argv), sys.argv))
