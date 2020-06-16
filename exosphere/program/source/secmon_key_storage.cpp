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
#include <exosphere.hpp>
#include "secmon_key_storage.hpp"

namespace ams::secmon {

    namespace {

        constexpr const u8 RsaPublicKey[] = { 0x00, 0x01, 0x00, 0x01 };

        constinit u8 g_rsa_moduli[ImportRsaKey_Count][se::RsaSize] = {};
        constinit bool g_rsa_modulus_committed[ImportRsaKey_Count] = {};

        ALWAYS_INLINE u8 *GetRsaKeyModulus(ImportRsaKey which) {
            return g_rsa_moduli[which];
        }

        ALWAYS_INLINE u8 *GetRsaKeyPrivateExponent(ImportRsaKey which) {
            return ::ams::secmon::impl::GetRsaPrivateExponentStorage(static_cast<int>(which));
        }

        ALWAYS_INLINE bool IsRsaKeyProvisional(ImportRsaKey which) {
            return g_rsa_modulus_committed[which] == false;
        }

        void ClearRsaKeyModulus(ImportRsaKey which) {
            g_rsa_modulus_committed[which] = false;
            std::memset(g_rsa_moduli[which], 0, se::RsaSize);
        }

        ALWAYS_INLINE u8 *GetMasterKeyStorage(int index) {
            return ::ams::secmon::impl::GetMasterKeyStorage(index);
        }

        ALWAYS_INLINE u8 *GetDeviceMasterKeyStorage(int index) {
            return ::ams::secmon::impl::GetDeviceMasterKeyStorage(index);
        }

    }

    void ImportRsaKeyExponent(ImportRsaKey which, const void *src, size_t size) {
        /* If we import an exponent, the modulus is not committed. */
        ClearRsaKeyModulus(which);

        /* Copy the exponent. */
        std::memcpy(GetRsaKeyPrivateExponent(which), src, size);
    }

    void ImportRsaKeyModulusProvisionally(ImportRsaKey which, const void *src, size_t size) {
        std::memcpy(GetRsaKeyModulus(which), src, std::min(static_cast<int>(size), se::RsaSize));
    }

    void CommitRsaKeyModulus(ImportRsaKey which) {
        g_rsa_modulus_committed[which] = true;
    }

    bool LoadRsaKey(int slot, ImportRsaKey which) {
        /* If the key is still provisional, we can't load it. */
        if (IsRsaKeyProvisional(which)) {
            return false;
        }

        se::SetRsaKey(slot, GetRsaKeyModulus(which), se::RsaSize, GetRsaKeyPrivateExponent(which), se::RsaSize);
        return true;
    }

    void LoadProvisionalRsaKey(int slot, ImportRsaKey which) {
        se::SetRsaKey(slot, GetRsaKeyModulus(which), se::RsaSize, GetRsaKeyPrivateExponent(which), se::RsaSize);
    }

    void LoadProvisionalRsaPublicKey(int slot, ImportRsaKey which) {
        se::SetRsaKey(slot, GetRsaKeyModulus(which), se::RsaSize, RsaPublicKey, sizeof(RsaPublicKey));
    }

    void SetMasterKey(int generation, const void *src, size_t size) {
        const int index = generation - pkg1::KeyGeneration_Min;
        se::EncryptAes128(GetMasterKeyStorage(index), se::AesBlockSize, pkg1::AesKeySlot_RandomForKeyStorageWrap, src, size);
    }

    void LoadMasterKey(int slot, int generation) {
        const int index = std::max(0, generation - pkg1::KeyGeneration_Min);
        se::SetEncryptedAesKey128(slot, pkg1::AesKeySlot_RandomForKeyStorageWrap, GetMasterKeyStorage(index), se::AesBlockSize);
    }

    void SetDeviceMasterKey(int generation, const void *src, size_t size) {
        const int index = generation - pkg1::KeyGeneration_4_0_0;
        se::EncryptAes128(GetDeviceMasterKeyStorage(index), se::AesBlockSize, pkg1::AesKeySlot_RandomForKeyStorageWrap, src, size);
    }

    void LoadDeviceMasterKey(int slot, int generation) {
        const int index = std::max(0, generation - pkg1::KeyGeneration_4_0_0);
        se::SetEncryptedAesKey128(slot, pkg1::AesKeySlot_RandomForKeyStorageWrap, GetDeviceMasterKeyStorage(index), se::AesBlockSize);
    }

}