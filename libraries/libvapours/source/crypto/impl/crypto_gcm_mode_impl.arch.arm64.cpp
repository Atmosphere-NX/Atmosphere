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
#include <vapours.hpp>

#if defined(ATMOSPHERE_IS_STRATOSPHERE)

/* TODO: EL0 implementation. */
namespace ams::crypto::impl {

}

#else

/* EL1+ implementation. */
namespace ams::crypto::impl {

    namespace {

        constexpr u64 GetMultiplyFactor(u8 value) {
            constexpr size_t Shift = BITSIZEOF(u8) - 1;
            constexpr u8     Mask  = (1u << Shift);
            return (value & Mask) >> Shift;
        }

        /* TODO: Big endian support, eventually? */
        constexpr void GaloisShiftLeft(u64 *block) {
            /* Shift the block left by one. */
            block[1] <<= 1;
            block[1] |= (block[0] & (static_cast<u64>(1) << (BITSIZEOF(u64) - 1))) >> (BITSIZEOF(u64) - 1);
            block[0] <<= 1;
        }

        constexpr u8 GaloisShiftRight(u64 *block) {
            /* Determine the mask to return. */
            constexpr u8 GaloisFieldMask = 0xE1;
            const u8 mask = (block[0] & 1) * GaloisFieldMask;

            /* Shift the block right by one. */
            block[0] >>= 1;
            block[0] |= (block[1] & 1) << (BITSIZEOF(u64) - 1);
            block[1] >>= 1;

            /* Return the mask. */
            return mask;
        }

        /* Multiply two 128-bit numbers X, Y in the GF(128) Galois Field. */
        void GaloisFieldMult(void *dst, const void *x, const void *y) {
            /* Our block size is 16 bytes (for a 128-bit integer). */
            constexpr size_t BlockSize = 16;
            constexpr size_t FieldSize = 128;

            /* Declare work blocks for us to store temporary values. */
            u8 x_block[BlockSize];
            u8 y_block[BlockSize];
            u8 out[BlockSize];

            /* Declare 64-bit pointers for our convenience. */
            u64 *x_64   = static_cast<u64 *>(static_cast<void *>(x_block));
            u64 *y_64   = static_cast<u64 *>(static_cast<void *>(y_block));
            u64 *out_64 = static_cast<u64 *>(static_cast<void *>(out));

            /* Initialize our work blocks. */
            for (size_t i = 0; i < BlockSize; ++i) {
                x_block[i] = static_cast<const u8 *>(x)[BlockSize - 1 - i];
                y_block[i] = static_cast<const u8 *>(y)[BlockSize - 1 - i];
                out[i]     = 0;
            }

            /* Perform multiplication on each bit in y. */
            for (size_t i = 0; i < FieldSize; ++i) {
                /* Get the multiply factor for this bit. */
                const auto y_mult = GetMultiplyFactor(y_block[BlockSize - 1]);

                /* Multiply x by the factor. */
                out_64[0] ^= x_64[0] * y_mult;
                out_64[1] ^= x_64[1] * y_mult;

                /* Shift left y by one. */
                GaloisShiftLeft(y_64);

                /* Shift right x by one, and mask appropriately. */
                const u8 x_mask = GaloisShiftRight(x_64);
                x_block[BlockSize - 1] ^= x_mask;
            }

            /* Copy out our result. */
            for (size_t i = 0; i < BlockSize; ++i) {
                static_cast<u8 *>(dst)[i] = out[BlockSize - 1 - i];
            }
        }

    }

    template<class BlockCipher>
    void GcmModeImpl<BlockCipher>::Initialize(const BlockCipher *block_cipher) {
        /* Set member variables. */
        this->block_cipher = block_cipher;
        this->cipher_func  = std::addressof(GcmModeImpl<BlockCipher>::ProcessBlock);

        /* Pre-calculate values to speed up galois field multiplications later. */
        this->InitializeHashKey();

        /* Note that we're initialized. */
        this->state = State_Initialized;
    }

    template<class BlockCipher>
    void GcmModeImpl<BlockCipher>::Reset(const void *iv, size_t iv_size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->state >= State_Initialized);

        /* Reset blocks. */
        this->block_x.block_128.Clear();
        this->block_tmp.block_128.Clear();

        /* Clear sizes. */
        this->aad_size      = 0;
        this->msg_size      = 0;
        this->aad_remaining = 0;
        this->msg_remaining = 0;

        /* Update our state. */
        this->state = State_ProcessingAad;

        /* Set our iv. */
        if (iv_size == 12) {
            /* If our iv is the correct size, simply copy in the iv, and set the magic bit. */
            std::memcpy(std::addressof(this->block_ek0), iv, iv_size);
            util::StoreBigEndian(this->block_ek0.block_32 + 3, static_cast<u32>(1));
        } else {
            /* Clear our ek0 block. */
            this->block_ek0.block_128.Clear();

            /* Update using the iv as aad. */
            this->UpdateAad(iv, iv_size);

            /* Treat the iv as fake msg for the mac that will become our iv. */
            this->msg_size = this->aad_size;
            this->aad_size = 0;

            /* Compute a non-final mac. */
            this->ComputeMac(false);

            /* Set our ek0 block to our calculated mac block. */
            this->block_ek0 = this->block_x;

            /* Clear our calculated mac block. */
            this->block_x.block_128.Clear();

            /* Reset our state. */
            this->msg_size      = 0;
            this->aad_size      = 0;
            this->msg_remaining = 0;
            this->aad_remaining = 0;
        }

