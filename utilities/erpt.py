#
# Copyright (c) Atmosphère-NX
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# erpt.py: Autogeneration tool for <stratosphere/erpt/erpt_ids.autogen.hpp>

import nxo64
import sys, os, string
from struct import unpack as up, pack as pk

LOAD_BASE = 0x7100000000

HEADER = '''/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <vapours.hpp>

/* NOTE: This file is auto-generated. */
/* Do not make edits to this file by hand. */

'''

if sys.version_info[0] == 3:
    iter_range = range
    int_types = (int,)
    ascii_string = lambda b: b.decode('ascii')
    bytes_to_list = lambda b: list(b)
    list_to_bytes = lambda l: bytes(l)
else:
    iter_range = xrange
    int_types = (int, long)
    ascii_string = lambda b: str(b)
    bytes_to_list = lambda b: map(ord, b)
    list_to_bytes = lambda l: ''.join(map(chr, l))

(DT_NULL, DT_NEEDED, DT_PLTRELSZ, DT_PLTGOT, DT_HASH, DT_STRTAB, DT_SYMTAB, DT_RELA, DT_RELASZ,
 DT_RELAENT, DT_STRSZ, DT_SYMENT, DT_INIT, DT_FINI, DT_SONAME, DT_RPATH, DT_SYMBOLIC, DT_REL,
 DT_RELSZ, DT_RELENT, DT_PLTREL, DT_DEBUG, DT_TEXTREL, DT_JMPREL, DT_BIND_NOW, DT_INIT_ARRAY,
 DT_FINI_ARRAY, DT_INIT_ARRAYSZ, DT_FINI_ARRAYSZ, DT_RUNPATH, DT_FLAGS) = iter_range(31)

DT_RELRSZ, DT_RELR, DT_RELRENT = 0x23, 0x24, 0x25

DT_GNU_HASH = 0x6ffffef5
DT_VERSYM = 0x6ffffff0
DT_RELACOUNT = 0x6ffffff9
DT_RELCOUNT = 0x6ffffffa
DT_FLAGS_1 = 0x6ffffffb
DT_VERDEF = 0x6ffffffc
DT_VERDEFNUM = 0x6ffffffd

STT_NOTYPE = 0
STT_OBJECT = 1
STT_FUNC = 2
STT_SECTION = 3

STB_LOCAL = 0
STB_GLOBAL = 1
STB_WEAK = 2

R_ARM_ABS32 = 2
R_ARM_TLS_DESC = 13
R_ARM_GLOB_DAT = 21
R_ARM_JUMP_SLOT = 22
R_ARM_RELATIVE = 23

R_FAKE_RELR = -1

R_AARCH64_ABS64 = 257
R_AARCH64_GLOB_DAT = 1025
R_AARCH64_JUMP_SLOT = 1026
R_AARCH64_RELATIVE = 1027
R_AARCH64_TLSDESC = 1031

