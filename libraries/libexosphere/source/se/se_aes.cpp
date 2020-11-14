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
#include "se_execute.hpp"

namespace ams::se {

    namespace {

        constexpr inline int AesKeySizeMax = 256 / BITSIZEOF(u8);

        enum AesMode {
            AesMode_Aes128 = ((SE_CONFIG_ENC_MODE_AESMODE_KEY128 << SE_CONFIG_ENC_MODE_OFFSET) | (SE_CONFIG_DEC_MODE_AESMODE_KEY128 << SE_CONFIG_DEC_MODE_OFFSET)) >> SE_CONFIG_DEC_MODE_OFFSET,
            AesMode_Aes192 = ((SE_CONFIG_ENC_MODE_AESMODE_KEY192 << SE_CONFIG_ENC_MODE_OFFSET) | (SE_CONFIG_DEC_MODE_AESMODE_KEY192 << SE_CONFIG_DEC_MODE_OFFSET)) >> SE_CONFIG_DEC_MODE_OFFSET,
            AesMode_Aes256 = ((SE_CONFIG_ENC_MODE_AESMODE_KEY256 << SE_CONFIG_ENC_MODE_OFFSET) | (SE_CONFIG_DEC_MODE_AESMODE_KEY256 << SE_CONFIG_DEC_MODE_OFFSET)) >> SE_CONFIG_DEC_MODE_OFFSET,
        };

        enum MemoryInterface {
            MemoryInterface_Ahb = SE_CRYPTO_CONFIG_MEMIF_AHB,
            MemoryInterface_Mc  = SE_CRYPTO_CONFIG_MEMIF_MCCIF,
        };