        /* Set the working block to the iv. */
        this->block_ek = this->block_ek0;
    }

    template<class BlockCipher>
    void GcmModeImpl<BlockCipher>::UpdateAad(const void *aad, size_t aad_size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->state    == State_ProcessingAad);
        AMS_ASSERT(this->msg_size == 0);

        /* Update our aad size. */
        this->aad_size += aad_size;

        /* Define a working tracker variable. */
        const u8 *cur_aad = static_cast<const u8 *>(aad);

        /* Process any leftover aad data from a previous invocation. */
        if (this->aad_remaining > 0) {
            while (aad_size > 0) {
                /* Copy in a byte of the aad to our partial block. */
                this->block_x.block_8[this->aad_remaining] ^= *(cur_aad++);

                /* Note that we consumed a byte. */
                --aad_size;

                /* Increment our partial block size. */
                this->aad_remaining = (this->aad_remaining + 1) % BlockSize;

                /* If we have a complete block, process it and move onward. */
                GaloisFieldMult(std::addressof(this->block_x), std::addressof(this->block_x), std::addressof(this->h_mult_blocks[0]));
            }
        }

        /* Process as many blocks as we can. */
        while (aad_size >= BlockSize) {
            /* Xor the current aad into our work block. */
            for (size_t i = 0; i < BlockSize; ++i) {
                this->block_x.block_8[i] ^= *(cur_aad++);
            }

            /* Multiply the blocks in our galois field. */
            GaloisFieldMult(std::addressof(this->block_x), std::addressof(this->block_x), std::addressof(this->h_mult_blocks[0]));

            /* Note that we've processed a block. */
            aad_size -= BlockSize;
        }

        /* Update our state with whatever aad is left over. */
        if (aad_size > 0) {
            /* Note how much left over data we have. */
            this->aad_remaining = static_cast<u32>(aad_size);

            /* Xor the data in. */
            for (size_t i = 0; i < aad_size; ++i) {
                this->block_x.block_8[i] ^= *(cur_aad++);
            }
        }
    }

    /* TODO: template<class BlockCipher> size_t GcmModeImpl<BlockCipher>::UpdateEncrypt(void *dst, size_t dst_size, const void *src, size_t src_size); */

    /* TODO: template<class BlockCipher> size_t GcmModeImpl<BlockCipher>::UpdateDecrypt(void *dst, size_t dst_size, const void *src, size_t src_size); */

    template<class BlockCipher>
    void GcmModeImpl<BlockCipher>::GetMac(void *dst, size_t dst_size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(State_ProcessingAad <= this->state && this->state <= State_Done);
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size >= MacSize);
        AMS_ASSERT(this->aad_remaining == 0);
        AMS_ASSERT(this->msg_remaining == 0);

        /* If we haven't already done so, compute the final mac. */
        if (this->state != State_Done) {
            this->ComputeMac(true);
            this->state = State_Done;
        }

        static_assert(sizeof(this->block_x) == MacSize);
        std::memcpy(dst, std::addressof(this->block_x), MacSize);
    }

    template<class BlockCipher>
    void GcmModeImpl<BlockCipher>::InitializeHashKey() {
        /* We want to encrypt an empty block to use for intermediate calculations. */
        /* NOTE: Non-EL1 implementations will do multiple encryptions ahead of time, */
        /*       to speed up galois field arithmetic. */
        constexpr const Block EmptyBlock = {};

        this->ProcessBlock(std::addressof(this->h_mult_blocks[0]), std::addressof(EmptyBlock), this->block_cipher);
    }

    template<class BlockCipher>
    void GcmModeImpl<BlockCipher>::ComputeMac(bool encrypt) {
        /* If we have leftover data, process it. */
        if (this->aad_remaining > 0 || this->msg_remaining > 0) {
            GaloisFieldMult(std::addressof(this->block_x), std::addressof(this->block_x), std::addressof(this->h_mult_blocks[0]));
        }

        /* Setup the last block. */
        Block last_block = Block{ .block_128 = { this->msg_size, this->aad_size } };

        /* Multiply the last block by 8 to account for bit vs byte sizes. */
        static_assert(offsetof(Block128, hi) == 0);
        GaloisShiftLeft(std::addressof(last_block.block_128.hi));
        GaloisShiftLeft(std::addressof(last_block.block_128.hi));
        GaloisShiftLeft(std::addressof(last_block.block_128.hi));

        /* Xor the data in. */
        for (size_t i = 0; i < BlockSize; ++i) {
            this->block_x.block_8[BlockSize - 1 - i] ^= last_block.block_8[i];
        }

        /* Perform the final multiplication. */
        GaloisFieldMult(std::addressof(this->block_x), std::addressof(this->block_x), std::addressof(this->h_mult_blocks[0]));

        /* If we need to do an encryption, do so. */
        if (encrypt) {
            /* Encrypt the iv. */
            u8 enc_result[BlockSize];
            this->ProcessBlock(enc_result, std::addressof(this->block_ek0), this->block_cipher);

            /* Xor the iv in. */
            for (size_t i = 0; i < BlockSize; ++i) {
                this->block_x.block_8[i] ^= enc_result[i];
            }
        }
    }

    /* Explicitly instantiate the valid template classes. */
    template class GcmModeImpl<AesEncryptor128>;

}

#endif