CATEGORIES = {
    0   : 'Test',
    1   : 'ErrorInfo',
    2   : 'ConnectionStatusInfo',
    3   : 'NetworkInfo',
    4   : 'NXMacAddressInfo',
    5   : 'StealthNetworkInfo',
    6   : 'LimitHighCapacityInfo',
    7   : 'NATTypeInfo',
    8   : 'WirelessAPMacAddressInfo',
    9   : 'GlobalIPAddressInfo',
    10  : 'EnableWirelessInterfaceInfo',
    11  : 'EnableWifiInfo',
    12  : 'EnableBluetoothInfo',
    13  : 'EnableNFCInfo',
    14  : 'NintendoZoneSSIDListVersionInfo',
    15  : 'LANAdapterMacAddressInfo',
    16  : 'ApplicationInfo',
    17  : 'OccurrenceInfo',
    18  : 'ProductModelInfo',
    19  : 'CurrentLanguageInfo',
    20  : 'UseNetworkTimeProtocolInfo',
    21  : 'TimeZoneInfo',
    22  : 'ControllerFirmwareInfo',
    23  : 'VideoOutputInfo',
    24  : 'NANDFreeSpaceInfo',
    25  : 'SDCardFreeSpaceInfo',
    26  : 'ScreenBrightnessInfo',
    27  : 'AudioFormatInfo',
    28  : 'MuteOnHeadsetUnpluggedInfo',
    29  : 'NumUserRegisteredInfo',
    30  : 'DataDeletionInfo',
    31  : 'ControllerVibrationInfo',
    32  : 'LockScreenInfo',
    33  : 'InternalBatteryLotNumberInfo',
    34  : 'LeftControllerSerialNumberInfo',
    35  : 'RightControllerSerialNumberInfo',
    36  : 'NotificationInfo',
    37  : 'TVInfo',
    38  : 'SleepInfo',
    39  : 'ConnectionInfo',
    40  : 'NetworkErrorInfo',
    41  : 'FileAccessPathInfo',
    42  : 'GameCardCIDInfo',
    43  : 'NANDCIDInfo',
    44  : 'MicroSDCIDInfo',
    45  : 'NANDSpeedModeInfo',
    46  : 'MicroSDSpeedModeInfo',
    47  : 'GameCardSpeedModeInfo',
    48  : 'UserAccountInternalIDInfo',
    49  : 'NetworkServiceAccountInternalIDInfo',
    50  : 'NintendoAccountInternalIDInfo',
    51  : 'USB3AvailableInfo',
    52  : 'CallStackInfo',
    53  : 'SystemStartupLogInfo',
    54  : 'RegionSettingInfo',
    55  : 'NintendoZoneConnectedInfo',
    56  : 'ForceSleepInfo',
    57  : 'ChargerInfo',
    58  : 'RadioStrengthInfo',
    59  : 'ErrorInfoAuto',
    60  : 'AccessPointInfo',
    61  : 'ErrorInfoDefaults',
    62  : 'SystemPowerStateInfo',
    63  : 'PerformanceInfo',
    64  : 'ThrottlingInfo',
    65  : 'GameCardErrorInfo',
    66  : 'EdidInfo',
    67  : 'ThermalInfo',
    68  : 'CradleFirmwareInfo',
    69  : 'RunningApplicationInfo',
    70  : 'RunningAppletInfo',
    71  : 'FocusedAppletHistoryInfo',
    72  : 'CompositorInfo',
    73  : 'BatteryChargeInfo',
    74  : 'NANDExtendedCsd',
    75  : 'NANDPatrolInfo',
    76  : 'NANDErrorInfo',
    77  : 'NANDDriverLog',
    78  : 'SdCardSizeSpec',
    79  : 'SdCardErrorInfo',
    80  : 'SdCardDriverLog ',
    81  : 'FsProxyErrorInfo',
    82  : 'SystemAppletSceneInfo',
    83  : 'VideoInfo',
    84  : 'GpuErrorInfo',
    85  : 'PowerClockInfo',
    86  : 'AdspErrorInfo',
    87  : 'NvDispDeviceInfo',
    88  : 'NvDispDcWindowInfo',
    89  : 'NvDispDpModeInfo',
    90  : 'NvDispDpLinkSpec',
    91  : 'NvDispDpLinkStatus',
    92  : 'NvDispDpHdcpInfo',
    93  : 'NvDispDpAuxCecInfo',
    94  : 'NvDispDcInfo',
    95  : 'NvDispDsiInfo',
    96  : 'NvDispErrIDInfo',
    97  : 'SdCardMountInfo',
    98  : 'RetailInteractiveDisplayInfo',
    99  : 'CompositorStateInfo',
    100 : 'CompositorLayerInfo',
    101 : 'CompositorDisplayInfo',
    102 : 'CompositorHWCInfo',
    103 : 'MonitorCapability',
    104 : 'ErrorReportSharePermissionInfo',
    105 : 'MultimediaInfo',
    106 : 'ConnectedControllerInfo',
    107 : 'FsMemoryInfo',
    108 : 'UserClockContextInfo',
    109 : 'NetworkClockContextInfo',
    110 : 'AcpGeneralSettingsInfo',
    111 : 'AcpPlayLogSettingsInfo',
    112 : 'AcpAocSettingsInfo',
    113 : 'AcpBcatSettingsInfo',
    114 : 'AcpStorageSettingsInfo',
    115 : 'AcpRatingSettingsInfo',
    116 : 'MonitorSettings',
    117 : 'RebootlessSystemUpdateVersionInfo',
    118 : 'NifmConnectionTestInfo',
    119 : 'PcieLoggedStateInfo',
    120 : 'NetworkSecurityCertificateInfo',
    121 : 'AcpNeighborDetectionInfo',
    122 : 'GpuCrashInfo',
    123 : 'UsbStateInfo',
    124 : 'NvHostErrInfo',
    125 : 'RunningUlaInfo',
    126 : 'InternalPanelInfo',
    127 : 'ResourceLimitLimitInfo',
    128 : 'ResourceLimitPeakInfo',
    129 : 'TouchScreenInfo',
    130 : 'AcpUserAccountSettingsInfo',
    131 : 'AudioDeviceInfo',
    132 : 'AbnormalWakeInfo',
    133 : 'ServiceProfileInfo',
    134 : 'BluetoothAudioInfo',
    135 : 'BluetoothPairingCountInfo',
    136 : 'FsProxyErrorInfo2',
    137 : 'BuiltInWirelessOUIInfo',
    138 : 'WirelessAPOUIInfo',
    139 : 'EthernetAdapterOUIInfo',
    140 : 'NANDTypeInfo',
    141 : 'MicroSDTypeInfo',
    142 : 'AttachmentFileInfo',
    143 : 'WlanInfo',
    144 : 'HalfAwakeStateInfo',
    145 : 'PctlSettingInfo',
    146 : 'GameCardLogInfo',
    147 : 'WlanIoctlErrorInfo',
    148 : 'SdCardActivationInfo',
    149 : 'GameCardDetailedErrorInfo',
    1000 : 'TestNx',
    1001 : 'NANDTypeInfo',
    1002 : 'NANDExtendedCsd',
}

