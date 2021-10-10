/*
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
#include <stratosphere.hpp>
#include "spl_secure_monitor_manager.hpp"

namespace ams::spl {

    class DeprecatedService {
        protected:
            SecureMonitorManager &m_manager;
        public:
            explicit DeprecatedService(SecureMonitorManager *manager) : m_manager(*manager) { /* ... */ }
        public:
            virtual ~DeprecatedService() {
                /* Free any keyslots this service is using. */
                m_manager.DeallocateAesKeySlots(this);
            }
        public:
            /* Actual commands. */
            Result GetConfig(sf::Out<u64> out, u32 which) {
                return m_manager.GetConfig(out.GetPointer(), static_cast<spl::ConfigItem>(which));
            }

            Result ModularExponentiate(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod) {
                return m_manager.ModularExponentiate(out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), exp.GetPointer(), exp.GetSize(), mod.GetPointer(), mod.GetSize());
            }

            Result GenerateAesKek(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option) {
                return m_manager.GenerateAesKek(out_access_key.GetPointer(), key_source, generation, option);
            }

            Result LoadAesKey(s32 keyslot, AccessKey access_key, KeySource key_source) {
                return m_manager.LoadAesKey(keyslot, this, access_key, key_source);
            }

            Result GenerateAesKey(sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source) {
                return m_manager.GenerateAesKey(out_key.GetPointer(), access_key, key_source);
            }

            Result SetConfig(u32 which, u64 value) {
                return m_manager.SetConfig(static_cast<spl::ConfigItem>(which), value);
            }

            Result GenerateRandomBytes(const sf::OutPointerBuffer &out) {
                return m_manager.GenerateRandomBytes(out.GetPointer(), out.GetSize());
            }

            Result DecryptAndStoreGcKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
                return m_manager.DecryptAndStoreGcKey(src.GetPointer(), src.GetSize(), access_key, key_source, option);
            }

            Result DecryptGcMessage(sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest) {
                return m_manager.DecryptGcMessage(out_size.GetPointer(), out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize());
            }

            Result IsDevelopment(sf::Out<bool> is_dev) {
                return m_manager.IsDevelopment(is_dev.GetPointer());
            }

            Result GenerateSpecificAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which) {
                return m_manager.GenerateSpecificAesKey(out_key.GetPointer(), key_source, generation, which);
            }

            Result DecryptDeviceUniqueData(const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
                return m_manager.DecryptDeviceUniqueData(dst.GetPointer(), dst.GetSize(), src.GetPointer(), src.GetSize(), access_key, key_source, option);
            }

            Result DecryptAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option) {
                return m_manager.DecryptAesKey(out_key.GetPointer(), key_source, generation, option);
            }

            Result ComputeCtrDeprecated(const sf::OutBuffer &out_buf, s32 keyslot, const sf::InBuffer &in_buf, IvCtr iv_ctr) {
                return m_manager.ComputeCtr(out_buf.GetPointer(), out_buf.GetSize(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize(), iv_ctr);
            }

            Result ComputeCtr(const sf::OutNonSecureBuffer &out_buf, s32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr) {
                return m_manager.ComputeCtr(out_buf.GetPointer(), out_buf.GetSize(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize(), iv_ctr);
            }

            Result ComputeCmac(sf::Out<Cmac> out_cmac, s32 keyslot, const sf::InPointerBuffer &in_buf) {
                return m_manager.ComputeCmac(out_cmac.GetPointer(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize());
            }

            Result LoadEsDeviceKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
                return m_manager.LoadEsDeviceKey(src.GetPointer(), src.GetSize(), access_key, key_source, option);
            }

            Result PrepareEsTitleKeyDeprecated(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest) {
                return m_manager.PrepareEsTitleKey(out_access_key.GetPointer(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize(), 0);
            }

            Result PrepareEsTitleKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation) {
                return m_manager.PrepareEsTitleKey(out_access_key.GetPointer(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize(), generation);
            }

            Result LoadPreparedAesKey(s32 keyslot, AccessKey access_key) {
                return m_manager.LoadPreparedAesKey(keyslot, this, access_key);
            }

            Result PrepareCommonEsTitleKeyDeprecated(sf::Out<AccessKey> out_access_key, KeySource key_source) {
                return m_manager.PrepareCommonEsTitleKey(out_access_key.GetPointer(), key_source, 0);
            }

            Result PrepareCommonEsTitleKey(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation) {
                return m_manager.PrepareCommonEsTitleKey(out_access_key.GetPointer(), key_source, generation);
            }

            Result AllocateAesKeySlot(sf::Out<s32> out_keyslot) {
                return m_manager.AllocateAesKeySlot(out_keyslot.GetPointer(), this);
            }

            Result DeallocateAesKeySlot(s32 keyslot) {
                return m_manager.DeallocateAesKeySlot(keyslot, this);
            }

            Result GetAesKeySlotAvailableEvent(sf::OutCopyHandle out_hnd) {
                out_hnd.SetValue(m_manager.GetAesKeySlotAvailableEvent()->GetReadableHandle(), false);
                return ResultSuccess();
            }

            Result SetBootReason(BootReasonValue boot_reason) {
                return m_manager.SetBootReason(boot_reason);
            }

            Result GetBootReason(sf::Out<BootReasonValue> out) {
                return m_manager.GetBootReason(out.GetPointer());
            }
    };
    static_assert(spl::impl::IsIDeprecatedGeneralInterface<DeprecatedService>);

}
