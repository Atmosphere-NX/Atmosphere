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
#include <stratosphere/spl/spl_types.hpp>

namespace ams::spl {

    void Initialize();
    void InitializeForCrypto();
    void InitializeForSsl();
    void InitializeForEs();
    void InitializeForFs();
    void InitializeForManu();

    void Finalize();

    Result AllocateAesKeySlot(s32 *out_slot);
    Result DeallocateAesKeySlot(s32 slot);

    Result GenerateAesKek(AccessKey *access_key, const void *key_source, size_t key_source_size, s32 generation, u32 option);
    Result LoadAesKey(s32 slot, const AccessKey &access_key, const void *key_source, size_t key_source_size);
    Result GenerateAesKey(void *dst, size_t dst_size, const AccessKey &access_key, const void *key_source, size_t key_source_size);
    Result GenerateSpecificAesKey(void *dst, size_t dst_size, const void *key_source, size_t key_source_size, s32 generation, u32 option);
    Result ComputeCtr(void *dst, size_t dst_size, s32 slot, const void *src, size_t src_size, const void *iv, size_t iv_size);
    Result DecryptAesKey(void *dst, size_t dst_size, const void *src, size_t src_size, s32 generation, u32 option);

    Result GetConfig(u64 *out, ConfigItem item);
    bool IsDevelopment();
    MemoryArrangement GetMemoryArrangement();

    inline bool GetConfigBool(ConfigItem item) {
        u64 v;
        R_ABORT_UNLESS(::ams::spl::GetConfig(std::addressof(v), item));
        return v != 0;
    }

    inline HardwareType GetHardwareType() {
        u64 v;
        R_ABORT_UNLESS(::ams::spl::GetConfig(std::addressof(v), ::ams::spl::ConfigItem::HardwareType));
        return static_cast<HardwareType>(v);
    }

    inline HardwareState GetHardwareState() {
        u64 v;
        R_ABORT_UNLESS(::ams::spl::GetConfig(std::addressof(v), ::ams::spl::ConfigItem::HardwareState));
        return static_cast<HardwareState>(v);
    }

    inline u64 GetDeviceIdLow() {
        u64 v;
        R_ABORT_UNLESS(::ams::spl::GetConfig(std::addressof(v), ::ams::spl::ConfigItem::DeviceId));
        return v;
    }

    inline bool IsRecoveryBoot() {
        return ::ams::spl::GetConfigBool(::ams::spl::ConfigItem::IsRecoveryBoot);
    }

    inline bool IsDevelopmentFunctionEnabled() {
        return ::ams::spl::GetConfigBool(::ams::spl::ConfigItem::IsDevelopmentFunctionEnabled);
    }

    inline bool IsDisabledProgramVerification() {
        return ::ams::spl::GetConfigBool(::ams::spl::ConfigItem::DisableProgramVerification);
    }

    Result SetBootReason(BootReasonValue boot_reason);
    Result GetBootReason(BootReasonValue *out);

    inline BootReasonValue GetBootReason() {
        BootReasonValue br;
        R_ABORT_UNLESS(::ams::spl::GetBootReason(std::addressof(br)));
        return br;
    }

    SocType GetSocType();

    Result GetPackage2Hash(void *dst, size_t dst_size);
    Result GenerateRandomBytes(void *out, size_t buffer_size);

    Result LoadPreparedAesKey(s32 slot, const AccessKey &access_key);

}