FIELD_TYPES = {
    0  : 'FieldType_NumericU64',
    1  : 'FieldType_NumericU32',
    2  : 'FieldType_NumericI64',
    3  : 'FieldType_NumericI32',
    4  : 'FieldType_String',
    5  : 'FieldType_U8Array',
    6  : 'FieldType_U32Array',
    7  : 'FieldType_U64Array',
    8  : 'FieldType_I32Array',
    9  : 'FieldType_I64Array',
    10 : 'FieldType_Bool',
    11 : 'FieldType_NumericU16',
    12 : 'FieldType_NumericU8',
    13 : 'FieldType_NumericI16',
    14 : 'FieldType_NumericI8',
    15 : 'FieldType_I8Array',
}

FIELD_FLAGS = {
    0 : 'FieldFlag_None',
    1 : 'FieldFlag_Encrypt',
}

def get_full(nxo):
    full = nxo.full[:]

    undef_count = 0
    for s in nxo.symbols:
        if not s.shndx and s.name:
            undef_count += 1
    last_ea = max(LOAD_BASE + end for start, end, name, kind in nxo.sections)
    undef_entry_size = 8
    undef_ea = ((last_ea + 0xFFF) & ~0xFFF) + undef_entry_size # plus 8 so we don't end up on the "end" symbol

    for i,s in enumerate(nxo.symbols):
        if not s.shndx and s.name:
            #idaapi.create_data(undef_ea, idc.FF_QWORD, 8, idaapi.BADADDR)
            #idaapi.force_name(undef_ea, s.name)
            s.resolved = undef_ea
            undef_ea += undef_entry_size
        elif i != 0:
            assert s.shndx
            s.resolved = LOAD_BASE + s.value
            if s.name:
                if s.type == STT_FUNC:
                    #print(hex(s.resolved), s.name)
                    #idaapi.add_entry(s.resolved, s.resolved, s.name, 0)
                    pass
                else:
                    #idaapi.force_name(s.resolved, s.name)
                    pass
        else:
            # NULL symbol
            s.resolved = 0

    def put_dword(z, target, val):
        return z[:target] + pk('<I', val) + z[target+4:]
    def put_qword(z, target, val):
        return z[:target] + pk('<Q', val) + z[target+8:]
    def get_dword(z, target):
        return up('<I', z[target:target+4])[0]
    def get_qword(z, target):
        return up('<Q', z[target:target+8])[0]

    for offset, r_type, sym, addend in nxo.relocations:
        #print offset, r_type, sym, addend
        target = offset + LOAD_BASE
        if r_type in (R_ARM_GLOB_DAT, R_ARM_JUMP_SLOT, R_ARM_ABS32):
            if not sym:
                print('error: relocation at %X failed' % target)
            else:
                full = put_dword(full, target, sym.resolved)
        elif r_type == R_ARM_RELATIVE:
            full = put_dword(full, target, get_dword(full, target) + LOAD_BASE)
        elif r_type in (R_AARCH64_GLOB_DAT, R_AARCH64_JUMP_SLOT, R_AARCH64_ABS64):
            full = put_qword(full, target, sym.resolved + addend)
        elif r_type == R_AARCH64_RELATIVE:
            full = put_qword(full, target, LOAD_BASE + addend)
        elif r_type == R_FAKE_RELR:
            addend = get_qword(full, offset)
            #print '%X %X %x' % (offset, target, addend)
            full = put_qword(full, offset, addend + LOAD_BASE)
        else:
            print('TODO r_type %d' % (r_type,))
    with open('E:\\full.bin', 'wb') as f:
        f.write(full)
    return full

