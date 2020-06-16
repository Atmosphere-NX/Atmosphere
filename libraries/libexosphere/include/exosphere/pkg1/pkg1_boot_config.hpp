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

    enum MemorySize {
        MemorySize_4GB = 0,
        MemorySize_6GB = 1,
        MemorySize_8GB = 2,
    };

    enum MemoryArrange {
        MemoryArrange_Normal    = 1,
        MemoryArrange_AppletDev = 2,
        MemoryArrange_SystemDev = 2,
    };

    enum MemoryMode {
        MemoryMode_SizeShift = 4,
        MemoryMode_SizeMask  = 0x30,

        MemoryMode_ArrangeMask = 0x0F,

        MemoryMode_Auto = 0x00,

        MemoryMode_4GB          = ((MemorySize_4GB << MemoryMode_SizeShift) | (MemoryArrange_Normal)),
        MemoryMode_4GBAppletDev = ((MemorySize_4GB << MemoryMode_SizeShift) | (MemoryArrange_AppletDev)),
        MemoryMode_4GBSystemDev = ((MemorySize_4GB << MemoryMode_SizeShift) | (MemoryArrange_SystemDev)),

        MemoryMode_6GB          = ((MemorySize_6GB << MemoryMode_SizeShift) | (MemoryArrange_Normal)),
        MemoryMode_6GBAppletDev = ((MemorySize_6GB << MemoryMode_SizeShift) | (MemoryArrange_AppletDev)),

        MemoryMode_8GB          = ((MemorySize_8GB << MemoryMode_SizeShift) | (MemoryArrange_Normal)),
    };

    constexpr ALWAYS_INLINE MemorySize GetMemorySize(MemoryMode mode) {
        return static_cast<MemorySize>(mode >> MemoryMode_SizeShift);
    }

    constexpr ALWAYS_INLINE MemoryArrange GetMemoryArrange(MemoryMode mode) {
        return static_cast<MemoryArrange>(mode & MemoryMode_ArrangeMask);
    }

    constexpr ALWAYS_INLINE MemoryMode MakeMemoryMode(MemorySize size, MemoryArrange arrange) {
        return static_cast<MemoryMode>((size << MemoryMode_SizeShift) | (arrange));
    }

    struct BootConfigData {
        u32 version;
        u32 reserved_04;
        u32 reserved_08;
        u32 reserved_0C;
        u8 flags1[0x10];
        u8 flags0[0x10];
        u64 initial_tsc_value;
        u8 padding_38[0x200 - 0x38];

        constexpr bool IsDevelopmentFunctionEnabled() const {
            return (this->flags1[0] & (1 << 1)) != 0;
        }

        constexpr bool IsSErrorDebugEnabled() const {
            return (this->flags1[0] & (1 << 2)) != 0;
        }

        constexpr u8 GetKernelFlags0() const {
            return this->flags0[1];
        }

        constexpr u8 GetKernelFlags1() const {
            return this->flags1[0];
        }

        constexpr MemoryMode GetMemoryMode() const {
            return static_cast<MemoryMode>(this->flags0[3]);
        }

        constexpr bool IsInitialTscValueValid() const {
            return (this->flags0[4] & (1 << 0)) != 0;
        }

        constexpr u64 GetInitialTscValue() const {
            return this->IsInitialTscValueValid() ? this->initial_tsc_value : 0;
        }
    };
    static_assert(util::is_pod<BootConfigData>::value);
    static_assert(sizeof(BootConfigData) == 0x200);

    struct BootConfigSignedData {
        u32 version;
        u32 reserved_04;
        u8  flags;
        u8  reserved_09[0x10 - 9];
        u8  ecid[0x10];
        u8  flags1[0x10];
        u8  flags0[0x10];
        u8  padding_40[0x100 - 0x40];

        constexpr bool IsPackage2EncryptionDisabled() const {
            return (this->flags & (1 << 0)) != 0;
        }

        constexpr bool IsPackage2SignatureVerificationDisabled() const {
            return (this->flags & (1 << 1)) != 0;
        }

        constexpr bool IsProgramVerificationDisabled() const {
            return (this->flags1[0] & (1 << 0)) != 0;
        }

        constexpr void SetPackage2SignatureVerificationDisabled(bool decrypted) {
            this->flags |= decrypted ? (1 << 1) : (0 << 0);
        }

        constexpr void SetPackage2EncryptionDisabled(bool decrypted) {
            this->flags |= decrypted ? (1 << 0) : (0 << 0);
        }
    };
    static_assert(util::is_pod<BootConfigSignedData>::value);
    static_assert(sizeof(BootConfigSignedData) == 0x100);

    struct BootConfig {
        BootConfigData data;
        u8 signature[0x100];
        BootConfigSignedData signed_data;
    };
    static_assert(util::is_pod<BootConfig>::value);
    static_assert(sizeof(BootConfig) == 0x400);

}
