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
#include "secmon_boot.hpp"

namespace ams::secmon::boot {

    namespace {

        constinit const u8 RsaPublicKeyExponent[] = {
            0x00, 0x01, 0x00, 0x01,
        };

        constexpr inline u8 TailMagic = 0xBC;

        bool VerifyRsaPssSha256(const u8 *sig, const void *msg, size_t msg_size) {
            /* Define constants. */
            constexpr int EmBits  = 2047;
            constexpr int EmLen   = util::DivideUp(EmBits, BITSIZEOF(u8));
            constexpr int SaltLen = 0x20;
            constexpr int HashLen = se::Sha256HashSize;

            /* Define a work buffer. */
            u8 work[EmLen];
            ON_SCOPE_EXIT { util::ClearMemory(work, sizeof(work)); };

            /* Calculate the message hash, first flushing cache to ensure SE sees correct data. */
            se::Sha256Hash msg_hash;
            hw::FlushDataCache(msg, msg_size);
            hw::DataSynchronizationBarrierInnerShareable();
            se::CalculateSha256(std::addressof(msg_hash), msg, msg_size);

            /* Verify the tail magic. */
            bool is_valid = sig[EmLen - 1] == TailMagic;

            /* Determine extents of masked db and h. */
            const u8 *masked_db = std::addressof(sig[0]);
            const u8 *h         = std::addressof(sig[EmLen - HashLen - 1]);

            /* Verify the extra bits are zero. */
            is_valid &= (masked_db[0] >> (BITSIZEOF(u8) - (BITSIZEOF(u8) * EmLen - EmBits))) == 0;

            /* Calculate the db mask. */
            {
                constexpr int MaskLen   = EmLen - HashLen - 1;
                constexpr int HashIters = util::DivideUp(MaskLen, HashLen);

                u8 mgf1_buf[sizeof(u32) + HashLen];

                std::memcpy(std::addressof(mgf1_buf[0]), h, HashLen);
                std::memset(std::addressof(mgf1_buf[HashLen]), 0, sizeof(u32));

                for (int i = 0; i < HashIters; ++i) {
                    /* Set the counter for this iteration. */
                    mgf1_buf[sizeof(mgf1_buf) - 1] = i;

                    /* Calculate the sha256 to the appropriate place in the work buffer. */
                    auto *mgf1_dst = reinterpret_cast<se::Sha256Hash *>(std::addressof(work[HashLen * i]));

                    hw::FlushDataCache(mgf1_buf, sizeof(mgf1_buf));
                    hw::DataSynchronizationBarrierInnerShareable();
                    se::CalculateSha256(mgf1_dst, mgf1_buf, sizeof(mgf1_buf));
                }
            }

            /* Decrypt masked db using the mask we just generated. */
            for (int i = 0; i < EmLen - HashLen - 1; ++i) {
                work[i] ^= masked_db[i];
            }

            /* Mask out the top bits. */
            u8 *db = work;
            db[0] &= 0xFF >> (BITSIZEOF(u8) * EmLen - EmBits);

            /* Verify that DB is of the form 0000...0001 */
            constexpr int DbLen = EmLen - HashLen - 1;
            int salt_ofs = 0;
            {
                int looking_for_one = 1;
                int invalid_db_padding = 0;
                int is_zero;
                int is_one;
                for (size_t i = 0; i < DbLen; /* ... */) {
                    is_zero = (db[i] == 0);
                    is_one  = (db[i] == 1);
                    salt_ofs += (looking_for_one & is_one) * (static_cast<s32>(++i));
                    looking_for_one &= ~is_one;
                    invalid_db_padding |= (looking_for_one & ~is_zero);
                }

                is_valid &= (invalid_db_padding == 0);
            }

            /* Verify salt. */
            is_valid &= (DbLen - salt_ofs) == SaltLen;

            /* Setup the message to verify. */
            const u8 *salt = std::addressof(db[DbLen - SaltLen]);
            u8 verif_msg[8 + HashLen + SaltLen];
            ON_SCOPE_EXIT { util::ClearMemory(verif_msg, sizeof(verif_msg)); };

            util::ClearMemory(std::addressof(verif_msg[0]), 8);
            std::memcpy(std::addressof(verif_msg[8]), std::addressof(msg_hash), HashLen);
            std::memcpy(std::addressof(verif_msg[8 + HashLen]), salt, SaltLen);

            /* Verify the final hash. */
            return VerifyHash(h, reinterpret_cast<uintptr_t>(std::addressof(verif_msg[0])), sizeof(verif_msg));
        }

        bool VerifyRsaPssSha256(int slot, void *sig, size_t sig_size, const void *msg, size_t msg_size) {
            /* Exponentiate the signature, using the signature as the destination buffer. */
            se::ModularExponentiate(sig, sig_size, slot, sig, sig_size);

            /* Verify the pss padding. */
            return VerifyRsaPssSha256(static_cast<const u8 *>(sig), msg, msg_size);
        }

    }

    bool VerifySignature(void *sig, size_t sig_size, const void *mod, size_t mod_size, const void *msg, size_t msg_size) {
        /* Load the public key into a temporary keyslot. */
        const int slot = pkg1::RsaKeySlot_Temporary;
        se::SetRsaKey(slot, mod, mod_size, RsaPublicKeyExponent, util::size(RsaPublicKeyExponent));

        return VerifyRsaPssSha256(slot, sig, sig_size, msg, msg_size);
    }

    bool VerifyHash(const void *hash, uintptr_t msg, size_t msg_size) {
        /* Zero-sized messages are always valid. */
        if (msg_size == 0) {
            return true;
        }

        /* Ensure that the SE sees correct data for the message. */
        hw::FlushDataCache(reinterpret_cast<void *>(msg), msg_size);
        hw::DataSynchronizationBarrierInnerShareable();

        /* Calculate the hash. */
        se::Sha256Hash calc_hash;
        se::CalculateSha256(std::addressof(calc_hash), reinterpret_cast<void *>(msg), msg_size);

        /* Verify the result. */
        return crypto::IsSameBytes(std::addressof(calc_hash), hash, sizeof(calc_hash));
    }

}
