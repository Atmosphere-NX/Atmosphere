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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_fourcc.hpp>

namespace ams::util::impl {

    class AvailableIndexFinder {
        private:
            static constexpr int MaxDepthOfBox = 5;

            struct BitFlags64 {
                private:
                    u64 m_data;
                public:
                    constexpr ALWAYS_INLINE bool GetFlag(int index) const {
                        AMS_ASSERT(index < 64);
                        return ((m_data >> index) & UINT64_C(1)) != 0;
                    }

                    constexpr ALWAYS_INLINE void SetFlag(int index) {
                        AMS_ASSERT(index < 64);
                        m_data |= (UINT64_C(1) << index);
                    }

                    constexpr ALWAYS_INLINE void ClearFlag(int index) {
                        AMS_ASSERT(index < 64);
                        m_data &= ~(UINT64_C(1) << index);
                    }

                    constexpr ALWAYS_INLINE bool IsFull() const {
                        return m_data == ~(UINT64_C(0));
                    }

                    constexpr ALWAYS_INLINE int FindIndexOfBitZero() const {
                        AMS_ASSERT(!this->IsFull());
                        return __builtin_ctzll(~m_data);
                    }
            };
        private:
            int *m_p_current_index;
            int *m_p_map_index;
            void *m_buffer;
            int m_depth;
            BitFlags64 *m_flags;
        private:
            static constexpr int Pow64(int e) {
                switch (e) {
                    case 0:  return 0x1;
                    case 1:  return 0x40;
                    case 2:  return 0x1000;
                    case 3:  return 0x40000;
                    case 4:  return 0x1000000;
                    case 5:  return 0x40000000;
                    default: return -1;
                }
            }

            static constexpr u64 Roundup64(u64 value) {
                return (value + (64 - 1)) / 64;
            }

            static constexpr size_t GetRequiredNumOfBox(int depth, size_t num_elements) {
                if (depth == 1) {
                    return Roundup64(num_elements);
                } else if (depth == 2) {
                    return Roundup64(num_elements) + Pow64(0);
                } else if (depth == 3) {
                    return Roundup64(num_elements) + Pow64(0) + Pow64(1) + Pow64(2);
                } else if (depth == 4) {
                    return Roundup64(num_elements) + Pow64(0) + Pow64(1) + Pow64(2) + Pow64(3);
                } else if (depth == 5) {
                    return Roundup64(num_elements) + Pow64(0) + Pow64(1) + Pow64(2) + Pow64(3) + Pow64(4);
                } else {
                    return 0;
                }
            }

            static constexpr int CalcOffset(int *arr, int depth) {
                int offset = 0;
                for (auto i = 0; i < depth; ++i) {
                    offset += Pow64(i);
                }
                for (auto i = 0; i < depth; ++i) {
                    offset += Pow64(i) - arr[depth - 1 - i];
                }
                return offset;
            }
        public:
            static consteval int GetSignature() {
                return static_cast<int>(util::ReverseFourCC<'B', 'I', 'T', 'S'>::Code);
            }

            static constexpr int GetNeedDepth(size_t num_elements) {
                if (num_elements <= 0x40) {
                    return 1;
                } else if (num_elements <= 0x1000) {
                    return 2;
                } else if (num_elements <= 0x40000) {
                    return 3;
                } else if (num_elements <= 0x1000000) {
                    return 4;
                } else if (num_elements <= 0x40000000) {
                    return 5;
                } else {
                    return -1;
                }
            }

            static constexpr size_t GetRequiredMemorySize(size_t num_elements) {
                return sizeof(BitFlags64) * GetRequiredNumOfBox(GetNeedDepth(num_elements), num_elements);
            }
        public:
            void Initialize(int *cur, int *map, u8 *buf) {
                const size_t num_elements = static_cast<size_t>(*map);
                AMS_ASSERT(num_elements <= static_cast<size_t>(Pow64(MaxDepthOfBox - 1)));

                /* Set fields. */
                m_p_current_index = cur;
                m_p_map_index     = map;
                m_buffer          = buf;
                m_depth           = GetNeedDepth(num_elements);

                /* Validate fields. */
                AMS_ASSERT(m_depth > 0);

                /* Setup memory. */
                std::memset(m_buffer, 0, GetRequiredMemorySize(num_elements));
                m_flags = reinterpret_cast<BitFlags64 *>(m_buffer);
            }

            int AcquireIndex() {
                /* Validate pre-conditions. */
                AMS_ASSERT(*m_p_current_index < *m_p_map_index);

                /* Build up arrays. */
                int table[MaxDepthOfBox];
                BitFlags64 *pos[MaxDepthOfBox];
                for (auto i = 0; i < m_depth; ++i) {
                    /* Determine the position. */
                    pos[i] = std::addressof(m_flags[CalcOffset(table, i)]);

                    /* Set table entry. */
                    table[i] = pos[i]->FindIndexOfBitZero();
                    AMS_ASSERT(table[i] != BITSIZEOF(u64));
                }

                /* Validate that the index is not acquired. */
                AMS_ASSERT(!pos[m_depth - 1]->GetFlag(table[m_depth - 1]));

                /* Acquire the index. */
                pos[m_depth - 1]->SetFlag(table[m_depth - 1]);

                /* Validate that the index was acquired. */
                AMS_ASSERT(pos[m_depth - 1]->GetFlag(table[m_depth - 1]));

                /* Update tracking flags. */
                for (auto i = m_depth - 1; i > 0; --i) {
                    if (pos[i]->IsFull()) {
                        pos[i - 1]->SetFlag(table[i - 1]);
                    }
                }

                /* Calculate the index we acquired. */
                int index = 0, pow = 0;
                for (auto i = m_depth; i > 0; --i, ++pow) {
                    index += Pow64(pow) * table[i - 1];
                }

                /* Increment current index. */
                ++(*m_p_current_index);

                return index;
            }

            void ReleaseIndex(int index) {
                /* Convert index to table. */
                int table[MaxDepthOfBox];
                for (auto i = 0; i < m_depth; ++i) {
                    table[m_depth - 1 - i] = index % BITSIZEOF(u64);
                    index /= BITSIZEOF(u64);
                }

                /* Build up arrays. */
                BitFlags64 *pos[MaxDepthOfBox];
                for (auto i = 0; i < m_depth; ++i) {
                    /* Determine the position. */
                    pos[i] = std::addressof(m_flags[CalcOffset(table, i)]);
                }

                /* Validate that the flag is set. */
                AMS_ASSERT(pos[m_depth - 1]->GetFlag(table[m_depth - 1]));

                /* Clear the flags. */
                for (auto i = m_depth - 1; i >= 0; --i) {
                    pos[i]->ClearFlag(table[i]);
                }

                /* Decrement current index. */
                --(*m_p_current_index);
            }
    };

}
