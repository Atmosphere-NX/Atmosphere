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
#include <stratosphere.hpp>

namespace ams::spl {

    HardwareType GetHardwareType() {
        u64 out_val = 0;
        R_ABORT_UNLESS(splGetConfig(SplConfigItem_HardwareType, &out_val));
        return static_cast<HardwareType>(out_val);
    }

    MemoryArrangement GetMemoryArrangement() {
        u64 arrange = 0;
        R_ABORT_UNLESS(splGetConfig(SplConfigItem_MemoryArrange, &arrange));
        arrange &= 0x3F;
        switch (arrange) {
            case 2:
                return MemoryArrangement_StandardForAppletDev;
            case 3:
                return MemoryArrangement_StandardForSystemDev;
            case 17:
                return MemoryArrangement_Expanded;
            case 18:
                return MemoryArrangement_ExpandedForAppletDev;
            default:
                return MemoryArrangement_Standard;
        }
    }

    bool IsDisabledProgramVerification() {
        u64 val = 0;
        R_ABORT_UNLESS(splGetConfig(SplConfigItem_DisableProgramVerification, &val));
        return val != 0;
    }

    bool IsDevelopmentHardware() {
        bool is_dev_hardware;
        R_ABORT_UNLESS(splIsDevelopment(&is_dev_hardware));
        return is_dev_hardware;
    }

    bool IsDevelopmentFunctionEnabled() {
        u64 val = 0;
        R_ABORT_UNLESS(splGetConfig(SplConfigItem_IsDebugMode, &val));
        return val != 0;
    }

    bool IsRecoveryBoot() {
        u64 val = 0;
        R_ABORT_UNLESS(splGetConfig(SplConfigItem_IsRecoveryBoot, &val));
        return val != 0;
    }

    bool IsMariko() {
        const auto hw_type = GetHardwareType();
        switch (hw_type) {
            case HardwareType::Icosa:
            case HardwareType::Copper:
                return false;
            case HardwareType::Hoag:
            case HardwareType::Iowa:
                return true;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result GenerateAesKek(AccessKey *access_key, const void *key_source, size_t key_source_size, u32 generation, u32 option) {
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        return splCryptoGenerateAesKek(key_source, generation, option, static_cast<void *>(access_key));
    }

    Result GenerateAesKey(void *dst, size_t dst_size, const AccessKey &access_key, const void *key_source, size_t key_source_size) {
        AMS_ASSERT(dst_size == crypto::AesEncryptor128::KeySize);
        AMS_ASSERT(key_source_size == sizeof(KeySource));
        return splCryptoGenerateAesKey(std::addressof(access_key), key_source, dst);
    }

}
