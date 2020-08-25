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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_system_control.hpp>

namespace ams::kern {

    class KPageBitmap {
        private:
            class RandomBitGenerator {
                private:
                    util::TinyMT rng;
                    u32 entropy;
                    u32 bits_available;
                private:
                    void RefreshEntropy() {
                        this->entropy        = rng.GenerateRandomU32();
                        this->bits_available = BITSIZEOF(this->entropy);
                    }

                    bool GenerateRandomBit() {
                        if (this->bits_available == 0) {
                            this->RefreshEntropy();
                        }

                        const bool rnd_bit = (this->entropy & 1) != 0;
                        this->entropy >>= 1;
                        --this->bits_available;
                        return rnd_bit;
                    }
                public:
                    RandomBitGenerator() : rng(), entropy(), bits_available() {
                        this->rng.Initialize(static_cast<u32>(KSystemControl::GenerateRandomU64()));
                    }

                    size_t SelectRandomBit(u64 bitmap) {
                        u64 selected     = 0;

                        u64 cur_num_bits = BITSIZEOF(bitmap) / 2;
                        u64 cur_mask     = (1ull << cur_num_bits) - 1;

                        while (cur_num_bits) {
                            const u64 low  = (bitmap >> 0) & cur_mask;
                            const u64 high = (bitmap >> cur_num_bits) & cur_mask;

                            bool choose_low;
                            if (high == 0) {
                                /* If only low val is set, choose low. */
                                choose_low = true;
                            } else if (low == 0) {
                                /* If only high val is set, choose high. */
                                choose_low = false;
                            } else {
                                /* If both are set, choose random. */
                                choose_low = this->GenerateRandomBit();
                            }

                            /* If we chose low, proceed with low. */
                            if (choose_low) {
                                bitmap    = low;
                                selected += 0;
                            } else {
                                bitmap    = high;
                                selected += cur_num_bits;
                            }

                            /* Proceed. */
                            cur_num_bits /= 2;
                            cur_mask >>= cur_num_bits;
                        }

                        return selected;
                    }
            };
        public:
            static constexpr size_t MaxDepth = 4;
        private:
            u64 *bit_storages[MaxDepth];
            RandomBitGenerator rng;
            size_t num_bits;
            size_t used_depths;
        public:
            KPageBitmap() : bit_storages(), rng(), num_bits(), used_depths() { /* ... */ }

            constexpr size_t GetNumBits() const { return this->num_bits; }
            constexpr s32 GetHighestDepthIndex() const { return static_cast<s32>(this->used_depths) - 1; }

            u64 *Initialize(u64 *storage, size_t size) {
                /* Initially, everything is un-set. */
                this->num_bits = 0;

                /* Calculate the needed bitmap depth. */
                this->used_depths = static_cast<size_t>(GetRequiredDepth(size));
                MESOSPHERE_ASSERT(this->used_depths <= MaxDepth);

                /* Set the bitmap pointers. */
                for (s32 depth = this->GetHighestDepthIndex(); depth >= 0; depth--) {
                    this->bit_storages[depth] = storage;
                    size = util::AlignUp(size, BITSIZEOF(u64)) / BITSIZEOF(u64);
                    storage += size;
                }

                return storage;
            }

            ssize_t FindFreeBlock(bool random) {
                uintptr_t offset = 0;
                s32 depth = 0;

                if (random) {
                    do {
                        const u64 v = this->bit_storages[depth][offset];
                        if (v == 0) {
                            /* If depth is bigger than zero, then a previous level indicated a block was free. */
                            MESOSPHERE_ASSERT(depth == 0);
                            return -1;
                        }
                        offset = offset * BITSIZEOF(u64) + this->rng.SelectRandomBit(v);
                        ++depth;
                    } while (depth < static_cast<s32>(this->used_depths));
                } else {
                    do {
                        const u64 v = this->bit_storages[depth][offset];
                        if (v == 0) {
                            /* If depth is bigger than zero, then a previous level indicated a block was free. */
                            MESOSPHERE_ASSERT(depth == 0);
                            return -1;
                        }
                        offset = offset * BITSIZEOF(u64) + __builtin_ctzll(v);
                        ++depth;
                    } while (depth < static_cast<s32>(this->used_depths));
                }

                return static_cast<ssize_t>(offset);
            }

