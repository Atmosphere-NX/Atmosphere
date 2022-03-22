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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_page_buffer.hpp>

namespace ams::kern {

    class KThread;
    class KProcess;

    class KThreadLocalPage : public util::IntrusiveRedBlackTreeBaseNode<KThreadLocalPage>, public KSlabAllocated<KThreadLocalPage> {
        public:
            static constexpr size_t RegionsPerPage = PageSize / ams::svc::ThreadLocalRegionSize;
            static_assert(RegionsPerPage > 0);
        private:
            KProcessAddress m_virt_addr;
            KProcess *m_owner;
            bool m_is_region_free[RegionsPerPage];
        public:
            explicit KThreadLocalPage(KProcessAddress addr) : m_virt_addr(addr), m_owner(nullptr) {
                for (size_t i = 0; i < util::size(m_is_region_free); i++) {
                    m_is_region_free[i] = true;
                }
            }

            explicit KThreadLocalPage() : KThreadLocalPage(Null<KProcessAddress>) { /* ... */ }

            constexpr ALWAYS_INLINE KProcessAddress GetAddress() const { return m_virt_addr; }
        public:
            using RedBlackKeyType = KProcessAddress;

            static constexpr ALWAYS_INLINE RedBlackKeyType GetRedBlackKey(const RedBlackKeyType  &v) { return v; }
            static constexpr ALWAYS_INLINE RedBlackKeyType GetRedBlackKey(const KThreadLocalPage &v) { return v.GetAddress(); }

            template<typename T> requires (std::same_as<T, KThreadLocalPage> || std::same_as<T, RedBlackKeyType>)
            static constexpr ALWAYS_INLINE int Compare(const T &lhs, const KThreadLocalPage &rhs) {
                const KProcessAddress lval = GetRedBlackKey(lhs);
                const KProcessAddress rval = GetRedBlackKey(rhs);

                if (lval < rval) {
                    return -1;
                } else if (lval == rval) {
                    return 0;
                } else {
                    return 1;
                }
            }
        private:
            constexpr ALWAYS_INLINE KProcessAddress GetRegionAddress(size_t i) {
                return this->GetAddress() + i * ams::svc::ThreadLocalRegionSize;
            }

            constexpr ALWAYS_INLINE bool Contains(KProcessAddress addr) {
                return this->GetAddress() <= addr && addr < this->GetAddress() + PageSize;
            }

            constexpr ALWAYS_INLINE size_t GetRegionIndex(KProcessAddress addr) {
                MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), ams::svc::ThreadLocalRegionSize));
                MESOSPHERE_ASSERT(this->Contains(addr));
                return (addr - this->GetAddress()) / ams::svc::ThreadLocalRegionSize;
            }
        public:
            Result Initialize(KProcess *process);
            Result Finalize();

            KProcessAddress Reserve();
            void Release(KProcessAddress addr);

            void *GetPointer() const;

            bool IsAllUsed() const {
                for (size_t i = 0; i < RegionsPerPage; i++) {
                    if (m_is_region_free[i]) {
                        return false;
                    }
                }
                return true;
            }

            bool IsAllFree() const {
                for (size_t i = 0; i < RegionsPerPage; i++) {
                    if (!m_is_region_free[i]) {
                        return false;
                    }
                }
                return true;
            }

            bool IsAnyUsed() const {
                return !this->IsAllFree();
            }

            bool IsAnyFree() const {
                return !this->IsAllUsed();
            }
    };

    /* Miscellaneous sanity checking. */
    static_assert(ams::svc::ThreadLocalRegionSize == THREAD_LOCAL_REGION_SIZE);
    static_assert(AMS_OFFSETOF(ams::svc::ThreadLocalRegion, message_buffer) == THREAD_LOCAL_REGION_MESSAGE_BUFFER);
    static_assert(AMS_OFFSETOF(ams::svc::ThreadLocalRegion, disable_count)  == THREAD_LOCAL_REGION_DISABLE_COUNT);
    static_assert(AMS_OFFSETOF(ams::svc::ThreadLocalRegion, interrupt_flag) == THREAD_LOCAL_REGION_INTERRUPT_FLAG);

}
