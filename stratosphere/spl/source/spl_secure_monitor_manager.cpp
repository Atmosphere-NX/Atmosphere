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
#include <stratosphere.hpp>
#include "spl_secure_monitor_manager.hpp"

namespace ams::spl {

    void SecureMonitorManager::Initialize() {
        return impl::Initialize();
    }

    Result SecureMonitorManager::ModularExponentiate(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size) {
        return impl::ModularExponentiate(out, out_size, base, base_size, exp, exp_size, mod, mod_size);
    }

    Result SecureMonitorManager::GenerateAesKek(AccessKey *out_access_key, const KeySource &key_source, u32 generation, u32 option) {
        return impl::GenerateAesKek(out_access_key, key_source, generation, option);
    }

    Result SecureMonitorManager::LoadAesKey(s32 keyslot, const void *owner, const AccessKey &access_key, const KeySource &key_source) {
        R_TRY(this->TestAesKeySlot(nullptr, keyslot, owner));
        return impl::LoadAesKey(keyslot, access_key, key_source);
    }

    Result SecureMonitorManager::GenerateAesKey(AesKey *out_key, const AccessKey &access_key, const KeySource &key_source) {
        return impl::GenerateAesKey(out_key, access_key, key_source);
    }

    Result SecureMonitorManager::DecryptDeviceUniqueData(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
        return impl::DecryptDeviceUniqueData(dst, dst_size, src, src_size, access_key, key_source, option);
    }

    Result SecureMonitorManager::ReencryptDeviceUniqueData(void *dst, size_t dst_size, const void *src, size_t src_size, const AccessKey &access_key_dec, const KeySource &source_dec, const AccessKey &access_key_enc, const KeySource &source_enc, u32 option) {
        return impl::ReencryptDeviceUniqueData(dst, dst_size, src, src_size, access_key_dec, source_dec, access_key_enc, source_enc, option);
    }

    Result SecureMonitorManager::GetConfig(u64 *out, spl::ConfigItem key) {
        return impl::GetConfig(out, key);
    }

    Result SecureMonitorManager::SetConfig(spl::ConfigItem key, u64 value) {
        return impl::SetConfig(key, value);
    }

    Result SecureMonitorManager::GetPackage2Hash(void *dst, const size_t size) {
        return impl::GetPackage2Hash(dst, size);
    }

    Result SecureMonitorManager::GenerateRandomBytes(void *out, size_t size) {
        return impl::GenerateRandomBytes(out, size);
    }

