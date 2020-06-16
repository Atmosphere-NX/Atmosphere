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
#include <exosphere/pkg2.hpp>

namespace ams::secmon {

    /* The VolatileStack page is reserved entirely for use for core 3 SMC handling. */
    constexpr inline const Address Core3SmcStackAddress = MemoryRegionVirtualTzramVolatileStack.GetAddress() + MemoryRegionVirtualTzramVolatileStack.GetSize();

    constexpr inline const size_t CoreExceptionStackSize = 0x80;

    /* Volatile keydata that we lose access to after boot. */
    struct VolatileKeys {
        u8 boot_config_rsa_modulus[0x100];
        u8 package2_dev_rsa_modulus[0x100];
        u8 package2_prod_rsa_modulus[0x100];
        u8 package2_aes_key[0x10];
    };

    /* Nintendo uses the bottom 0x740 of this as a stack for warmboot setup, and another 0x740 for the core 0/1/2 SMC stacks. */
    /* This is...wasteful. The warmboot stack is not deep. We will thus save 1K+ of nonvolatile storage by keeping the random cache in here. */
    struct VolatileData {
        union {
            u8 random_cache[0x400];
            VolatileKeys keys;
            pkg2::Package2Meta pkg2_meta;
        };
        u8 se_work_block[crypto::AesEncryptor128::BlockSize];
        u8 reserved_danger_zone[0x30]; /* This memory is "available", but careful consideration must be taken before declaring it used. */
        u8 warmboot_stack[0x380];
        u8 core012_smc_stack[0x6C0];
        u8 core_exception_stacks[3][CoreExceptionStackSize];
    };
    static_assert(util::is_pod<VolatileData>::value);
    static_assert(sizeof(VolatileData) == 0x1000);
    static_assert(sizeof(VolatileKeys{}.boot_config_rsa_modulus) == sizeof(pkg2::Package2Meta));
    static_assert(offsetof(VolatileData, keys.boot_config_rsa_modulus) == offsetof(VolatileData, pkg2_meta));

    ALWAYS_INLINE VolatileData &GetVolatileData() {
        return *MemoryRegionVirtualTzramVolatileData.GetPointer<VolatileData>();
    }

    ALWAYS_INLINE u8 *GetRandomBytesCache() {
        return GetVolatileData().random_cache;
    }

    constexpr ALWAYS_INLINE size_t GetRandomBytesCacheSize() {
        return sizeof(VolatileData::random_cache);
    }

    ALWAYS_INLINE u8 *GetSecurityEngineEphemeralWorkBlock() {
        return GetVolatileData().se_work_block;
    }

    namespace boot {

        ALWAYS_INLINE const u8 *GetBootConfigRsaModulus() {
            return GetVolatileData().keys.boot_config_rsa_modulus;
        }

        ALWAYS_INLINE const u8 *GetPackage2RsaModulus(bool is_prod) {
            auto &volatile_data = GetVolatileData();
            return is_prod ? volatile_data.keys.package2_prod_rsa_modulus : volatile_data.keys.package2_dev_rsa_modulus;
        }

        ALWAYS_INLINE const u8 *GetPackage2AesKey() {
            return GetVolatileData().keys.package2_aes_key;
        }

        ALWAYS_INLINE pkg2::Package2Meta &GetEphemeralPackage2Meta() {
            return GetVolatileData().pkg2_meta;
        }

    }

    constexpr inline const Address WarmbootStackAddress   = MemoryRegionVirtualTzramVolatileData.GetAddress() + offsetof(VolatileData, warmboot_stack)    + sizeof(VolatileData::warmboot_stack);
    constexpr inline const Address Core012SmcStackAddress = MemoryRegionVirtualTzramVolatileData.GetAddress() + offsetof(VolatileData, core012_smc_stack) + sizeof(VolatileData::core012_smc_stack);

    constexpr inline const Address Core0ExceptionStackAddress = MemoryRegionVirtualTzramVolatileData.GetAddress() + offsetof(VolatileData, core_exception_stacks) + CoreExceptionStackSize;
    constexpr inline const Address Core1ExceptionStackAddress = Core0ExceptionStackAddress + CoreExceptionStackSize;
    constexpr inline const Address Core2ExceptionStackAddress = Core1ExceptionStackAddress + CoreExceptionStackSize;

}
