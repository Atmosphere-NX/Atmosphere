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
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    class KBlockInfoManager;

    class KBlockInfo : public util::IntrusiveListBaseNode<KBlockInfo> {
        private:
            KVirtualAddress m_address;
            size_t m_num_pages;
        public:
            constexpr KBlockInfo() : util::IntrusiveListBaseNode<KBlockInfo>(), m_address(), m_num_pages() { /* ... */ }

            constexpr void Initialize(KVirtualAddress addr, size_t np) {
                m_address = addr;
                m_num_pages = np;
            }

            constexpr KVirtualAddress GetAddress() const { return m_address; }
            constexpr size_t GetNumPages() const { return m_num_pages; }
            constexpr size_t GetSize() const { return this->GetNumPages() * PageSize; }
            constexpr KVirtualAddress GetEndAddress() const { return this->GetAddress() + this->GetSize(); }
            constexpr KVirtualAddress GetLastAddress() const { return this->GetEndAddress() - 1; }

            constexpr bool IsEquivalentTo(const KBlockInfo &rhs) const {
                return m_address == rhs.m_address && m_num_pages == rhs.m_num_pages;
            }

            constexpr bool operator==(const KBlockInfo &rhs) const {
                return this->IsEquivalentTo(rhs);
            }

            constexpr bool operator!=(const KBlockInfo &rhs) const {
                return !(*this == rhs);
            }

            constexpr bool IsStrictlyBefore(KVirtualAddress addr) const {
                const KVirtualAddress end = this->GetEndAddress();

                if (m_address != Null<KVirtualAddress> && end == Null<KVirtualAddress>) {
                    return false;
                }

                return end < addr;
            }

            constexpr bool operator<(KVirtualAddress addr) const {
                return this->IsStrictlyBefore(addr);
            }

            constexpr bool TryConcatenate(KVirtualAddress addr, size_t np) {
                if (addr != Null<KVirtualAddress> && addr == this->GetEndAddress()) {
                    m_num_pages += np;
                    return true;
                }
                return false;
            }
    };

    class KPageGroup {
        public:
            using BlockInfoList = util::IntrusiveListBaseTraits<KBlockInfo>::ListType;
            using iterator = BlockInfoList::const_iterator;
        private:
            BlockInfoList m_block_list;
            KBlockInfoManager *m_manager;
        public:
            explicit KPageGroup(KBlockInfoManager *m) : m_block_list(), m_manager(m) { /* ... */ }
            ~KPageGroup() { this->Finalize(); }

            void Finalize();

            iterator begin() const { return m_block_list.begin(); }
            iterator end() const { return m_block_list.end(); }
            bool empty() const { return m_block_list.empty(); }

            Result AddBlock(KVirtualAddress addr, size_t num_pages);
            void Open() const;
            void Close() const;

            size_t GetNumPages() const;

            bool IsEquivalentTo(const KPageGroup &rhs) const;

            bool operator==(const KPageGroup &rhs) const {
                return this->IsEquivalentTo(rhs);
            }

            bool operator!=(const KPageGroup &rhs) const {
                return !(*this == rhs);
            }
    };

    class KScopedPageGroup {
        private:
            const KPageGroup *m_pg;
        public:
            explicit ALWAYS_INLINE KScopedPageGroup(const KPageGroup *gp) : m_pg(gp) { if (m_pg) { m_pg->Open(); } }
            explicit ALWAYS_INLINE KScopedPageGroup(const KPageGroup &gp) : KScopedPageGroup(std::addressof(gp)) { /* ... */ }
            ALWAYS_INLINE ~KScopedPageGroup() { if (m_pg) { m_pg->Close(); } }

            ALWAYS_INLINE void CancelClose() {
                m_pg = nullptr;
            }
    };

}