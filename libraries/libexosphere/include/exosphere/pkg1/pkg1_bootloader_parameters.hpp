/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

namespace ams::pkg1 {

    enum BootloaderState {
        BootloaderState_Start            = 0,
        BootloaderState_LoadedBootConfig = 1,
        BootloaderState_InitializedDram  = 2,
        BootloaderState_LoadedPackage2   = 3,
        BootloaderState_Done             = 4,
    };

    enum SecureMonitorState {
        SecureMonitorState_Start       = 0,
        SecureMonitorState_Initialized = 1,
    };

    struct BctParameters {
        u32 bootloader_version;
        u32 bootloader_start_block;
        u32 bootloader_start_page;
        u32 bootloader_attributes;
    };
    static_assert(util::is_pod<BctParameters>::value && sizeof(BctParameters) == 0x10);

    struct SecureMonitorParameters {
        u32 bootloader_start_time;
        u32 bootloader_end_time;
        u32 secmon_start_time;
        u32 secmon_end_time;
        BctParameters bct_params;
        u32 deprecated_boot_reason_value;
        u8  deprecated_boot_reason_state;
        u8 reserved[0xD3];
        u32 bootloader_state;
        u32 secmon_state;
        u8 reserved2[0x100];
    };
    static_assert(util::is_pod<SecureMonitorParameters>::value);
    static_assert(sizeof(SecureMonitorParameters) == 0x200);

    static_assert(offsetof(SecureMonitorParameters, bct_params)       == 0x10);
    static_assert(offsetof(SecureMonitorParameters, bootloader_state) == 0xF8);
    static_assert(offsetof(SecureMonitorParameters, secmon_state) == 0xFC);

    enum BootloaderAttribute {
        BootloaderAttribute_None         = (0u << 0),
        BootloaderAttribute_RecoveryBoot = (1u << 0),

        BootloaderAttribute_RestrictedSmcShift = 1,
        BootloaderAttribute_RestrictedSmcMask  = (0xFu << BootloaderAttribute_RestrictedSmcShift),
    };

}
