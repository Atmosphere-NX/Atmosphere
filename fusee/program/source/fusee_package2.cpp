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
#include "fusee_package2.hpp"
#include "fusee_key_derivation.hpp"
#include "fusee_fatal.hpp"

namespace ams::nxboot {

    namespace {

        alignas(se::AesBlockSize) constexpr inline const u8 Package2KeySource[se::AesBlockSize] = {
            0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7
        };

        void PreparePackage2Key(int pkg2_slot, int key_generation) {
            /* Get keyslot for the desired master key. */
            const int master_slot = PrepareMasterKey(key_generation);

            /* Load the package2 key into the desired keyslot. */
            se::SetEncryptedAesKey128(pkg2_slot, master_slot, Package2KeySource, sizeof(Package2KeySource));
        }

        void DecryptPackage2(void *dst, size_t dst_size, const void *src, size_t src_size, const void *iv, size_t iv_size, u8 key_generation) {
            /* Ensure that the SE sees consistent data. */
            hw::FlushDataCache(src, src_size);
            if (src != dst) {
                hw::FlushDataCache(dst, dst_size);
            }

            /* Load the package2 key into the temporary keyslot. */
            PreparePackage2Key(pkg1::AesKeySlot_Temporary, key_generation);

            /* Decrypt the data. */
            se::ComputeAes128Ctr(dst, dst_size, pkg1::AesKeySlot_Temporary, src, src_size, iv, iv_size);

            /* Clear the keyslot we just used. */
            se::ClearAesKeySlot(pkg1::AesKeySlot_Temporary);

            /* Ensure that the cpu sees consistent data. */
            hw::InvalidateDataCache(dst, dst_size);
        }

        void DecryptPackage2Header(pkg2::Package2Meta *dst, const pkg2::Package2Meta &src) {
            constexpr int IvSize = 0x10;

            /* Decrypt the header. */
            DecryptPackage2(dst, sizeof(*dst), std::addressof(src), sizeof(src), std::addressof(src), IvSize, src.GetKeyGeneration());

            /* Copy back the iv, which encodes encrypted metadata. */
            std::memcpy(dst, std::addressof(src), IvSize);
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

    }

    void DecryptPackage2(u8 *package2) {
        /* Decrypt package2 header. */
        pkg2::Package2Header *header = reinterpret_cast<pkg2::Package2Header *>(package2);
        {
            pkg2::Package2Header tmp = *header;
            DecryptPackage2Header(std::addressof(header->meta), tmp.meta);
        }

        /* Check package2 magic. */
        if (!VerifyPackage2Meta(header->meta)) {
            ShowFatalError("Package2 meta is invalid!\n");
        }

        /* Decrypt package2 payloads. */
        u8 *payload = package2 + sizeof(*header);
        const u8 key_generation = header->meta.GetKeyGeneration();
        for (int i = 0; i < pkg2::PayloadCount; ++i) {
            if (header->meta.payload_sizes[i] == 0) {
                continue;
            }

            DecryptPackage2(payload, header->meta.payload_sizes[i], payload, header->meta.payload_sizes[i], header->meta.payload_ivs[i], sizeof(header->meta.payload_ivs[i]), key_generation);

            payload += header->meta.payload_sizes[i];
        }
    }

}
