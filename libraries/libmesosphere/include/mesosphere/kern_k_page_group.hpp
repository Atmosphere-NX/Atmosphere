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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    class KBlockInfoManager;

    class KPageGroup;

    class KBlockInfo {
        private:
            friend class KPageGroup;
        private:
            KBlockInfo *m_next;
            u32 m_page_index;
            u32 m_num_pages;
        public:
            KBlockInfo() : m_next(nullptr) { /* ... */ }

            constexpr ALWAYS_INLINE void Initialize(KPhysicalAddress addr, size_t np) {
                MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), PageSize));
                MESOSPHERE_ASSERT(static_cast<u32>(np) == np);

                m_page_index = GetInteger(addr) / PageSize;
                m_num_pages  = np;
            }

            constexpr ALWAYS_INLINE KPhysicalAddress GetAddress() const { return m_page_index * PageSize; }
            constexpr ALWAYS_INLINE size_t GetNumPages() const { return m_num_pages; }
            constexpr ALWAYS_INLINE size_t GetSize() const { return this->GetNumPages() * PageSize; }
            constexpr ALWAYS_INLINE KPhysicalAddress GetEndAddress() const { return (m_page_index + m_num_pages) * PageSize; }
            constexpr ALWAYS_INLINE KPhysicalAddress GetLastAddress() const { return this->GetEndAddress() - 1; }

            constexpr ALWAYS_INLINE KBlockInfo *GetNext() const { return m_next; }

            constexpr ALWAYS_INLINE bool IsEquivalentTo(const KBlockInfo &rhs) const {
                return m_page_index == rhs.m_page_index && m_num_pages == rhs.m_num_pages;
            }

            constexpr ALWAYS_INLINE bool operator==(const KBlockInfo &rhs) const {
                return this->IsEquivalentTo(rhs);
            }

            constexpr ALWAYS_INLINE bool operator!=(const KBlockInfo &rhs) const {
                return !(*this == rhs);
            }

            constexpr ALWAYS_INLINE bool IsStrictlyBefore(KPhysicalAddress addr) const {
                const KPhysicalAddress end = this->GetEndAddress();

                if (m_page_index != 0 && end == Null<KPhysicalAddress>) {
                    return false;
                }

                return end < addr;
            }

            constexpr ALWAYS_INLINE bool operator<(KPhysicalAddress addr) const {
                return this->IsStrictlyBefore(addr);
            }

            constexpr ALWAYS_INLINE bool TryConcatenate(KPhysicalAddress addr, size_t np) {
                if (addr != Null<KPhysicalAddress> && addr == this->GetEndAddress()) {
                    m_num_pages += np;
                    return true;
                }
                return false;
            }
        private:
            constexpr ALWAYS_INLINE void SetNext(KBlockInfo *next) {
                m_next = next;
            }
    };
    static_assert(sizeof(KBlockInfo) <= 0x10);

    class KPageGroup {
        public:
            class Iterator {
                public:
                    using iterator_category = std::forward_iterator_tag;
                    using value_type        = const KBlockInfo;
                    using difference_type   = std::ptrdiff_t;
                    using pointer           = value_type *;
                    using reference         = value_type &;
                private:
                    pointer m_node;
                public:
                    constexpr explicit ALWAYS_INLINE Iterator(pointer n) : m_node(n) { /* ... */ }

                    constexpr ALWAYS_INLINE bool operator==(const Iterator &rhs) const { return m_node == rhs.m_node; }
                    constexpr ALWAYS_INLINE bool operator!=(const Iterator &rhs) const { return !(*this == rhs); }

                    constexpr ALWAYS_INLINE pointer operator->() const { return m_node; }
                    constexpr ALWAYS_INLINE reference operator*() const { return *m_node; }

                    constexpr ALWAYS_INLINE Iterator &operator++() {
                        m_node = m_node->GetNext();
                        return *this;
                    }

                    constexpr ALWAYS_INLINE Iterator operator++(int) {
                        const Iterator it{*this};
                        ++(*this);
                        return it;
                    }
            };
        private:
            KBlockInfo *m_first_block;
            KBlockInfo *m_last_block;
            KBlockInfoManager *m_manager;
        public:
            explicit KPageGroup(KBlockInfoManager *m) : m_first_block(), m_last_block(), m_manager(m) { /* ... */ }
            ~KPageGroup() { this->Finalize(); }

            void CloseAndReset();
            void Finalize();

            ALWAYS_INLINE Iterator begin() const { return Iterator{m_first_block}; }
            ALWAYS_INLINE Iterator end() const { return Iterator{nullptr}; }
            ALWAYS_INLINE bool empty() const { return m_first_block == nullptr; }

            Result AddBlock(KPhysicalAddress addr, size_t num_pages);
            void Open() const;
            void Close() const;

            size_t GetNumPages() const;

            bool IsEquivalentTo(const KPageGroup &rhs) const;

            ALWAYS_INLINE bool operator==(const KPageGroup &rhs) const {
                return this->IsEquivalentTo(rhs);
            }

            ALWAYS_INLINE bool operator!=(const KPageGroup &rhs) const {
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