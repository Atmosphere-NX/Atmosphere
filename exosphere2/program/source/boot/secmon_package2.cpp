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
#include "../secmon_error.hpp"
#include "../secmon_key_storage.hpp"
#include "secmon_boot.hpp"

namespace ams::secmon::boot {

    bool VerifyPackage2Signature(pkg2::Package2Header &header, const void *mod, size_t mod_size) {
        return VerifySignature(header.signature, sizeof(header.signature), mod, mod_size, std::addressof(header.meta), sizeof(header.meta));
    }

    void DecryptPackage2(void *dst, size_t dst_size, const void *src, size_t src_size, const void *key, size_t key_size, const void *iv, size_t iv_size, u8 key_generation) {
        /* Ensure that the SE sees consistent data. */
        hw::FlushDataCache(key, key_size);
        hw::FlushDataCache(src, src_size);
        hw::FlushDataCache(dst, dst_size);
        hw::DataSynchronizationBarrierInnerShareable();

        /* Load the needed master key into the temporary keyslot. */
        secmon::LoadMasterKey(pkg1::AesKeySlot_Temporary, key_generation);

        /* Load the package2 key into the temporary keyslot. */
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_Temporary, pkg1::AesKeySlot_Temporary, key, key_size);

        /* Decrypt the data. */
        se::ComputeAes128Ctr(dst, dst_size, pkg1::AesKeySlot_Temporary,  src, src_size, iv, iv_size);

        /* Clear the keyslot we just used. */
        se::ClearAesKeySlot(pkg1::AesKeySlot_Temporary);

        /* Ensure that the cpu sees consistent data. */
        hw::DataSynchronizationBarrierInnerShareable();
        hw::FlushDataCache(dst, dst_size);
        hw::DataSynchronizationBarrierInnerShareable();
    }

}
