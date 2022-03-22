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
#include <vapours/util/util_alignment.hpp>
#include <vapours/util/util_bitutil.hpp>

namespace ams::util {

    namespace impl {

        template<size_t Count, typename Storage>
        constexpr ALWAYS_INLINE void NegateImpl(Storage arr[]) {
            for (size_t i = 0; i < Count; i++) {
                arr[i] = ~arr[i];
            }
        }

        template<size_t Count, typename Storage>
        constexpr ALWAYS_INLINE void AndImpl(Storage dst[], const Storage src[]) {
            for (size_t i = 0; i < Count; i++) {
                dst[i] &= src[i];
            }
        }

        template<size_t Count, typename Storage>
        constexpr ALWAYS_INLINE void OrImpl(Storage dst[], const Storage src[]) {
            for (size_t i = 0; i < Count; i++) {
                dst[i] |= src[i];
            }
        }

        template<size_t Count, typename Storage>
        constexpr ALWAYS_INLINE void XorImpl(Storage dst[], const Storage src[]) {
            for (size_t i = 0; i < Count; i++) {
                dst[i] ^= src[i];
            }
        }

        template<size_t Count, typename Storage>
        constexpr ALWAYS_INLINE bool IsEqual(const Storage lhs[], const Storage rhs[]) {
            for (size_t i = 0; i < Count; i++) {
                if (lhs[i] != rhs[i]) {
                    return false;
                }
            }
            return true;
        }

        template<size_t Count, typename Storage>
        constexpr ALWAYS_INLINE bool IsAnySet(const Storage arr[]) {
            for (size_t i = 0; i < Count; i++) {
                if (arr[i]) {
                    return true;
                }
            }
            return false;
        }

        template<size_t Count, typename Storage>
        constexpr ALWAYS_INLINE int PopCount(const Storage arr[]) {
            int count = 0;
            for (size_t i = 0; i < Count; i++) {
                count += ::ams::util::PopCount(arr[i]);
            }
            return count;
        }

    }

    template<size_t N, typename T = void>
    struct BitFlagSet {
        static_assert(N > 0);
        using Storage = typename std::conditional<N <= BITSIZEOF(u32), u32, u64>::type;
        static constexpr size_t StorageBitCount = BITSIZEOF(Storage);
        static constexpr size_t StorageCount    = util::AlignUp(N, StorageBitCount) / StorageBitCount;
        Storage _storage[StorageCount];
        private:
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &SetImpl(s32 idx, Storage mask, bool en) {
                if (en) {
                    this->_storage[idx] |= mask;
                } else {
                    this->_storage[idx] &= ~mask;
                }
                return *this;
            }

            constexpr ALWAYS_INLINE bool TestImpl(s32 idx, Storage mask) const { return (this->_storage[idx] & mask) != 0; }
            constexpr ALWAYS_INLINE void Truncate() { TruncateIf(std::integral_constant<bool, N % StorageBitCount != 0>{}); }
            constexpr ALWAYS_INLINE void TruncateIf(std::true_type) { this->_storage[StorageCount - 1] &= MakeStorageMask(N) - 1; }
            constexpr ALWAYS_INLINE void TruncateIf(std::false_type) { /* ... */ }

            static constexpr ALWAYS_INLINE s32 GetStorageIndex(s32 idx) { return idx / StorageBitCount; }
            static constexpr ALWAYS_INLINE Storage MakeStorageMask(s32 idx) { return static_cast<Storage>(1) << (idx % StorageBitCount); }
        public:
            class Reference {
                friend struct BitFlagSet<N, T>;
                private:
                    BitFlagSet<N, T> *m_set;
                    s32 m_idx;
                private:
                    constexpr ALWAYS_INLINE Reference() : m_set(nullptr), m_idx(0) { /* ... */ }
                    constexpr ALWAYS_INLINE Reference(BitFlagSet<N, T> &s, s32 i) : m_set(std::addressof(s)), m_idx(i) { /* ... */ }
                public:
                    constexpr ALWAYS_INLINE Reference &operator=(bool en) { m_set->Set(m_idx, en); return *this; }
                    constexpr ALWAYS_INLINE Reference &operator=(const Reference &r) { m_set->Set(m_idx, r); return *this; }
                    constexpr ALWAYS_INLINE Reference &Negate() { m_set->Negate(m_idx); return *this; }
                    constexpr ALWAYS_INLINE operator bool() const { return m_set->Test(m_idx); }
                    constexpr ALWAYS_INLINE bool operator~() const { return !m_set->Test(m_idx); }
            };