def locate_fields(full):
    start = ['TestU64', 'TestU32', 'TestI64', 'TestI32']
    inds = [full.index('%s\x00' % s) for s in start]
    target = pk('<QQQQ', LOAD_BASE + inds[0], LOAD_BASE + inds[1], LOAD_BASE + inds[2], LOAD_BASE + inds[3])
    if target in full:
        return 0, full.index(target)
    else:
        # 17.0.0
        ofs = 0
        while ofs < len(full) - 0x10:
            test = full[ofs:ofs + 0x10]
            if test == pk('<IIII', *[((x - ofs) & 0xFFFFFFFF) for x in inds]):
                return 1, ofs
            ofs += 4

def read_string(full, ofs):
    s = ''
    if ofs >= len(full):
        return ''
    while full[ofs] != '\x00':
        s += full[ofs]
        ofs += 1
        if ofs >= len(full):
            return ''
    return s

def is_valid_field_name(s):
    ALLOWED = string.lowercase + string.uppercase + string.digits + '_'
    if not s:
        return False
    for c in s:
        if not c in ALLOWED:
            return False
    return True

def parse_fields(full, table, format_version):
    fields = []
    ofs = 0
    if format_version == 0:
        while True:
            val = up('<Q', full[table + ofs:table + ofs + 8])[0]
            if (val & 0xFFFFFFFF00000000) != LOAD_BASE:
                break
            s = read_string(full, val - LOAD_BASE)
            if not is_valid_field_name(s):
                break
            fields.append(s)
            ofs += 8
    elif format_version == 1:
        while True:
            val = up('<I', full[table + ofs:table + ofs + 4])[0]
            s = read_string(full, (table + val) & 0xFFFFFFFF)
            if not is_valid_field_name(s):
                break
            fields.append(s)
            ofs += 4
    return fields

def find_categories(full, num_fields):
    KNOWN = [0] * 10 + [1] * 2 + [0x3B] * 2
    ind = full.index(''.join(pk('<I', i) for i in KNOWN))
    return list(up('<'+'I'*num_fields, full[ind:ind+4*num_fields]))

def find_types(full, num_fields):
    KNOWN     = range(10) + [4, 4, 2, 4]
    KNOWN_OLD = range(10) + [4, 4, 0, 4]
    try:
        ind = full.index(''.join(pk('<I', i) for i in KNOWN))
    except ValueError:
        ind = full.index(''.join(pk('<I', i) for i in KNOWN_OLD))
    return list(up('<'+'I'*num_fields, full[ind:ind+4*num_fields]))