            void SetBit(size_t offset) {
                this->SetBit(this->GetHighestDepthIndex(), offset);
                this->num_bits++;
            }

            void ClearBit(size_t offset) {
                this->ClearBit(this->GetHighestDepthIndex(), offset);
                this->num_bits--;
            }

            bool ClearRange(size_t offset, size_t count) {
                s32 depth = this->GetHighestDepthIndex();
                u64 *bits = this->bit_storages[depth];
                size_t bit_ind = offset / BITSIZEOF(u64);
                if (AMS_LIKELY(count < BITSIZEOF(u64))) {
                    const size_t shift = offset % BITSIZEOF(u64);
                    MESOSPHERE_ASSERT(shift + count <= BITSIZEOF(u64));
                    /* Check that all the bits are set. */
                    const u64 mask = ((u64(1) << count) - 1) << shift;
                    u64 v = bits[bit_ind];
                    if ((v & mask) != mask) {
                        return false;
                    }

                    /* Clear the bits. */
                    v &= ~mask;
                    bits[bit_ind] = v;
                    if (v == 0) {
                        this->ClearBit(depth - 1, bit_ind);
                    }
                } else {
                    MESOSPHERE_ASSERT(offset % BITSIZEOF(u64) == 0);
                    MESOSPHERE_ASSERT(count  % BITSIZEOF(u64) == 0);
                    /* Check that all the bits are set. */
                    size_t remaining = count;
                    size_t i = 0;
                    do {
                        if (bits[bit_ind + i++] != ~u64(0)) {
                            return false;
                        }
                        remaining -= BITSIZEOF(u64);
                    } while (remaining > 0);

                    /* Clear the bits. */
                    remaining = count;
                    i = 0;
                    do {
                        bits[bit_ind + i] = 0;
                        this->ClearBit(depth - 1, bit_ind + i);
                        i++;
                        remaining -= BITSIZEOF(u64);
                    } while (remaining > 0);
                }

                this->num_bits -= count;
                return true;
            }
        private:
            void SetBit(s32 depth, size_t offset) {
                while (depth >= 0) {
                    size_t ind   = offset / BITSIZEOF(u64);
                    size_t which = offset % BITSIZEOF(u64);
                    const u64 mask = u64(1) << which;

                    u64 *bit = std::addressof(this->bit_storages[depth][ind]);
                    u64 v = *bit;
                    MESOSPHERE_ASSERT((v & mask) == 0);
                    *bit = v | mask;
                    if (v) {
                        break;
                    }
                    offset = ind;
                    depth--;
                }
            }

            void ClearBit(s32 depth, size_t offset) {
                while (depth >= 0) {
                    size_t ind   = offset / BITSIZEOF(u64);
                    size_t which = offset % BITSIZEOF(u64);
                    const u64 mask = u64(1) << which;

                    u64 *bit = std::addressof(this->bit_storages[depth][ind]);
                    u64 v = *bit;
                    MESOSPHERE_ASSERT((v & mask) != 0);
                    v &= ~mask;
                    *bit = v;
                    if (v) {
                        break;
                    }
                    offset = ind;
                    depth--;
                }
            }
        private:
            static constexpr s32 GetRequiredDepth(size_t region_size) {
                s32 depth = 0;
                while (true) {
                    region_size /= BITSIZEOF(u64);
                    depth++;
                    if (region_size == 0) {
                        return depth;
                    }
                }
            }
        public:
            static constexpr size_t CalculateManagementOverheadSize(size_t region_size) {
                size_t overhead_bits = 0;
                for (s32 depth = GetRequiredDepth(region_size) - 1; depth >= 0; depth--) {
                    region_size = util::AlignUp(region_size, BITSIZEOF(u64)) / BITSIZEOF(u64);
                    overhead_bits += region_size;
                }
                return overhead_bits * sizeof(u64);
            }
    };

}
