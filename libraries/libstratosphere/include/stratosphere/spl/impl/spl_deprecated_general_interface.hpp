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
#include <stratosphere/sf.hpp>
#include <stratosphere/spl/spl_types.hpp>

namespace ams::spl::impl {

    #define AMS_SPL_I_DEPRECATED_GENERAL_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                     \
        AMS_SF_METHOD_INFO(C, H,  0, Result, GetConfig,                         (sf::Out<u64> out, u32 which))                                                                                                                                                                              \
        AMS_SF_METHOD_INFO(C, H,  1, Result, ModularExponentiate,               (const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod))                                                                         \
        AMS_SF_METHOD_INFO(C, H,  2, Result, GenerateAesKek,                    (sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option))                                                                                                                      \
        AMS_SF_METHOD_INFO(C, H,  3, Result, LoadAesKey,                        (s32 keyslot, AccessKey access_key, KeySource key_source))                                                                                                                                                  \
        AMS_SF_METHOD_INFO(C, H,  4, Result, GenerateAesKey,                    (sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source))                                                                                                                                      \
        AMS_SF_METHOD_INFO(C, H,  5, Result, SetConfig,                         (u32 which, u64 value))                                                                                                                                                                                     \
        AMS_SF_METHOD_INFO(C, H,  7, Result, GenerateRandomBytes,               (const sf::OutPointerBuffer &out))                                                                                                                                                                          \
        AMS_SF_METHOD_INFO(C, H,  9, Result, DecryptAndStoreGcKey,              (const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option))                                                                                                                   \
        AMS_SF_METHOD_INFO(C, H, 10, Result, DecryptGcMessage,                  (sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest))                                         \
        AMS_SF_METHOD_INFO(C, H, 11, Result, IsDevelopment,                     (sf::Out<bool> is_dev))                                                                                                                                                                                     \
        AMS_SF_METHOD_INFO(C, H, 12, Result, GenerateSpecificAesKey,            (sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which))                                                                                                                                 \
        AMS_SF_METHOD_INFO(C, H, 13, Result, DecryptDeviceUniqueData,           (const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option))                                                                                  \
        AMS_SF_METHOD_INFO(C, H, 14, Result, DecryptAesKey,                     (sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option))                                                                                                                                \
        AMS_SF_METHOD_INFO(C, H, 15, Result, ComputeCtrDeprecated,              (const sf::OutBuffer &out_buf, s32 keyslot, const sf::InBuffer &in_buf, IvCtr iv_ctr),                                                                              hos::Version_1_0_0, hos::Version_1_0_0) \
        AMS_SF_METHOD_INFO(C, H, 15, Result, ComputeCtr,                        (const sf::OutNonSecureBuffer &out_buf, s32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr),                                                            hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 16, Result, ComputeCmac,                       (sf::Out<Cmac> out_cmac, s32 keyslot, const sf::InPointerBuffer &in_buf))                                                                                                                                   \
        AMS_SF_METHOD_INFO(C, H, 17, Result, LoadEsDeviceKey,                   (const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option))                                                                                                                   \
        AMS_SF_METHOD_INFO(C, H, 18, Result, PrepareEsTitleKeyDeprecated,       (sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest),                      hos::Version_1_0_0, hos::Version_2_3_0) \
        AMS_SF_METHOD_INFO(C, H, 18, Result, PrepareEsTitleKey,                 (sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation),      hos::Version_3_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 19, Result, LoadPreparedAesKey,                (s32 keyslot, AccessKey access_key))                                                                                                                                                                        \
        AMS_SF_METHOD_INFO(C, H, 20, Result, PrepareCommonEsTitleKeyDeprecated, (sf::Out<AccessKey> out_access_key, KeySource key_source),                                                                                                          hos::Version_2_0_0, hos::Version_2_3_0) \
        AMS_SF_METHOD_INFO(C, H, 20, Result, PrepareCommonEsTitleKey,           (sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation),                                                                                          hos::Version_3_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 21, Result, AllocateAesKeySlot,                (sf::Out<s32> out_keyslot))                                                                                                                                                                                 \
        AMS_SF_METHOD_INFO(C, H, 22, Result, DeallocateAesKeySlot,              (s32 keyslot))                                                                                                                                                                                              \
        AMS_SF_METHOD_INFO(C, H, 23, Result, GetAesKeySlotAvailableEvent,       (sf::OutCopyHandle out_hnd))                                                                                                                                                                                \
        AMS_SF_METHOD_INFO(C, H, 24, Result, SetBootReason,                     (spl::BootReasonValue boot_reason),                                                                                                                                 hos::Version_3_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 25, Result, GetBootReason,                     (sf::Out<spl::BootReasonValue> out),                                                                                                                                hos::Version_3_0_0)

    AMS_SF_DEFINE_INTERFACE(IDeprecatedGeneralInterface, AMS_SPL_I_DEPRECATED_GENERAL_INTERFACE_INTERFACE_INFO)

}