    Result SecureMonitorManager::DecryptAndStoreGcKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
        return impl::DecryptAndStoreGcKey(src, src_size, access_key, key_source, option);
    }

    Result SecureMonitorManager::DecryptGcMessage(u32 *out_size, void *dst, size_t dst_size, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size) {
        return impl::DecryptGcMessage(out_size, dst, dst_size, base, base_size, mod, mod_size, label_digest, label_digest_size);
    }

    Result SecureMonitorManager::DecryptAndStoreSslClientCertKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source) {
        return impl::DecryptAndStoreSslClientCertKey(src, src_size, access_key, key_source);
    }

    Result SecureMonitorManager::ModularExponentiateWithSslClientCertKey(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size) {
        return impl::ModularExponentiateWithSslClientCertKey(out, out_size, base, base_size, mod, mod_size);
    }

    Result SecureMonitorManager::DecryptAndStoreDrmDeviceCertKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source) {
        return impl::DecryptAndStoreDrmDeviceCertKey(src, src_size, access_key, key_source);
    }

    Result SecureMonitorManager::ModularExponentiateWithDrmDeviceCertKey(void *out, size_t out_size, const void *base, size_t base_size, const void *mod, size_t mod_size) {
        return impl::ModularExponentiateWithDrmDeviceCertKey(out, out_size, base, base_size, mod, mod_size);
    }

    Result SecureMonitorManager::IsDevelopment(bool *out) {
        return impl::IsDevelopment(out);
    }

    Result SecureMonitorManager::GenerateSpecificAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 which) {
        return impl::GenerateSpecificAesKey(out_key, key_source, generation, which);
    }

    Result SecureMonitorManager::DecryptAesKey(AesKey *out_key, const KeySource &key_source, u32 generation, u32 option) {
        return impl::DecryptAesKey(out_key, key_source, generation, option);
    }

    Result SecureMonitorManager::ComputeCtr(void *dst, size_t dst_size, s32 keyslot, const void *owner, const void *src, size_t src_size, const IvCtr &iv_ctr) {
        R_TRY(this->TestAesKeySlot(nullptr, keyslot, owner));
        return impl::ComputeCtr(dst, dst_size, keyslot, src, src_size, iv_ctr);
    }

    Result SecureMonitorManager::ComputeCmac(Cmac *out_cmac, s32 keyslot, const void *owner, const void *data, size_t size) {
        R_TRY(this->TestAesKeySlot(nullptr, keyslot, owner));
        return impl::ComputeCmac(out_cmac, keyslot, data, size);
    }

    Result SecureMonitorManager::LoadEsDeviceKey(const void *src, size_t src_size, const AccessKey &access_key, const KeySource &key_source, u32 option) {
        return impl::LoadEsDeviceKey(src, src_size, access_key, key_source, option);
    }

    Result SecureMonitorManager::PrepareEsTitleKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation) {
        return impl::PrepareEsTitleKey(out_access_key, base, base_size, mod, mod_size, label_digest, label_digest_size, generation);
    }

    Result SecureMonitorManager::PrepareEsArchiveKey(AccessKey *out_access_key, const void *base, size_t base_size, const void *mod, size_t mod_size, const void *label_digest, size_t label_digest_size, u32 generation) {
        return impl::PrepareEsArchiveKey(out_access_key, base, base_size, mod, mod_size, label_digest, label_digest_size, generation);
    }

    Result SecureMonitorManager::PrepareCommonEsTitleKey(AccessKey *out_access_key, const KeySource &key_source, u32 generation) {
        return impl::PrepareCommonEsTitleKey(out_access_key, key_source, generation);
    }

    Result SecureMonitorManager::LoadPreparedAesKey(s32 keyslot, const void *owner, const AccessKey &access_key) {
        R_TRY(this->TestAesKeySlot(nullptr, keyslot, owner));
        return impl::LoadPreparedAesKey(keyslot, access_key);
    }

    Result SecureMonitorManager::AllocateAesKeySlot(s32 *out_keyslot, const void *owner) {
        /* Allocate a new virtual keyslot. */
        s32 keyslot;
        R_TRY(impl::AllocateAesKeySlot(std::addressof(keyslot)));

        /* Get the keyslot's index. */
        s32 index;
        bool virt;
        R_ABORT_UNLESS(impl::TestAesKeySlot(std::addressof(index), std::addressof(virt), keyslot));

        /* All allocated keyslots must be virtual. */
        AMS_ABORT_UNLESS(virt);

        m_aes_keyslot_owners[index] = owner;
        *out_keyslot                = keyslot;
        return ResultSuccess();
    }

    Result SecureMonitorManager::DeallocateAesKeySlot(s32 keyslot, const void *owner) {
        s32 index;
        R_TRY(this->TestAesKeySlot(std::addressof(index), keyslot, owner));

        m_aes_keyslot_owners[index] = nullptr;
        return impl::DeallocateAesKeySlot(keyslot);
    }

    void SecureMonitorManager::DeallocateAesKeySlots(const void *owner) {
        for (auto i = 0; i < impl::AesKeySlotCount; ++i) {
            if (m_aes_keyslot_owners[i] == owner) {
                m_aes_keyslot_owners[i] = nullptr;
                impl::DeallocateAesKeySlot(impl::AesKeySlotMin + i);
            }
        }
    }

    Result SecureMonitorManager::SetBootReason(BootReasonValue boot_reason) {
        return impl::SetBootReason(boot_reason);
    }

    Result SecureMonitorManager::GetBootReason(BootReasonValue *out) {
        return impl::GetBootReason(out);
    }

    os::SystemEvent *SecureMonitorManager::GetAesKeySlotAvailableEvent() {
        return impl::GetAesKeySlotAvailableEvent();
    }

    Result SecureMonitorManager::TestAesKeySlot(s32 *out_index, s32 keyslot, const void *owner) {
        /* Validate the keyslot (and get the index). */
        s32 index;
        bool virt;
        R_TRY(impl::TestAesKeySlot(std::addressof(index), std::addressof(virt), keyslot));

        /* Check that the keyslot is physical (for legacy compat) or owned by the request maker. */
        R_UNLESS(!virt || m_aes_keyslot_owners[index] == owner, spl::ResultInvalidKeySlot());

        /* Set output index. */
        if (out_index != nullptr) {
            *out_index = index;
        }
        return ResultSuccess();
    }


}
