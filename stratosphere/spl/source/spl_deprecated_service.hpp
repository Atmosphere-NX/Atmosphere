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
#include <stratosphere.hpp>

namespace ams::spl {

    class DeprecatedService final {
        public:
            virtual ~DeprecatedService();
        public:
            /* Actual commands. */
            Result GetConfig(sf::Out<u64> out, u32 which);
            Result ModularExponentiate(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod);
            Result GenerateAesKek(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option);
            Result LoadAesKey(s32 keyslot, AccessKey access_key, KeySource key_source);
            Result GenerateAesKey(sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source);
            Result SetConfig(u32 which, u64 value);
            Result GenerateRandomBytes(const sf::OutPointerBuffer &out);
            Result DecryptAndStoreGcKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            Result DecryptGcMessage(sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest);
            Result IsDevelopment(sf::Out<bool> is_dev);
            Result GenerateSpecificAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which);
            Result DecryptDeviceUniqueData(const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            Result DecryptAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option);
            Result ComputeCtrDeprecated(const sf::OutBuffer &out_buf, s32 keyslot, const sf::InBuffer &in_buf, IvCtr iv_ctr);
            Result ComputeCtr(const sf::OutNonSecureBuffer &out_buf, s32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr);
            Result ComputeCmac(sf::Out<Cmac> out_cmac, s32 keyslot, const sf::InPointerBuffer &in_buf);
            Result LoadEsDeviceKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option);
            Result PrepareEsTitleKeyDeprecated(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest);
            Result PrepareEsTitleKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation);
            Result LoadPreparedAesKey(s32 keyslot, AccessKey access_key);
            Result PrepareCommonEsTitleKeyDeprecated(sf::Out<AccessKey> out_access_key, KeySource key_source);
            Result PrepareCommonEsTitleKey(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation);
            Result AllocateAesKeySlot(sf::Out<s32> out_keyslot);
            Result DeallocateAesKeySlot(s32 keyslot);
            Result GetAesKeySlotAvailableEvent(sf::OutCopyHandle out_hnd);
            Result SetBootReason(BootReasonValue boot_reason);
            Result GetBootReason(sf::Out<BootReasonValue> out);
    };
    static_assert(spl::impl::IsIDeprecatedGeneralInterface<DeprecatedService>);

}
