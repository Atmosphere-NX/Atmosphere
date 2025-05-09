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
#include <exosphere.hpp>
#include "../secmon_error.hpp"
#include "../secmon_key_storage.hpp"
#include "secmon_boot.hpp"

namespace ams::secmon::boot {

    void CalculatePackage2Hash(se::Sha256Hash *dst, const pkg2::Package2Meta &meta, uintptr_t package2_start) {
        /* Determine the region to hash. */
        const void *data  = reinterpret_cast<const void *>(package2_start);
        const size_t size = meta.GetSize();

        /* Flush to ensure the SE sees the correct data. */
        hw::FlushDataCache(data, size);
        hw::DataSynchronizationBarrierInnerShareable();

        /* Calculate the hash. */
        se::CalculateSha256(dst, data, size);
    }

    bool VerifyPackage2Signature(pkg2::Package2Header &header, const void *mod, size_t mod_size) {
        return VerifySignature(header.signature, sizeof(header.signature), mod, mod_size, std::addressof(header.meta), sizeof(header.meta));
    }

    int PrepareMasterKey(int key_generation) {
        if (key_generation == GetKeyGeneration()) {
            return pkg1::AesKeySlot_Master;
        }

        constexpr int Slot = pkg1::AesKeySlot_Temporary;
        LoadMasterKey(Slot, key_generation);

        return Slot;
    }

    void PreparePackage2Key(int pkg2_slot, int key_generation, const void *key, size_t key_size) {
        /* Get keyslot for the desired master key. */
        const int master_slot = PrepareMasterKey(key_generation);

        /* Load the package2 key into the desired keyslot. */
        se::SetEncryptedAesKey128(pkg2_slot, master_slot, key, key_size);
    }

    void DecryptPackage2(void *dst, size_t dst_size, const void *src, size_t src_size, const void *key, size_t key_size, const void *iv, size_t iv_size, u8 key_generation) {
        /* Ensure that the SE sees consistent data. */
        hw::FlushDataCache(key, key_size);
        hw::FlushDataCache(src, src_size);
        hw::FlushDataCache(dst, dst_size);
        hw::DataSynchronizationBarrierInnerShareable();

        /* Load the package2 key into the temporary keyslot. */
        PreparePackage2Key(pkg1::AesKeySlot_Temporary, key_generation, key, key_size);

        /* Decrypt the data. */
        se::ComputeAes128Ctr(dst, dst_size, pkg1::AesKeySlot_Temporary, src, src_size, iv, iv_size);

        /* Clear the keyslot we just used. */
        se::ClearAesKeySlot(pkg1::AesKeySlot_Temporary);

        /* Ensure that the cpu sees consistent data. */
        hw::DataSynchronizationBarrierInnerShareable();
        hw::FlushDataCache(dst, dst_size);
        hw::DataSynchronizationBarrierInnerShareable();
    }

    bool VerifyPackage2Meta(const pkg2::Package2Meta &meta) {
        /* Get the obfuscated metadata. */
        const size_t size       = meta.GetSize();
        const u8 key_generation = meta.GetKeyGeneration();

        /* Check that size is big enough for the header. */
        if (size <= sizeof(pkg2::Package2Header)) {
            return false;
        }

        /* Check that the size isn't larger than what we allow. */
        if (size > pkg2::Package2SizeMax) {
            return false;
        }

        /* Check that the key generation is one that we can use. */
        static_assert(pkg1::KeyGeneration_Count == 20);
        if (key_generation >= pkg1::KeyGeneration_Count) {
            return false;
        }

        /* Check the magic number. */
        if (!crypto::IsSameBytes(meta.magic, pkg2::Package2Meta::Magic::String, sizeof(meta.magic))) {
            return false;
        }

        /* Check the payload alignments. */
        if ((meta.entrypoint % pkg2::PayloadAlignment) != 0) {
            return false;
        }

        for (int i = 0; i < pkg2::PayloadCount; ++i) {
            if ((meta.payload_sizes[i] % pkg2::PayloadAlignment) != 0) {
                return false;
            }
        }

        /* Check that the sizes sum to the total. */
        if (size != sizeof(pkg2::Package2Header) + meta.payload_sizes[0] + meta.payload_sizes[1] + meta.payload_sizes[2]) {
            return false;
        }

        /* Check that the payloads do not overflow. */
        for (int i = 0; i < pkg2::PayloadCount; ++i) {
            if (meta.payload_offsets[i] > meta.payload_offsets[i] + meta.payload_sizes[i]) {
                return false;
            }
        }

        /* Verify that no payloads overlap. */
        for (int i = 0; i < pkg2::PayloadCount - 1; ++i) {
            for (int j = i + 1; j < pkg2::PayloadCount; ++j) {
                if (util::HasOverlap(meta.payload_offsets[i], meta.payload_sizes[i], meta.payload_offsets[j], meta.payload_sizes[j])) {
                    return false;
                }
            }
        }

        /* Check whether any payload contains the entrypoint. */
        for (int i = 0; i < pkg2::PayloadCount; ++i) {
            if (util::Contains(meta.payload_offsets[i], meta.payload_sizes[i], meta.entrypoint)) {
                return true;
            }
        }

        /* No payload contains the entrypoint, so we're not valid. */
        return false;
    }

    bool VerifyPackage2Version(const pkg2::Package2Meta &meta) {
        return meta.bootloader_version <= pkg2::CurrentBootloaderVersion && meta.package2_version >= pkg2::MinimumValidDataVersion;
    }

    bool VerifyPackage2Payloads(const pkg2::Package2Meta &meta, uintptr_t payload_address) {
        /* Verify hashes match for all payloads. */
        for (int i = 0; i < pkg2::PayloadCount; ++i) {
            /* Allow all-zero bytes to match any payload. */
            if (!(meta.payload_hashes[i][0] == 0 && std::memcmp(meta.payload_hashes[i] + 0, meta.payload_hashes[i] + 1, sizeof(meta.payload_hashes[i]) - 1) == 0)) {
                if (!VerifyHash(meta.payload_hashes[i], payload_address, meta.payload_sizes[i])) {
                    return false;
                }
            }

            payload_address += meta.payload_sizes[i];
        }

        return true;
    }

}
