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
#include <exosphere/pkg1.hpp>
#include <exosphere/se.hpp>
#include <exosphere/secmon/secmon_monitor_context.hpp>

namespace ams::secmon {

    struct ConfigurationContext {
        union {
            SecureMonitorConfiguration secmon_cfg;
            u8 _raw_exosphere_config[0x80];
        };
        union {
            EmummcConfiguration emummc_cfg;
            u8 _raw_emummc_config[0x120];
        };
        u8 sealed_device_keys[pkg1::KeyGeneration_Max][se::AesBlockSize];
        u8 sealed_master_keys[pkg1::KeyGeneration_Max][se::AesBlockSize];
        pkg1::BootConfig boot_config;
        u8 rsa_private_exponents[4][se::RsaSize];
        union {
            u8 _misc_data[0xFC0 - sizeof(_raw_exosphere_config) - sizeof(_raw_emummc_config) - sizeof(sealed_device_keys) - sizeof(sealed_master_keys) - sizeof(boot_config) - sizeof(rsa_private_exponents)];
        };
        /* u8 l1_page_table[0x40]; */
    };
    static_assert(sizeof(ConfigurationContext) == 0xFC0);
    static_assert(util::is_pod<ConfigurationContext>::value);

    namespace impl {

        ALWAYS_INLINE uintptr_t GetConfigurationContextAddress() {
            register uintptr_t x18 asm("x18");
            __asm__ __volatile__("" : [x18]"=r"(x18));
            return x18;
        }

        ALWAYS_INLINE ConfigurationContext &GetConfigurationContext() {
            return *reinterpret_cast<ConfigurationContext *>(GetConfigurationContextAddress());
        }

        ALWAYS_INLINE u8 *GetMasterKeyStorage(int generation) {
            return GetConfigurationContext().sealed_master_keys[generation];
        }

        ALWAYS_INLINE u8 *GetDeviceMasterKeyStorage(int generation) {
            return GetConfigurationContext().sealed_device_keys[generation];
        }

        ALWAYS_INLINE u8 *GetRsaPrivateExponentStorage(int which) {
            return GetConfigurationContext().rsa_private_exponents[which];
        }

        ALWAYS_INLINE void SetKeyGeneration(int generation) {
            GetConfigurationContext().secmon_cfg.key_generation = generation;
        }

        ALWAYS_INLINE pkg1::BootConfig *GetBootConfigStorage() {
            return std::addressof(GetConfigurationContext().boot_config);
        }

    }

    ALWAYS_INLINE const ConfigurationContext &GetConfigurationContext() {
        return *reinterpret_cast<const ConfigurationContext *>(impl::GetConfigurationContextAddress());
    }

    ALWAYS_INLINE const SecureMonitorConfiguration &GetSecmonConfiguration() {
        return GetConfigurationContext().secmon_cfg;
    }

    ALWAYS_INLINE const EmummcConfiguration &GetEmummcConfiguration() {
        return GetConfigurationContext().emummc_cfg;
    }

    ALWAYS_INLINE const pkg1::BootConfig &GetBootConfig() {
        return GetConfigurationContext().boot_config;
    }

    ALWAYS_INLINE ams::TargetFirmware GetTargetFirmware() {
        return GetSecmonConfiguration().GetTargetFirmware();
    }

    ALWAYS_INLINE int GetKeyGeneration() {
        return GetSecmonConfiguration().GetKeyGeneration();
    }

    ALWAYS_INLINE fuse::HardwareType GetHardwareType() {
        return GetSecmonConfiguration().GetHardwareType();
    }

    ALWAYS_INLINE fuse::SocType GetSocType() {
        return GetSecmonConfiguration().GetSocType();
    }

    ALWAYS_INLINE fuse::HardwareState GetHardwareState() {
        return GetSecmonConfiguration().GetHardwareState();
    }

    ALWAYS_INLINE u16 GetLcdVendor() {
        return GetSecmonConfiguration().GetLcdVendor();
    }

    ALWAYS_INLINE uart::Port GetLogPort() {
        return GetSecmonConfiguration().GetLogPort();
    }

    ALWAYS_INLINE u8 GetLogFlags() {
        return GetSecmonConfiguration().GetLogFlags();
    }

    ALWAYS_INLINE u32 GetLogBaudRate() {
        return GetSecmonConfiguration().GetLogBaudRate();
    }

    ALWAYS_INLINE bool IsProduction() {
        return GetSecmonConfiguration().IsProduction();
    }

}