            template<s32 _Index>
            struct Flag {
                static_assert(_Index < static_cast<s32>(N));
                friend struct BitFlagSet<N, T>;
                static constexpr s32 Index = _Index;
                static const BitFlagSet<N, T> Mask;
                private:
                    static constexpr s32 StorageIndex = Index / StorageBitCount;
                    static constexpr Storage StorageMask = static_cast<Storage>(1) << (Index % StorageBitCount);

                    template<size_t StorageCount>
                    struct SingleStorageTrait {
                        static_assert(StorageCount == 1);
                        using Type = Storage;
                    };
            };

            template<typename FlagType>
            constexpr ALWAYS_INLINE bool Test()       const { return this->TestImpl(FlagType::StorageIndex, FlagType::StorageMask); }
            constexpr ALWAYS_INLINE bool Test(s32 idx) const { return this->TestImpl(GetStorageIndex(idx), MakeStorageMask(idx)); }

            template<typename FlagType>
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Set(bool en = true) { return this->SetImpl(FlagType::StorageIndex, FlagType::StorageMask, en); }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Set(s32 idx, bool en = true) { return this->SetImpl(GetStorageIndex(idx), MakeStorageMask(idx), en); }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Set() { std::memset(this->_storage, ~0, sizeof(this->_storage)); this->Truncate(); return *this; }

            template<typename FlagType>
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Reset() { return this->Set<FlagType>(false); }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Reset(s32 idx) { return this->Set(idx, false); }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Reset() { std::memset(this->_storage, 0, sizeof(this->_storage)); this->Truncate(); return *this; }

            template<typename FlagType>
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Negate() { return this->Set<FlagType>(!this->Test<FlagType>()); }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Negate(s32 idx) { return this->Set(idx, !this->Test(idx)); }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &Negate() { ams::util::impl::NegateImpl<StorageCount>(this->_storage); this->Truncate(); return *this; }

            consteval static int GetCount() { return static_cast<int>(N); }

            constexpr ALWAYS_INLINE bool IsAnySet() const { return ams::util::impl::IsAnySet<StorageCount>(this->_storage); }
            constexpr ALWAYS_INLINE int  PopCount() const { return ams::util::impl::PopCount<StorageCount>(this->_storage); }
            constexpr ALWAYS_INLINE bool IsAllSet() const { return this->PopCount() == this->GetCount(); }
            constexpr ALWAYS_INLINE bool IsAllOff() const { return !this->IsAnySet(); }

            constexpr ALWAYS_INLINE bool operator[](s32 idx) const { return this->Test(idx); }
            constexpr ALWAYS_INLINE Reference operator[](s32 idx) { return Reference(*this, idx); }

            constexpr ALWAYS_INLINE bool operator==(const BitFlagSet<N, T> &rhs) const { return ams::util::impl::IsEqual<StorageCount>(this->_storage, rhs._storage); }
            constexpr ALWAYS_INLINE bool operator!=(const BitFlagSet<N, T> &rhs) const { return !(*this == rhs); }

            constexpr ALWAYS_INLINE BitFlagSet<N, T> operator~() const { BitFlagSet<N, T> tmp = *this; return tmp.Negate(); }

            constexpr ALWAYS_INLINE BitFlagSet<N, T> operator&(const BitFlagSet<N, T> &rhs) const { BitFlagSet<N, T> v = *this; v &= rhs; return v; }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> operator^(const BitFlagSet<N, T> &rhs) const { BitFlagSet<N, T> v = *this; v ^= rhs; return v; }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> operator|(const BitFlagSet<N, T> &rhs) const { BitFlagSet<N, T> v = *this; v |= rhs; return v; }

            constexpr ALWAYS_INLINE BitFlagSet<N, T> &operator&=(const BitFlagSet<N, T> &rhs) { ams::util::impl::AndImpl<StorageCount>(this->_storage, rhs._storage); return *this; }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &operator^=(const BitFlagSet<N, T> &rhs) { ams::util::impl::XorImpl<StorageCount>(this->_storage, rhs._storage); return *this; }
            constexpr ALWAYS_INLINE BitFlagSet<N, T> &operator|=(const BitFlagSet<N, T> &rhs) { ams::util::impl::OrImpl<StorageCount>(this->_storage, rhs._storage); return *this; }
    };

    template<size_t N, typename T>
    template<s32 Index>
    constexpr inline const BitFlagSet<N, T> BitFlagSet<N, T>::Flag<Index>::Mask = { { static_cast<typename SingleStorageTrait<BitFlagSet<N, T>::StorageCount>::Type>(1) << Index } };

    template<size_t N, typename T>
    constexpr BitFlagSet<N, T> MakeBitFlagSet() { return BitFlagSet<N, T>{}; }

    template<size_t N>
    constexpr BitFlagSet<N, void> MakeBitFlagSet() { return MakeBitFlagSet<N, void>(); }

}