        constexpr inline u32 AesConfigEcb        = reg::Encode(SE_REG_BITS_VALUE(CRYPTO_CONFIG_CTR_CNTN,                     0),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_KEYSCH_BYPASS,          DISABLE),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_IV_SELECT,             ORIGINAL),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_VCTRAM_SEL,              MEMORY),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_INPUT_SEL,               MEMORY),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_XOR_POS,                 BYPASS),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_HASH_ENB,               DISABLE));

        constexpr inline u32 AesConfigCtr        = reg::Encode(SE_REG_BITS_VALUE(CRYPTO_CONFIG_CTR_CNTN,                     1),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_KEYSCH_BYPASS,          DISABLE),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_IV_SELECT,             ORIGINAL),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_VCTRAM_SEL,              MEMORY),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_INPUT_SEL,           LINEAR_CTR),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_XOR_POS,                 BOTTOM),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_HASH_ENB,               DISABLE));

        constexpr inline u32 AesConfigCmac       = reg::Encode(SE_REG_BITS_VALUE(CRYPTO_CONFIG_CTR_CNTN,                     0),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_KEYSCH_BYPASS,          DISABLE),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_IV_SELECT,             ORIGINAL),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_VCTRAM_SEL,         INIT_AESOUT),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_INPUT_SEL,               MEMORY),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_XOR_POS,                    TOP),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_HASH_ENB,                ENABLE));

        constexpr inline u32 AesConfigCbcEncrypt = reg::Encode(SE_REG_BITS_VALUE(CRYPTO_CONFIG_CTR_CNTN,                     0),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_KEYSCH_BYPASS,          DISABLE),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_IV_SELECT,             ORIGINAL),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_VCTRAM_SEL,         INIT_AESOUT),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_INPUT_SEL,               MEMORY),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_XOR_POS,                    TOP),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_HASH_ENB,               DISABLE));

        constexpr inline u32 AesConfigCbcDecrypt = reg::Encode(SE_REG_BITS_VALUE(CRYPTO_CONFIG_CTR_CNTN,                     0),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_KEYSCH_BYPASS,          DISABLE),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_IV_SELECT,             ORIGINAL),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_VCTRAM_SEL,    INIT_PREV_MEMORY),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_INPUT_SEL,               MEMORY),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_XOR_POS,                 BOTTOM),
                                                               SE_REG_BITS_ENUM (CRYPTO_CONFIG_HASH_ENB,               DISABLE));

        void SetConfig(volatile SecurityEngineRegisters *SE, bool encrypt, SE_CONFIG_DST dst) {
            reg::Write(SE->SE_CONFIG, SE_REG_BITS_ENUM    (CONFIG_ENC_MODE, AESMODE_KEY128),
                                      SE_REG_BITS_ENUM    (CONFIG_DEC_MODE, AESMODE_KEY128),
                                      SE_REG_BITS_ENUM_SEL(CONFIG_ENC_ALG,          encrypt, AES_ENC,     NOP),
                                      SE_REG_BITS_ENUM_SEL(CONFIG_DEC_ALG,          encrypt,     NOP, AES_DEC),
                                      SE_REG_BITS_VALUE   (CONFIG_DST,                  dst));
        }

        void SetAesConfig(volatile SecurityEngineRegisters *SE, int slot, bool encrypt, u32 config) {
            const u32 encoded = reg::Encode(SE_REG_BITS_ENUM    (CRYPTO_CONFIG_MEMIF,        AHB),
                                            SE_REG_BITS_VALUE   (CRYPTO_CONFIG_KEY_INDEX,   slot),
                                            SE_REG_BITS_ENUM_SEL(CRYPTO_CONFIG_CORE_SEL,  encrypt, ENCRYPT, DECRYPT));

            reg::Write(SE->SE_CRYPTO_CONFIG, (config | encoded));
        }

        void SetBlockCount(volatile SecurityEngineRegisters *SE, int count) {
            reg::Write(SE->SE_CRYPTO_LAST_BLOCK, count - 1);
        }

        void UpdateAesMode(volatile SecurityEngineRegisters *SE, AesMode mode) {
            reg::ReadWrite(SE->SE_CONFIG, REG_BITS_VALUE(16, 16, mode));
        }

        void UpdateMemoryInterface(volatile SecurityEngineRegisters *SE, MemoryInterface memif) {
            reg::ReadWrite(SE->SE_CRYPTO_CONFIG, SE_REG_BITS_VALUE(CRYPTO_CONFIG_MEMIF, memif));
        }

        void SetCounter(volatile SecurityEngineRegisters *SE, const void *ctr) {
            const u32 *ctr_32 = reinterpret_cast<const u32 *>(ctr);

            /* Copy the input ctr to the linear CTR registers. */
            reg::Write(SE->SE_CRYPTO_LINEAR_CTR[0], util::LoadLittleEndian(ctr_32 + 0));
            reg::Write(SE->SE_CRYPTO_LINEAR_CTR[1], util::LoadLittleEndian(ctr_32 + 1));
            reg::Write(SE->SE_CRYPTO_LINEAR_CTR[2], util::LoadLittleEndian(ctr_32 + 2));
            reg::Write(SE->SE_CRYPTO_LINEAR_CTR[3], util::LoadLittleEndian(ctr_32 + 3));
        }

        void SetAesKeyIv(volatile SecurityEngineRegisters *SE, int slot, const void *iv, size_t iv_size) {
            AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);
            AMS_ABORT_UNLESS(iv_size <= AesBlockSize);

            /* Set each iv word in order. */
            const u32 *iv_u32  = static_cast<const u32 *>(iv);
            const int num_words = iv_size / sizeof(u32);
            for (int i = 0; i < num_words; ++i) {
                /* Select the keyslot. */
                reg::Write(SE->SE_CRYPTO_KEYTABLE_ADDR, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_SLOT,         slot),
                                                        SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_KEYIV_SEL,          IV),
                                                        SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_IV_SEL,    ORIGINAL_IV),
                                                        SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_WORD,            i));

                /* Set the iv word. */
                SE->SE_CRYPTO_KEYTABLE_DATA = *(iv_u32++);
            }
        }

        void SetEncryptedAesKey(int dst_slot, int kek_slot, const void *key, size_t key_size, AesMode mode) {
            AMS_ABORT_UNLESS(key_size <= AesKeySizeMax);
            AMS_ABORT_UNLESS(0 <= dst_slot && dst_slot < AesKeySlotCount);
            AMS_ABORT_UNLESS(0 <= kek_slot && kek_slot < AesKeySlotCount);

            /* Get the engine. */
            auto *SE = GetRegisters();

            /* Configure for single AES ECB decryption to key table. */
            SetConfig(SE, false, SE_CONFIG_DST_KEYTABLE);
            SetAesConfig(SE, kek_slot, false, AesConfigEcb);
            UpdateAesMode(SE, mode);
            SetBlockCount(SE, 1);

            /* Select the destination keyslot. */
            reg::Write(SE->SE_CRYPTO_KEYTABLE_DST, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_DST_KEY_INDEX, dst_slot), SE_REG_BITS_ENUM(CRYPTO_KEYTABLE_DST_WORD_QUAD, KEYS_0_3));

            /* Ensure that the se sees the keydata we want it to. */
            hw::FlushDataCache(key, key_size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Execute the operation. */
            ExecuteOperation(SE, SE_OPERATION_OP_START, nullptr, 0, key, key_size);
        }

        void EncryptAes(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, AesMode mode) {
            /* If nothing to decrypt, succeed. */
            if (src_size == 0) { return; }

            /* Validate input. */
            AMS_ABORT_UNLESS(dst_size == AesBlockSize);
            AMS_ABORT_UNLESS(src_size == AesBlockSize);
            AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

            /* Get the engine. */
            auto *SE = GetRegisters();

            /* Configure for AES-ECB encryption to memory. */
            SetConfig(SE, true, SE_CONFIG_DST_MEMORY);
            SetAesConfig(SE, slot, true, AesConfigEcb);
            UpdateAesMode(SE, mode);

            /* Execute the operation. */
            ExecuteOperationSingleBlock(SE, dst, dst_size, src, src_size);
        }

        void ExpandSubkey(u8 *subkey) {
            /* Shift everything left one bit. */
            u8 prev = 0;
            for (int i = AesBlockSize - 1; i >= 0; --i) {
                const u8 top = (subkey[i] >> 7);
                subkey[i] = ((subkey[i] << 1) | prev);
                prev = top;
            }

            /* And xor with Rb if necessary. */
            if (prev != 0) {
                subkey[AesBlockSize - 1] ^= 0x87;
            }
        }

        void GetCmacResult(volatile SecurityEngineRegisters *SE, void *dst, size_t dst_size) {
            const int num_words = dst_size / sizeof(u32);
            for (int i = 0; i < num_words; ++i) {
                reg::Write(static_cast<u32 *>(dst) + i, reg::Read(SE->SE_HASH_RESULT[i]));
            }
        }

        void ComputeAesCmac(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, AesMode mode) {
            /* Validate input. */
            AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

            /* Get the engine. */
            auto *SE = GetRegisters();

            /* Determine mac extents. */
            const int num_blocks         = util::DivideUp(src_size, AesBlockSize);
            const size_t last_block_size = (src_size == 0) ? 0 : (src_size - ((num_blocks - 1) * AesBlockSize));

            /* Create subkey. */
            u8 subkey[AesBlockSize];
            {
                /* Encrypt zeroes. */
                std::memset(subkey, 0, sizeof(subkey));
                EncryptAes(subkey, sizeof(subkey), slot, subkey, sizeof(subkey), mode);

                /* Expand. */
                ExpandSubkey(subkey);

                /* Account for last block. */
                if (last_block_size != AesBlockSize) {
                    ExpandSubkey(subkey);
                }
            }

            /* Configure for AES-CMAC. */
            SetConfig(SE, true, SE_CONFIG_DST_HASH_REG);
            SetAesConfig(SE, slot, true, AesConfigCmac);
            UpdateAesMode(SE, mode);

            /* Set the IV to zero. */
            for (int i = 0; i < 4; ++i) {
                /* Select the keyslot. */
                reg::Write(SE->SE_CRYPTO_KEYTABLE_ADDR, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_SLOT,         slot),
                                                        SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_KEYIV_SEL,          IV),
                                                        SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_IV_SEL,    ORIGINAL_IV),
                                                        SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_WORD,            i));

                /* Set the iv word. */
                SE->SE_CRYPTO_KEYTABLE_DATA = 0;
            }

            /* Handle blocks before the last. */
            if (num_blocks > 1) {
                SetBlockCount(SE, num_blocks - 1);
                ExecuteOperation(SE, SE_OPERATION_OP_START, nullptr, 0, src, src_size);
                reg::ReadWrite(SE->SE_CRYPTO_CONFIG, SE_REG_BITS_ENUM(CRYPTO_CONFIG_IV_SELECT, UPDATED));
            }

            /* Handle the last block. */
            {
                SetBlockCount(SE, 1);

                /* Create the last block. */
                u8 last_block[AesBlockSize];
                if (last_block_size < sizeof(last_block)) {
                    std::memset(last_block, 0, sizeof(last_block));
                    last_block[last_block_size] = 0x80;
                }
                std::memcpy(last_block, static_cast<const u8 *>(src) + src_size - last_block_size, last_block_size);

                /* Xor with the subkey. */
                for (size_t i = 0; i < AesBlockSize; ++i) {
                    last_block[i] ^= subkey[i];
                }

                /* Ensure the SE sees correct data. */
                hw::FlushDataCache(last_block, sizeof(last_block));
                hw::DataSynchronizationBarrierInnerShareable();

                ExecuteOperation(SE, SE_OPERATION_OP_START, nullptr, 0, last_block, sizeof(last_block));
            }

            /* Get the output. */
            GetCmacResult(SE, dst, dst_size);
        }

        void EncryptAesCbc(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, const void *iv, size_t iv_size, AesMode mode) {
            /* If nothing to encrypt, succeed. */
            if (src_size == 0) { return; }

            /* Validate input. */
            AMS_ABORT_UNLESS(iv_size == AesBlockSize);
            AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

            /* Get the engine. */
            auto *SE = GetRegisters();

            /* Determine extents. */
            const size_t num_blocks   = src_size / AesBlockSize;
            const size_t aligned_size = num_blocks * AesBlockSize;
            AMS_ABORT_UNLESS(src_size == aligned_size);

            /* Configure for aes-cbc encryption. */
            SetConfig(SE, true, SE_CONFIG_DST_MEMORY);
            SetAesConfig(SE, slot, true, AesConfigCbcEncrypt);
            UpdateAesMode(SE, mode);

            /* Set the iv. */
            SetAesKeyIv(SE, slot, iv, iv_size);

            /* Set the block count. */
            SetBlockCount(SE, num_blocks);

            /* Execute the operation. */
            ExecuteOperation(SE, SE_OPERATION_OP_START, dst, dst_size, src, aligned_size);
        }

        void DecryptAesCbc(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, const void *iv, size_t iv_size, AesMode mode) {
            /* If nothing to decrypt, succeed. */
            if (src_size == 0) { return; }

            /* Validate input. */
            AMS_ABORT_UNLESS(iv_size == AesBlockSize);
            AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

            /* Get the engine. */
            auto *SE = GetRegisters();

            /* Determine extents. */
            const size_t num_blocks   = src_size / AesBlockSize;
            const size_t aligned_size = num_blocks * AesBlockSize;
            AMS_ABORT_UNLESS(src_size == aligned_size);

            /* Configure for aes-cbc encryption. */
            SetConfig(SE, false, SE_CONFIG_DST_MEMORY);
            SetAesConfig(SE, slot, false, AesConfigCbcDecrypt);
            UpdateAesMode(SE, mode);

            /* Set the iv. */
            SetAesKeyIv(SE, slot, iv, iv_size);

            /* Set the block count. */
            SetBlockCount(SE, num_blocks);

            /* Execute the operation. */
            ExecuteOperation(SE, SE_OPERATION_OP_START, dst, dst_size, src, aligned_size);
        }

        void ComputeAes128Async(u32 out_ll_address, int slot, u32 in_ll_address, u32 size, DoneHandler handler, u32 config, bool encrypt, volatile SecurityEngineRegisters *SE) {
            /* If nothing to decrypt, succeed. */
            if (size == 0) { return; }

            /* Validate input. */
            AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

            /* Configure for the specific operation. */
            SetConfig(SE, encrypt, SE_CONFIG_DST_MEMORY);
            SetAesConfig(SE, slot, encrypt, config);
            UpdateMemoryInterface(SE, MemoryInterface_Mc);

            /* Configure the number of blocks. */
            const int num_blocks = size / AesBlockSize;
            SetBlockCount(SE, num_blocks);

            /* Set the done handler. */
            SetDoneHandler(SE, handler);

            /* Start the raw operation. */
            StartOperationRaw(SE, SE_OPERATION_OP_START, out_ll_address, in_ll_address);
        }

        void ClearAesKeySlot(volatile SecurityEngineRegisters *SE, int slot) {
            /* Validate the key slot. */
            AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

            for (int i = 0; i < 16; ++i) {
                /* Select the keyslot. */
                reg::Write(SE->SE_CRYPTO_KEYTABLE_ADDR, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_SLOT, slot), SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_WORD, i));

                /* Write the data. */
                SE->SE_CRYPTO_KEYTABLE_DATA = 0;
            }
        }

    }

    void ClearAesKeySlot(int slot) {
        /* Clear the slot in SE1. */
        ClearAesKeySlot(GetRegisters(), slot);
    }

    void ClearAesKeySlot2(int slot) {
        /* Clear the slot in SE2. */
        ClearAesKeySlot(GetRegisters2(), slot);
    }

    void ClearAesKeyIv(int slot) {
        /* Validate the key slot. */
        AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Set each iv word in order. */
        for (int i = 0; i < 4; ++i) {
            /* Select the keyslot original iv. */
            reg::Write(SE->SE_CRYPTO_KEYTABLE_ADDR, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_SLOT,         slot),
                                                    SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_KEYIV_SEL,          IV),
                                                    SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_IV_SEL,    ORIGINAL_IV),
                                                    SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_WORD,            i));

            /* Set the iv word. */
            SE->SE_CRYPTO_KEYTABLE_DATA = 0;

            /* Select the keyslot updated iv. */
            reg::Write(SE->SE_CRYPTO_KEYTABLE_ADDR, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_SLOT,         slot),
                                                    SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_KEYIV_SEL,          IV),
                                                    SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_IV_SEL,     UPDATED_IV),
                                                    SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_WORD,            i));

            /* Set the iv word. */
            SE->SE_CRYPTO_KEYTABLE_DATA = 0;
        }
    }

    void LockAesKeySlot(int slot, u32 flags) {
        /* Validate the key slot. */
        AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Set non per-key flags. */
        if ((flags & ~KeySlotLockFlags_PerKey) != 0) {
            /* KeySlotLockFlags_DstKeyTableOnly is Mariko-only. */
            if (fuse::GetSocType() == fuse::SocType_Mariko) {
                reg::ReadWrite(SE->SE_CRYPTO_KEYTABLE_ACCESS[slot], REG_BITS_VALUE(0, 7, ~flags), REG_BITS_VALUE(7, 1, ((flags & KeySlotLockFlags_DstKeyTableOnly) != 0) ? 1 : 0));
            } else {
                reg::ReadWrite(SE->SE_CRYPTO_KEYTABLE_ACCESS[slot], REG_BITS_VALUE(0, 7, ~flags));
            }
        }

        /* Set per-key flag. */
        if ((flags & KeySlotLockFlags_PerKey) != 0) {
            reg::ReadWrite(SE->SE_CRYPTO_SECURITY_PERKEY, REG_BITS_VALUE(slot, 1, 0));
        }
    }

    void SetAesKey(int slot, const void *key, size_t key_size) {
        /* Validate the key slot and key size. */
        AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);
        AMS_ABORT_UNLESS(key_size <= AesKeySizeMax);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Set each key word in order. */
        const u32 *key_u32  = static_cast<const u32 *>(key);
        const int num_words = key_size / sizeof(u32);
        for (int i = 0; i < num_words; ++i) {
            /* Select the keyslot. */
            reg::Write(SE->SE_CRYPTO_KEYTABLE_ADDR, SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_SLOT,   slot),
                                                    SE_REG_BITS_ENUM (CRYPTO_KEYTABLE_ADDR_KEYIV_KEYIV_SEL,   KEY),
                                                    SE_REG_BITS_VALUE(CRYPTO_KEYTABLE_ADDR_KEYIV_KEY_WORD,      i));

            /* Set the key word. */
            SE->SE_CRYPTO_KEYTABLE_DATA = *(key_u32++);
        }
    }

    void SetEncryptedAesKey128(int dst_slot, int kek_slot, const void *key, size_t key_size) {
        return SetEncryptedAesKey(dst_slot, kek_slot, key, key_size, AesMode_Aes128);
    }

    void SetEncryptedAesKey256(int dst_slot, int kek_slot, const void *key, size_t key_size) {
        return SetEncryptedAesKey(dst_slot, kek_slot, key, key_size, AesMode_Aes256);
    }

    void EncryptAes128(void *dst, size_t dst_size, int slot, const void *src, size_t src_size) {
        return EncryptAes(dst, dst_size, slot, src, src_size, AesMode_Aes128);
    }

    void DecryptAes128(void *dst, size_t dst_size, int slot, const void *src, size_t src_size) {
        /* If nothing to decrypt, succeed. */
        if (src_size == 0) { return; }

        /* Validate input. */
        AMS_ABORT_UNLESS(dst_size == AesBlockSize);
        AMS_ABORT_UNLESS(src_size == AesBlockSize);
        AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Configure for AES-ECB decryption to memory. */
        SetConfig(SE, false, SE_CONFIG_DST_MEMORY);
        SetAesConfig(SE, slot, false, AesConfigEcb);

        ExecuteOperationSingleBlock(SE, dst, dst_size, src, src_size);
    }

    void ComputeAes128Ctr(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, const void *iv, size_t iv_size) {
        /* If nothing to do, succeed. */
        if (src_size == 0) { return; }

        /* Validate input. */
        AMS_ABORT_UNLESS(iv_size == AesBlockSize);
        AMS_ABORT_UNLESS(0 <= slot && slot < AesKeySlotCount);

        /* Get the engine. */
        auto *SE = GetRegisters();

        /* Determine how many full blocks we can operate on. */
        const size_t num_blocks   = src_size / AesBlockSize;
        const size_t aligned_size = num_blocks * AesBlockSize;
        const size_t fractional   = src_size - aligned_size;

        /* Here Nintendo writes 1 to SE_SPARE. It's unclear why they do this, but we will do so as well. */
        SE->SE_SPARE = 0x1;

        /* Configure for AES-CTR encryption/decryption to memory. */
        SetConfig(SE, true, SE_CONFIG_DST_MEMORY);
        SetAesConfig(SE, slot, true, AesConfigCtr);

        /* Set the counter. */
        SetCounter(SE, iv);

        /* Process as many aligned blocks as we can. */
        if (aligned_size > 0) {
            /* Configure the engine to process the right number of blocks. */
            SetBlockCount(SE, num_blocks);

            /* Execute the operation. */
            ExecuteOperation(SE, SE_OPERATION_OP_START, dst, dst_size, src, aligned_size);

            /* Synchronize around this point. */
            hw::DataSynchronizationBarrierInnerShareable();
        }

        /* Process a single block to output. */
        if (fractional > 0 && dst_size > aligned_size) {
            const size_t copy_size = std::min(fractional, dst_size - aligned_size);

            ExecuteOperationSingleBlock(SE, static_cast<u8 *>(dst) + aligned_size, copy_size, static_cast<const u8 *>(src) + aligned_size, fractional);
        }
    }

    void ComputeAes128Cmac(void *dst, size_t dst_size, int slot, const void *src, size_t src_size) {
        return ComputeAesCmac(dst, dst_size, slot, src, src_size, AesMode_Aes128);
    }

    void ComputeAes256Cmac(void *dst, size_t dst_size, int slot, const void *src, size_t src_size) {
        return ComputeAesCmac(dst, dst_size, slot, src, src_size, AesMode_Aes256);
    }

    void EncryptAes128Cbc(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, const void *iv, size_t iv_size) {
        return EncryptAesCbc(dst, dst_size, slot, src, src_size, iv, iv_size, AesMode_Aes128);
    }

    void EncryptAes256Cbc(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, const void *iv, size_t iv_size) {
        return EncryptAesCbc(dst, dst_size, slot, src, src_size, iv, iv_size, AesMode_Aes256);
    }

    void DecryptAes128Cbc(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, const void *iv, size_t iv_size) {
        return DecryptAesCbc(dst, dst_size, slot, src, src_size, iv, iv_size, AesMode_Aes128);
    }

    void DecryptAes256Cbc(void *dst, size_t dst_size, int slot, const void *src, size_t src_size, const void *iv, size_t iv_size) {
        return DecryptAesCbc(dst, dst_size, slot, src, src_size, iv, iv_size, AesMode_Aes256);
    }

    void EncryptAes128CbcAsync(u32 out_ll_address, int slot, u32 in_ll_address, u32 size, const void *iv, size_t iv_size, DoneHandler handler) {
        /* Validate the iv. */
        AMS_ABORT_UNLESS(iv_size == AesBlockSize);

        /* Get the registers. */
        volatile auto *SE = GetRegisters();

        /* Set the iv. */
        SetAesKeyIv(SE, slot, iv, iv_size);

        /* Perform the asynchronous aes operation. */
        ComputeAes128Async(out_ll_address, slot, in_ll_address, size, handler, AesConfigCbcEncrypt, true, SE);
    }

    void DecryptAes128CbcAsync(u32 out_ll_address, int slot, u32 in_ll_address, u32 size, const void *iv, size_t iv_size, DoneHandler handler) {
        /* Validate the iv. */
        AMS_ABORT_UNLESS(iv_size == AesBlockSize);

        /* Get the registers. */
        volatile auto *SE = GetRegisters();

        /* Set the iv. */
        SetAesKeyIv(SE, slot, iv, iv_size);

        /* Perform the asynchronous aes operation. */
        ComputeAes128Async(out_ll_address, slot, in_ll_address, size, handler, AesConfigCbcDecrypt, false, SE);
    }

    void ComputeAes128CtrAsync(u32 out_ll_address, int slot, u32 in_ll_address, u32 size, const void *iv, size_t iv_size, DoneHandler handler) {
        /* Validate the iv. */
        AMS_ABORT_UNLESS(iv_size == AesBlockSize);

        /* Get the registers. */
        volatile auto *SE = GetRegisters();

        /* Here Nintendo writes 1 to SE_SPARE. It's unclear why they do this, but we will do so as well. */
        SE->SE_SPARE = 0x1;

        /* Set the counter. */
        SetCounter(SE, iv);

        /* Perform the asynchronous aes operation. */
        ComputeAes128Async(out_ll_address, slot, in_ll_address, size, handler, AesConfigCtr, true, SE);
    }

}