def find_flags(full, num_fields, magic_idx):
    KNOWN = '\x00' + ('\x01'*6) + '\x00\x01\x01\x00'
    if num_fields < magic_idx + len(KNOWN):
        return [0] * num_fields
    ind = full.index(KNOWN) - magic_idx
    return list(up('<'+'B'*num_fields, full[ind:ind+num_fields]))

def find_id_array(full, num_fields, magic_idx, table_format):
    if table_format == 0:
        return list(range(num_fields))
    else:
        KNOWN = pk('<IIIIII', *range(444, 450))
        ind = full.index(KNOWN) - 4 * magic_idx
        return list(up('<' + 'I'*num_fields, full[ind:ind+4*num_fields]))

def cat_to_string(c):
    return CATEGORIES[c] if c in CATEGORIES else 'Category_Unknown%d' % c

def typ_to_string(t):
    return FIELD_TYPES[t] if t in FIELD_TYPES else 'FieldType_Unknown%d' % t

def flg_to_string(f):
    return FIELD_FLAGS[f] if f in FIELD_FLAGS else 'FieldFlag_Unknown%d' % f

def main(argc, argv):
    if argc != 2 and not (argc == 3 and argv[1] == '-f'):
        print 'Usage: %s [-f] erpt_nso' % argv[0]
        return 1
    f = open(argv[-1], 'rb')
    nxo = nxo64.load_nxo(f)
    full = get_full(nxo)
    table_format, field_table = locate_fields(full)
    fields = parse_fields(full, field_table, table_format)
    NUM_FIELDS = len(fields)
    cats = find_categories(full, NUM_FIELDS)
    types = find_types(full, NUM_FIELDS)
    flags = find_flags(full, NUM_FIELDS, fields.index('TestStringEncrypt') - 1)
    ids   = find_id_array(full, NUM_FIELDS, fields.index('TestStringEncrypt'), table_format)
    assert ids[:4] == [0, 1, 2, 3]
    print 'Identified %d fields.' % NUM_FIELDS
    mf = max(len(s) for s in fields)
    mc = max(len(cat_to_string(c)) for c in cats)
    mt = max(len(typ_to_string(t)) for t in types)
    ml = max(len(flg_to_string(f)) for f in flags)
    if argc == 3:
        mf, mc, mt, ml = (64, 48, 32, 32)
    format_string = '- %%-%ds %%-%ds %%-%ds %%-%ds' % (mf+1, mc+1, mt+1, ml)
    for i in xrange(NUM_FIELDS):
        f, c, t, l = fields[i], cat_to_string(cats[i]), typ_to_string(types[i]), flg_to_string(flags[i])
        print format_string % (f+',', c+',', t+',', l)
    with open(argv[-1]+'.hpp', 'w') as out:
        out.write(HEADER)
        out.write('#define AMS_ERPT_FOREACH_FIELD_TYPE(HANDLER) \\\n')
        for tp in sorted(list(set(types + FIELD_TYPES.keys()))):
            out.write(('    HANDLER(%%-%ds %%-2d) \\\n' % (mt+1)) % (typ_to_string(tp)+',', tp))
        out.write('\n')
        out.write('#define AMS_ERPT_FOREACH_CATEGORY(HANDLER) \\\n')
        for ct in sorted(list(set(cats + CATEGORIES.keys()))):
            out.write(('    HANDLER(%%-%ds %%-3d) \\\n' % (mc+1)) % (cat_to_string(ct)+',', ct))
        out.write('\n')
        out.write('#define AMS_ERPT_FOREACH_FIELD(HANDLER) \\\n')
        for i in xrange(NUM_FIELDS):
            f, c, t, l, d = fields[i], cats[i], types[i], flags[i], ids[i]
            out.write(('    HANDLER(%%-%ds %%-4s %%-%ds %%-%ds %%-%ds) \\\n' % (mf+1, mc+1, mt+1, ml)) % (f+',', '%d,'%d, cat_to_string(c)+',', typ_to_string(t)+',', flg_to_string(l)))
        out.write('\n')
    return 0

if __name__ == '__main__':
    try:
        ret = main(len(sys.argv), sys.argv)
    except Exception as e:
        print e
        ret = 1
        print 'exception'
    sys.exit(ret)