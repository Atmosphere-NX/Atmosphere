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
#include <stratosphere.hpp>
#include "mem_impl_heap_platform.hpp"
#include "mem_impl_heap_tls_heap_static.hpp"
#include "mem_impl_heap_tls_heap_central.hpp"

namespace ams::mem::impl::heap {

    namespace {

        void InitializeSpanPage(SpanPage *sp) {
            static_assert(SpanPage::MaxSpanCount <= BITSIZEOF(u64));
            constexpr size_t NumUnusedBits = BITSIZEOF(u64) - SpanPage::MaxSpanCount;

            sp->info.free_count   = SpanPage::MaxSpanCount;
            sp->info.is_sticky    = 0;
            sp->info.alloc_bitmap = (static_cast<u64>(1) << NumUnusedBits) - 1;

            ListClearLink(sp);

            sp->info.span_of_spanpage.start.u           = 0;
            sp->info.span_of_spanpage.num_pages         = 0;
            sp->info.span_of_spanpage.aux.small.objects = nullptr;
            sp->info.span_of_spanpage.object_count      = 0;
            sp->info.span_of_spanpage.page_class        = 0;
            sp->info.span_of_spanpage.status            = Span::Status_NotUsed;
            sp->info.span_of_spanpage.id                = 0;

            ListClearLink(std::addressof(sp->info.span_of_spanpage));
        }

        void RegisterSpan(SpanTable *span_table, Span *span) {
            const size_t idx = TlsHeapStatic::GetPageIndex(span->start.u - reinterpret_cast<uintptr_t>(span_table));
            span->page_class = 0;
            span_table->page_to_span[idx]                       = span;
            span_table->page_to_span[idx + span->num_pages - 1] = span;
        }

        void UnregisterSpan(SpanTable *span_table, Span *span) {
            AMS_ASSERT(span->page_class == 0);
            const size_t idx = TlsHeapStatic::GetPageIndex(span->start.u - reinterpret_cast<uintptr_t>(span_table));
            span_table->page_to_span[idx]                       = nullptr;
            span_table->page_to_span[idx + span->num_pages - 1] = nullptr;
        }

        void ChangeRangeOfSpan(SpanTable *span_table, Span *span, uintptr_t start, size_t new_pages) {
            const size_t idx = TlsHeapStatic::GetPageIndex(span->start.u - reinterpret_cast<uintptr_t>(span_table));
            if (span->start.u == start) {
                if (span->num_pages != 1) {
                    span_table->page_to_span[idx + span->num_pages - 1] = nullptr;
                }
                span_table->page_to_span[idx + new_pages - 1] = span;
                span->num_pages = new_pages;
            } else {
                span_table->page_to_span[idx] = nullptr;
                span_table->page_to_span[idx + span->num_pages - 1] = nullptr;

                const size_t new_idx = TlsHeapStatic::GetPageIndex(start - reinterpret_cast<uintptr_t>(span_table));
                span_table->page_to_span[new_idx] = span;
                span_table->page_to_span[new_idx + new_pages - 1] = span;

                span->start.u   = start;
                span->num_pages = new_pages;
            }
        }

        void MigrateSpan(SpanTable *span_table, Span *from, Span *to) {
            AMS_ASSERT(from != to);

            std::memcpy(to, from, sizeof(*from));

            from->status = Span::Status_NotUsed;

            if (from->list_next) {
                to->list_next = from->list_next;
                from->list_next->list_prev = to;
                from->list_next = nullptr;
            } else {
                to->list_next = nullptr;
            }

            if (from->list_prev) {
                to->list_prev = from->list_prev;
                from->list_prev->list_next = to;
                from->list_prev = nullptr;
            } else {
                to->list_prev = nullptr;
            }

            const size_t idx = TlsHeapStatic::GetPageIndex(to->start.u - reinterpret_cast<uintptr_t>(span_table));
            if (from->page_class) {
                for (size_t i = 0; i < to->num_pages; i++) {
                    span_table->page_to_span[idx + i] = to;
                }
            } else {
                span_table->page_to_span[idx] = to;
                span_table->page_to_span[idx + to->num_pages - 1] = to;
            }
        }

        bool IsNthSmallMemoryMarked(const Span *span, size_t n) {
            return (span->aux.small.is_allocated[n / BITSIZEOF(u64)] & (1ull << (n % BITSIZEOF(u64)))) != 0;
        }

        void MarkNthSmallMemory(Span *span, size_t n) {
            span->aux.small.is_allocated[n / BITSIZEOF(u64)] |= (1ull << (n % BITSIZEOF(u64)));
        }

        void UnmarkNthSmallMemory(Span *span, size_t n) {
            span->aux.small.is_allocated[n / BITSIZEOF(u64)] &= ~(1ull << (n % BITSIZEOF(u64)));
        }

        void *AllocateSmallMemory(Span *span) {
            Span::SmallMemory *sm = span->aux.small.objects;
            const size_t chunk_size = TlsHeapStatic::GetChunkSize(span->page_class);
            const uintptr_t span_end = span->start.u + (span->num_pages * TlsHeapStatic::PageSize);
            if (span->start.u <= reinterpret_cast<uintptr_t>(sm) && reinterpret_cast<uintptr_t>(sm) < span_end) {
                const size_t idx = (reinterpret_cast<uintptr_t>(sm) - span->start.u) / chunk_size;
                if (reinterpret_cast<uintptr_t>(sm) == (span->start.u + idx * chunk_size) && !IsNthSmallMemoryMarked(span, idx)) {
                    MarkNthSmallMemory(span, idx);
                    span->aux.small.objects = sm->next;
                    sm->next = nullptr;
                    span->object_count++;
                    return sm;
                }
            } else if (sm == nullptr) {
                return nullptr;
            }

            /* Data corruption error. */
            AMS_ASSERT(false);
            span->aux.small.objects = nullptr;
            return nullptr;
        }

        struct MangledSmallMemory {
            Span::SmallMemory *from;
            Span::SmallMemory *to;
        };

        size_t AllocateSmallMemory(Span *span, TlsHeapCache *cache, size_t n, MangledSmallMemory *memlist) {
            auto ManglePointer = [cache](void *ptr) ALWAYS_INLINE_LAMBDA {
                return static_cast<Span::SmallMemory *>(cache->ManglePointer(ptr));
            };

            Span::SmallMemory *sm = span->aux.small.objects;
            if (sm) {
                size_t count = 0;
                memlist->from = ManglePointer(sm);

                const size_t chunk_size = TlsHeapStatic::GetChunkSize(span->page_class);
                const uintptr_t span_end = span->start.u + (span->num_pages * TlsHeapStatic::PageSize);
                while (span->start.u <= reinterpret_cast<uintptr_t>(sm) && reinterpret_cast<uintptr_t>(sm) < span_end) {
                    const size_t idx = (reinterpret_cast<uintptr_t>(sm) - span->start.u) / chunk_size;
                    if (span->start.u + idx * chunk_size != reinterpret_cast<uintptr_t>(sm)) {
                        break;
                    }
                    if (IsNthSmallMemoryMarked(span, idx)) {
                        break;
                    }
                    MarkNthSmallMemory(span, idx);
                    count++;

                    Span::SmallMemory *next = sm->next;
                    sm->next = ManglePointer(next);
                    if (count >= n || next == nullptr) {
                        memlist->to = sm;
                        memlist->to->next = nullptr;
                        span->aux.small.objects = next;
                        span->object_count += count;
                        return count;
                    }

                    sm = next;
                }

                /* Data corruption error. */
                Span::SmallMemory *prev = span->aux.small.objects;
                Span::SmallMemory *cur  = span->aux.small.objects;
                while (cur != sm) {
                    prev = cur;
                    cur = ManglePointer(cur->next);
                }

                memlist->to = ManglePointer(prev);
                memlist->to->next = nullptr;
                span->aux.small.objects = nullptr;
                span->object_count += count;
                return count;
            } else {
                memlist->from = nullptr;
                memlist->to   = nullptr;
                return 0;
            }
        }

        void ReleaseSmallMemory(Span *span, void *ptr) {
            AMS_ASSERT(span->object_count > 0);

            const size_t span_ofs = reinterpret_cast<uintptr_t>(ptr) - span->start.u;
            const size_t chunk_size = TlsHeapStatic::GetChunkSize(span->page_class);
            const size_t span_idx = span_ofs / chunk_size;
            if (span_ofs != (span_idx * chunk_size)) {
                /* Invalid pointer. Do the best we can. */
                ptr = reinterpret_cast<void *>(span->start.u + span_idx * chunk_size);
            }
            if (IsNthSmallMemoryMarked(span, span_idx)) {
                UnmarkNthSmallMemory(span, span_idx);

                Span::SmallMemory *sm = reinterpret_cast<Span::SmallMemory *>(ptr);
                sm->next = span->aux.small.objects;
                span->aux.small.objects = sm;
                span->object_count--;
            } else {
                /* Double free error. */
                /* TODO: Anything? */
            }
        }

        void SpanToSmallMemorySpan(SpanTable *span_table, Span *span, size_t cls) {
            AMS_ASSERT(cls != 0 && span->page_class == 0);

            const size_t span_idx = TlsHeapStatic::GetPageIndex(span->start.u - reinterpret_cast<uintptr_t>(span_table));
            Span **table_entry = std::addressof(span_table->page_to_span[span_idx]);

            span->page_class = cls;
            for (size_t i = 0; i < span->num_pages; i++) {
                table_entry[i] = span;
            }

            std::atomic_thread_fence(std::memory_order_release);

            std::memset(std::addressof(span_table->pageclass_cache[span_idx]), cls, span->num_pages);
        }

        void SmallMemorySpanToSpan(SpanTable *span_table, Span *span) {
            AMS_ASSERT(span->page_class != 0);

            const size_t span_idx = TlsHeapStatic::GetPageIndex(span->start.u - reinterpret_cast<uintptr_t>(span_table));
            Span **table_entry = std::addressof(span_table->page_to_span[span_idx]);

            for (size_t i = 0; i < span->num_pages; i++) {
                table_entry[i] = nullptr;
            }
            span->page_class = 0;

            table_entry[0] = span;
            table_entry[span->num_pages - 1] = span;

            std::atomic_thread_fence(std::memory_order_release);

            std::memset(std::addressof(span_table->pageclass_cache[span_idx]), 0, span->num_pages);
        }

        void InitSmallMemorySpan(Span *span, size_t cls, bool for_system, int id) {
            AMS_ASSERT(cls != 0);
            span->page_class = cls;

            AMS_ASSERT(span->status == Span::Status_InUse);
            if (for_system) {
                span->status = Span::Status_InUseSystem;
            }

            span->aux.small.objects = span->start.sm;
            span->object_count = 0;
            span->id = id;

            const size_t chunk_size = TlsHeapStatic::GetChunkSize(cls);
            const size_t num_chunks = (span->num_pages * TlsHeapStatic::PageSize) / chunk_size;
            AMS_ASSERT(num_chunks <= sizeof(span->aux.small.is_allocated) * BITSIZEOF(u8));

            Span::SmallMemory *last = reinterpret_cast<Span::SmallMemory *>(span->start.u + num_chunks * chunk_size);
            Span::SmallMemory *prev = reinterpret_cast<Span::SmallMemory *>(span->start.u);
            for (Span::SmallMemory *cur = reinterpret_cast<Span::SmallMemory *>(span->start.u + chunk_size); cur != last; cur = reinterpret_cast<Span::SmallMemory *>(reinterpret_cast<uintptr_t>(cur) + chunk_size)) {
                prev->next = cur;
                prev = cur;
            }
            prev->next = nullptr;
            std::memset(span->aux.small.is_allocated, 0, sizeof(span->aux.small.is_allocated));
        }

    }

    errno_t TlsHeapCentral::Initialize(void *start, size_t size, bool use_virtual_memory) {
        AMS_ASSERT(size > 0);
        AMS_ASSERT(TlsHeapStatic::IsPageAligned(start));
        AMS_ASSERT(TlsHeapStatic::IsPageAligned(size));

        /* Clear lists. */
        ListClearLink(std::addressof(m_spanpage_list));
        ListClearLink(std::addressof(m_full_spanpage_list));
        for (size_t i = 0; i < util::size(m_freelists); i++) {
            ListClearLink(std::addressof(m_freelists[i]));
        }
        for (size_t i = 0; i < util::size(m_freelists_bitmap); i++) {
            m_freelists_bitmap[i] = 0;
        }
        for (size_t i = 0; i < util::size(m_smallmem_lists); i++) {
            ListClearLink(std::addressof(m_smallmem_lists[i]));
        }

        /* Setup span table. */
        const size_t total_pages         = TlsHeapStatic::GetPageIndex(size);
        const size_t n                   = total_pages * sizeof(Span *);
        m_span_table.total_pages     = total_pages;
        m_span_table.page_to_span    = reinterpret_cast<Span **>(static_cast<u8 *>(start) + sizeof(*this));
        m_span_table.pageclass_cache = static_cast<u8 *>(start) + sizeof(*this) + n;

        u8 *meta_end = m_span_table.pageclass_cache + total_pages;
        size_t num_physical_page_flags;
        if (use_virtual_memory) {
            m_physical_page_flags = meta_end;
            const uintptr_t phys_start = TlsHeapStatic::AlignDownPhysicalPage(reinterpret_cast<uintptr_t>(start));
            const uintptr_t phys_end   = TlsHeapStatic::AlignUpPhysicalPage(reinterpret_cast<uintptr_t>(start) + size);
            num_physical_page_flags    = TlsHeapStatic::GetPhysicalPageIndex(phys_end - phys_start);
            meta_end = TlsHeapStatic::AlignUpPage(meta_end + num_physical_page_flags);
        } else {
            m_physical_page_flags = nullptr;
            num_physical_page_flags   = 0;
            meta_end = TlsHeapStatic::AlignUpPage(meta_end);
        }
        AMS_ASSERT(TlsHeapStatic::IsPageAligned(meta_end));

        if (use_virtual_memory) {
            const uintptr_t phys_end = TlsHeapStatic::AlignUpPhysicalPage(reinterpret_cast<uintptr_t>(meta_end) + TlsHeapStatic::PageSize);
            size_t phys_size = phys_end - reinterpret_cast<uintptr_t>(start);
            phys_size = std::min(phys_size, size);
            if (auto err = AllocatePhysicalMemory(start, phys_size); err != 0) {
                m_span_table.total_pages = 0;
                return err;
            }
            std::memset(m_physical_page_flags, 0, num_physical_page_flags);
            std::memset(m_physical_page_flags, 1, TlsHeapStatic::GetPhysicalPageIndex(phys_end) - TlsHeapStatic::GetPhysicalPageIndex(reinterpret_cast<uintptr_t>(start)));
        }

        std::memset(m_span_table.page_to_span, 0, n);
        std::memset(m_span_table.pageclass_cache, 0, total_pages);

        SpanPage *span_page = reinterpret_cast<SpanPage *>(meta_end);
        InitializeSpanPage(span_page);
        ListInsertAfter(std::addressof(m_spanpage_list), span_page);

        meta_end += TlsHeapStatic::PageSize;
        AMS_ASSERT(TlsHeapStatic::IsPageAligned(meta_end));

        /* Setup the spans. */
        Span *span = this->AllocateSpanFromSpanPage(span_page);
        AMS_ASSERT(span != nullptr);

        Span *span_admin = this->AllocateSpanFromSpanPage(span_page);
        AMS_ASSERT(span_admin != nullptr);

        span_page->info.is_sticky = 1;

        span->start.u           = reinterpret_cast<uintptr_t>(meta_end);
        span->num_pages         = TlsHeapStatic::GetPageIndex(reinterpret_cast<uintptr_t>(start) + size - reinterpret_cast<uintptr_t>(meta_end));
        span->aux.small.objects = nullptr;
        span->object_count      = 0;
        span->status            = Span::Status_InFreeList;
        span->id                = 0;

        span_page->info.span_of_spanpage.start.u           = reinterpret_cast<uintptr_t>(span_page);
        span_page->info.span_of_spanpage.num_pages         = 1;
        span_page->info.span_of_spanpage.aux.small.objects = nullptr;
        span_page->info.span_of_spanpage.object_count      = 0;
        span_page->info.span_of_spanpage.status            = Span::Status_InUseSystem;
        span_page->info.span_of_spanpage.id                = 0;

        span_admin->start.u           = reinterpret_cast<uintptr_t>(start);
        span_admin->num_pages         = TlsHeapStatic::GetPageIndex(reinterpret_cast<uintptr_t>(span_page) - reinterpret_cast<uintptr_t>(start));
        span_admin->aux.small.objects = nullptr;
        span_admin->object_count      = 0;
        span_admin->status            = Span::Status_InUseSystem;
        span_admin->id                = 0;

        RegisterSpan(std::addressof(m_span_table), span_admin);
        RegisterSpan(std::addressof(m_span_table), std::addressof(span_page->info.span_of_spanpage));
        RegisterSpan(std::addressof(m_span_table), span);

        this->AddToFreeBlockList(span);

        m_num_threads = 1;
        m_static_thread_quota = std::min((m_span_table.total_pages * TlsHeapStatic::PageSize) / sizeof(void *), 2_MB);
        m_dynamic_thread_quota = m_static_thread_quota;
        m_use_virtual_memory = use_virtual_memory;

        return 0;
    }

    bool TlsHeapCentral::IsClean() {
        std::scoped_lock lk(m_lock);

        this->MakeFreeSpan(std::numeric_limits<size_t>::max());

        Span *span = this->GetFirstSpan();
        Span *next = GetNextSpan(std::addressof(m_span_table), span);
        if (next && next->status == Span::Status_InFreeList && GetNextSpan(std::addressof(m_span_table), next) == nullptr) {
            return true;
        } else {
            return false;
        }
    }

    errno_t TlsHeapCentral::ReallocateLargeMemory(void *ptr, size_t size, void **p) {
        if (!TlsHeapStatic::IsPageAligned(ptr)) {
            return EFAULT;
        }

        AMS_ASSERT(size <= MaxSize);

        /* NOTE: This function uses locks unsafely (unscoped) */

        m_lock.Lock();

        Span *ptr_span = GetSpanFromPointer(std::addressof(m_span_table), ptr);
        if (!ptr_span) {
            AMS_ASSERT(ptr_span != nullptr);
            m_lock.Unlock();
            return EFAULT;
        }

        const size_t num_pages = TlsHeapStatic::GetPageIndex(size + TlsHeapStatic::PageSize - 1);
        if (ptr_span->num_pages != num_pages) {
            if (ptr_span->num_pages >= num_pages) {
                Span *span = this->AllocateSpanStruct();
                if (span != nullptr) {
                    span->start.u = ptr_span->start.u + (num_pages * TlsHeapStatic::PageSize);
                    span->num_pages = ptr_span->num_pages - num_pages;
                    span->id = 0;
                    span->status = Span::Status_InUse;
                    ChangeRangeOfSpan(std::addressof(m_span_table), ptr_span, ptr_span->start.u, num_pages);
                    RegisterSpan(std::addressof(m_span_table), span);
                    this->FreePagesImpl(span);
                }
            } else {
                Span *next_span = GetNextSpan(std::addressof(m_span_table), ptr_span);
                if (!next_span || next_span->status != Span::Status_InFreeList || next_span->num_pages < num_pages - ptr_span->num_pages) {
                    m_lock.Unlock();

                    m_lock.Lock();

                    Span *span = this->AllocatePagesImpl(num_pages);
                    if (span) {
                        *p = span->start.p;
                        std::memcpy(std::addressof(span->aux.large), std::addressof(ptr_span->aux.large), sizeof(ptr_span->aux.large));
                    } else {
                        *p = nullptr;
                    }

                    m_lock.Unlock();
                    if (*p == nullptr) {
                        return ENOMEM;
                    }

                    std::memcpy(*p, ptr, num_pages * TlsHeapStatic::PageSize);

                    m_lock.Lock();
                    this->FreePagesImpl(ptr_span);
                    m_lock.Unlock();

                    return 0;
                }

                if (m_use_virtual_memory && this->AllocatePhysical(next_span->start.p, (num_pages - ptr_span->num_pages) * TlsHeapStatic::PageSize)) {
                    m_lock.Unlock();
                    return ENOMEM;
                }

                this->RemoveFromFreeBlockList(next_span);
                if (next_span->num_pages == num_pages - ptr_span->num_pages) {
                    UnregisterSpan(std::addressof(m_span_table), next_span);
                    ChangeRangeOfSpan(std::addressof(m_span_table), ptr_span, ptr_span->start.u, num_pages);
                    SpanPage *sp = GetSpanPage(next_span);
                    this->FreeSpanToSpanPage(next_span, sp);
                    this->DestroySpanPageIfEmpty(sp, false);
                } else {
                    const uintptr_t new_end = ptr_span->start.u + num_pages * TlsHeapStatic::PageSize;
                    const size_t new_num_pages = next_span->num_pages - (num_pages - ptr_span->num_pages);
                    ChangeRangeOfSpan(std::addressof(m_span_table), next_span, new_end, new_num_pages);
                    ChangeRangeOfSpan(std::addressof(m_span_table), ptr_span, ptr_span->start.u, num_pages);
                    this->MergeIntoFreeList(next_span);
                }
            }
        }

        *p = ptr;
        m_lock.Unlock();
        return 0;
    }

    errno_t TlsHeapCentral::ShrinkLargeMemory(void *ptr, size_t size) {
        if (!TlsHeapStatic::IsPageAligned(ptr)) {
            return EFAULT;
        }

        AMS_ASSERT(size <= MaxSize);

        std::scoped_lock lk(m_lock);

        Span *ptr_span = GetSpanFromPointer(std::addressof(m_span_table), ptr);
        if (!ptr_span) {
            AMS_ASSERT(ptr_span != nullptr);
            return EFAULT;
        }

        const size_t num_pages = TlsHeapStatic::GetPageIndex(size + TlsHeapStatic::PageSize - 1);
        if (ptr_span->num_pages != num_pages) {
            if (ptr_span->num_pages < num_pages) {
                return EINVAL;
            }
            Span *span = this->AllocateSpanStruct();
            if (span != nullptr) {
                span->start.u = ptr_span->start.u + (num_pages * TlsHeapStatic::PageSize);
                span->num_pages = ptr_span->num_pages - num_pages;
                span->id = 0;
                span->status = Span::Status_InUse;
                ChangeRangeOfSpan(std::addressof(m_span_table), ptr_span, ptr_span->start.u, num_pages);
                RegisterSpan(std::addressof(m_span_table), span);
                this->FreePagesImpl(span);
            }
        }

        return 0;
    }

    void TlsHeapCentral::CalculateHeapHash(HeapHash *out) {
        size_t alloc_count = 0;
        size_t alloc_size  = 0;
        size_t hash        = 0;

        {
            std::scoped_lock lk(m_lock);

            for (Span *span = GetSpanFromPointer(std::addressof(m_span_table), this); span != nullptr; span = GetNextSpan(std::addressof(m_span_table), span)) {
                if (span->status != Span::Status_InUse) {
                    continue;
                }

                const size_t size = span->num_pages * TlsHeapStatic::PageSize;

                if (span->page_class == 0) {
                    alloc_count++;
                    alloc_size += size;
                    hash       += std::hash<size_t>{}(size) + std::hash<size_t>{}(span->start.u);
                } else {
                    const size_t chunk_size = TlsHeapStatic::GetChunkSize(span->page_class);

                    const size_t n = size / chunk_size;
                    AMS_ASSERT(n <= TlsHeapStatic::PageSize);

                    static_assert(util::IsAligned(TlsHeapStatic::PageSize, BITSIZEOF(FreeListAvailableWord)));
                    FreeListAvailableWord flags[TlsHeapStatic::PageSize / BITSIZEOF(FreeListAvailableWord)];
                    std::memset(flags, 0, sizeof(flags));

                    for (Span::SmallMemory *sm = span->aux.small.objects; sm != nullptr; sm = sm->next) {
                        const size_t idx = (reinterpret_cast<uintptr_t>(sm) - span->start.u) / chunk_size;
                        flags[FreeListAvailableIndex(idx)] |= FreeListAvailableMask(idx);
                    }

                    for (size_t i = 0; i < n; i++) {
                        if (!(flags[FreeListAvailableIndex(i)] & FreeListAvailableMask(i))) {
                            alloc_count++;
                            alloc_size += chunk_size;
                            hash       += std::hash<size_t>{}(chunk_size) + std::hash<size_t>{}(span->start.u + chunk_size * n);
                        }
                    }
                }
            }
        }

        out->alloc_count = alloc_count;
        out->alloc_size  = alloc_size;
        out->hash        = hash;
    }

    SpanPage *TlsHeapCentral::AllocateSpanPage() {
        Span *span = this->SearchFreeSpan(1);
        if (span == nullptr) {
            return nullptr;
        }
        AMS_ASSERT(span->page_class == 0);

        if (m_use_virtual_memory && this->AllocatePhysical(span->start.p, TlsHeapStatic::PageSize) != 0) {
            return nullptr;
        }

        SpanPage *sp = static_cast<SpanPage *>(span->start.p);

        InitializeSpanPage(sp);
        Span *new_span = GetSpanPageSpan(sp);
        if (span->num_pages == 1) {
            this->RemoveFromFreeBlockList(span);
            MigrateSpan(std::addressof(m_span_table), span, new_span);
            AMS_ASSERT(new_span->num_pages == 1);
            new_span->status = Span::Status_InUseSystem;

            SpanPage *sp_of_span = GetSpanPage(span);
            this->FreeSpanToSpanPage(span, sp_of_span);
            this->DestroySpanPageIfEmpty(sp_of_span, false);
        } else {
            new_span->start.u = span->start.u;
            new_span->num_pages = 1;
            new_span->status = Span::Status_InUseSystem;
            new_span->id = 0;

            if (span->num_pages - 1 < FreeListCount) {
                this->RemoveFromFreeBlockList(span);
            }
            ChangeRangeOfSpan(std::addressof(m_span_table), span, span->start.u + TlsHeapStatic::PageSize, span->num_pages - 1);
            RegisterSpan(std::addressof(m_span_table), new_span);

            if (span->num_pages < FreeListCount) {
                this->AddToFreeBlockList(span);
            }
        }

        ListInsertAfter(std::addressof(m_spanpage_list), sp);
        return sp;
    }

    Span *TlsHeapCentral::AllocateSpanFromSpanPage(SpanPage *sp) {
        const size_t span_idx = __builtin_clzll(~sp->info.alloc_bitmap);
        AMS_ASSERT(span_idx < SpanPage::MaxSpanCount);

        constexpr u64 TopBit = static_cast<u64>(1) << (BITSIZEOF(u64) - 1);
        sp->info.alloc_bitmap |= (TopBit >> span_idx);
        sp->info.free_count--;

        Span *span = std::addressof(sp->spans[span_idx]);
        ListClearLink(span);
        span->status       = Span::Status_NotUsed;
        span->page_class   = 0;
        span->object_count = 0;

        if (sp->info.free_count == 0) {
            ListRemoveSelf(sp);
            ListInsertAfter(std::addressof(m_full_spanpage_list), sp);
        }

        return span;
    }

    Span *TlsHeapCentral::SplitSpan(Span *span, size_t num_pages, Span *new_span) {
        AMS_ASSERT(span->status == Span::Status_InFreeList);
        AMS_ASSERT(span->num_pages > num_pages);

        const size_t remaining_pages = span->num_pages - num_pages;

        uintptr_t new_start, old_start;
        if (num_pages < FreeListCount) {
            new_start = span->start.u;
            old_start = span->start.u + (num_pages * TlsHeapStatic::PageSize);
        } else {
            new_start = span->start.u + (remaining_pages * TlsHeapStatic::PageSize);
            old_start = span->start.u;
        }


        if (remaining_pages >= FreeListCount) {
            ChangeRangeOfSpan(std::addressof(m_span_table), span, old_start, remaining_pages);
        } else {
            this->RemoveFromFreeBlockList(span);
            ChangeRangeOfSpan(std::addressof(m_span_table), span, old_start, remaining_pages);
            this->AddToFreeBlockList(span);
        }

        new_span->start.u = new_start;
        new_span->num_pages = num_pages;
        new_span->page_class = 0;
        new_span->status = Span::Status_InUse;
        new_span->id = 0;
        new_span->aux.large_clear.zero = 0;
        span->aux.large_clear.zero = 0;

        if (m_use_virtual_memory && this->AllocatePhysical(new_span->start.p, new_span->num_pages * TlsHeapStatic::PageSize) != 0) {
            new_span->status = Span::Status_InFreeList;
            this->MergeIntoFreeList(new_span);
            return nullptr;
        }

        RegisterSpan(std::addressof(m_span_table), new_span);
        return new_span;
    }

    void TlsHeapCentral::MergeFreeSpans(Span *span, Span *span_to_merge, uintptr_t start) {
        const size_t total_pages = span->num_pages + span_to_merge->num_pages;
        UnregisterSpan(std::addressof(m_span_table), span_to_merge);
        SpanPage *span_page = GetSpanPage(span_to_merge);
        this->FreeSpanToSpanPage(span_to_merge, span_page);
        ChangeRangeOfSpan(std::addressof(m_span_table), span, start, total_pages);
    }

    bool TlsHeapCentral::DestroySpanPageIfEmpty(SpanPage *sp, bool full) {
        if (sp->info.is_sticky) {
            if (!(full || sp->info.free_count > 0x10)) {
                return false;
            }
            if (!sp->info.free_count) {
                return false;
            }

            size_t first = this->FreeListFirstNonEmpty(0);

            while (first < FreeListCount) {
                for (Span *target = ListGetNext(std::addressof(m_freelists[first])); target; target = ListGetNext(target)) {
                    AMS_ASSERT(target->status == Span::Status_InFreeList);

                    SpanPage *target_sp = GetSpanPage(target);
                    if (target_sp != sp) {
                        Span *new_span = this->AllocateSpanFromSpanPage(sp);
                        AMS_ASSERT(new_span != nullptr);

                        MigrateSpan(std::addressof(m_span_table), target, new_span);
                        this->FreeSpanToSpanPage(target, target_sp);
                        this->DestroySpanPageIfEmpty(target_sp, full);
                    }
                }
                first = this->FreeListFirstNonEmpty(first + 1);
            }

            return false;
        } else if (sp->info.free_count == SpanPage::MaxSpanCount) {
            if (!ListGetNext(sp) && !ListGetPrev(sp)) {
                return false;
            }
            SpanPage *other_sp = GetSpanPage(this->GetFirstSpan());

            SpanPage *target;
            if (other_sp->info.free_count > 0x10) {
                target = other_sp;
            } else {
                for (target = ListGetNext(std::addressof(m_spanpage_list)); target && (target == sp || !target->info.free_count); target = ListGetNext(target)) {
                    /* ... */
                }
                if (!target) {
                    if (!other_sp->info.free_count) {
                        return false;
                    }
                    target = other_sp;
                }
            }

            Span *new_span = this->AllocateSpanFromSpanPage(target);
            AMS_ASSERT(new_span != nullptr);

            MigrateSpan(std::addressof(m_span_table), GetSpanPageSpan(sp), new_span);

            ListRemoveSelf(sp);
            this->FreePagesImpl(new_span);
            return true;
        } else {
            return false;
        }
    }

    Span *TlsHeapCentral::GetFirstSpan() const {
        Span *span = GetSpanFromPointer(std::addressof(m_span_table), reinterpret_cast<const void *>(this));
        AMS_ASSERT(span != nullptr);
        return GetNextSpan(std::addressof(m_span_table), span);
    }

    Span *TlsHeapCentral::MakeFreeSpan(size_t num_pages) {
        while (true) {
            SpanPage *sp;
            for (sp = ListGetNext(std::addressof(m_spanpage_list)); sp && !this->DestroySpanPageIfEmpty(sp, true); sp = ListGetNext(sp)) {
                /* ... */
            }
            if (!sp) {
                break;
            }
            if (Span *span = this->SearchFreeSpan(num_pages); span != nullptr) {
                return span;
            }
        }
        return nullptr;
    }

    Span *TlsHeapCentral::SearchFreeSpan(size_t num_pages) const {
        size_t start = FreeListCount - 1;
        if (num_pages < FreeListCount) {
            start = this->FreeListFirstNonEmpty(num_pages - 1);
            if (start == FreeListCount) {
                return nullptr;
            }
        }

        Span *cur  = ListGetNext(std::addressof(m_freelists[start]));
        Span *best = cur;
        if (start == FreeListCount - 1) {
            if (num_pages >= FreeListCount) {
                best = nullptr;
                while (cur) {
                    if (num_pages <= cur->num_pages) {
                        if (best) {
                            if (cur->num_pages >= best->num_pages) {
                                if (cur->num_pages == best->num_pages && cur->start.u < best->start.u) {
                                    best = cur;
                                }
                            } else {
                                best = cur;
                            }
                        } else {
                            best = cur;
                        }
                    }
                    cur = ListGetNext(cur);
                }
            } else {
                while (cur) {
                    if (cur->num_pages >= best->num_pages) {
                        if (cur->num_pages == best->num_pages && cur->start.u < best->start.u) {
                            best = cur;
                        }
                    } else {
                        best = cur;
                    }
                    cur = ListGetNext(cur);
                }
            }
        }

        if (best != nullptr) {
            AMS_ASSERT(best->status == Span::Status_InFreeList);
        }

        return best;
    }

    void TlsHeapCentral::FreeSpanToSpanPage(Span *span, SpanPage *sp) {
        span->status = Span::Status_NotUsed;

        const size_t span_idx = span - sp->spans;

        constexpr u64 TopBit = static_cast<u64>(1) << (BITSIZEOF(u64) - 1);
        sp->info.alloc_bitmap &= ~(TopBit >> span_idx);
        if ((++(sp->info.free_count)) == 1) {
            ListRemoveSelf(sp);
            ListInsertAfter(std::addressof(m_spanpage_list), sp);
        }
    }

    void TlsHeapCentral::FreeSpanToSpanPage(Span *span) {
        return this->FreeSpanToSpanPage(span, GetSpanPage(span));
    }

    void TlsHeapCentral::MergeIntoFreeList(Span *&span) {
        AMS_ASSERT(!span->list_prev && !span->list_next);
        AMS_ASSERT(span->status != Span::Status_InUse);

        Span *prev_span = GetPrevSpan(std::addressof(m_span_table), span);
        Span *next_span = GetNextSpan(std::addressof(m_span_table), span);
        const bool prev_free  = prev_span && prev_span->status == Span::Status_InFreeList;
        const bool prev_small = prev_span && prev_span->num_pages < FreeListCount;
        const bool next_free  = next_span && next_span->status == Span::Status_InFreeList;
        const bool next_small = next_span && next_span->num_pages < FreeListCount;

        if (prev_free) {
            if (next_free) {
                if (prev_small) {
                    if (next_small) {
                        this->RemoveFromFreeBlockList(prev_span);
                        this->RemoveFromFreeBlockList(next_span);
                        this->MergeFreeSpans(prev_span, span, prev_span->start.u);
                        this->MergeFreeSpans(prev_span, next_span, prev_span->start.u);
                        this->AddToFreeBlockList(prev_span);
                        span = prev_span;
                    } else {
                        this->RemoveFromFreeBlockList(prev_span);
                        this->MergeFreeSpans(prev_span, span, prev_span->start.u);
                        this->MergeFreeSpans(next_span, prev_span, prev_span->start.u);
                        span = next_span;
                    }
                } else {
                    this->RemoveFromFreeBlockList(next_span);
                    this->MergeFreeSpans(prev_span, span, prev_span->start.u);
                    this->MergeFreeSpans(prev_span, next_span, prev_span->start.u);
                    span = prev_span;
                }
            } else {
                if (prev_small) {
                    this->RemoveFromFreeBlockList(prev_span);
                    this->MergeFreeSpans(prev_span, span, prev_span->start.u);
                    this->AddToFreeBlockList(prev_span);
                } else {
                    this->MergeFreeSpans(prev_span, span, prev_span->start.u);
                }
                span = prev_span;
            }
        } else if (next_free) {
            if (next_small) {
                this->RemoveFromFreeBlockList(next_span);
                this->MergeFreeSpans(span, next_span, span->start.u);
                this->AddToFreeBlockList(span);
            } else {
                this->MergeFreeSpans(next_span, span, span->start.u);
                span = next_span;
            }
        } else {
            this->AddToFreeBlockList(span);
        }

        AMS_ASSERT(GetSpanPageSpan(GetSpanPage(span)) != span);
        AMS_ASSERT(span->status == Span::Status_InFreeList);
    }

    errno_t TlsHeapCentral::AllocatePhysical(void *start, size_t size) {
        /* TODO: Implement physical tls heap central logic. */
        AMS_UNUSED(start, size);
        return 0;
    }

    errno_t TlsHeapCentral::FreePhysical(void *start, size_t size) {
        const uintptr_t start_alignup = TlsHeapStatic::AlignUpPhysicalPage(reinterpret_cast<uintptr_t>(start));
        const uintptr_t end_aligndown = TlsHeapStatic::AlignDownPhysicalPage(reinterpret_cast<uintptr_t>(start) + size);
        if (end_aligndown <= start_alignup) {
            return 0;
        }

        const uintptr_t phys_heap_start = TlsHeapStatic::AlignDownPhysicalPage(reinterpret_cast<uintptr_t>(this));
        const uintptr_t i = TlsHeapStatic::GetPhysicalPageIndex(start_alignup - phys_heap_start);
        const uintptr_t idx_end = TlsHeapStatic::GetPhysicalPageIndex(end_aligndown - phys_heap_start);
        AMS_ASSERT(i < idx_end);

        if (i + 1 == idx_end) {
            if (m_physical_page_flags[i]) {
                m_physical_page_flags[i] = 2;
            }
        } else {
            const void *set_flag = ::memchr(std::addressof(m_physical_page_flags[i]), 1, idx_end - i);
            if (set_flag) {
                const uintptr_t set_idx = reinterpret_cast<const u8 *>(set_flag) - m_physical_page_flags;
                const void *lst_flag = ::memrchr(std::addressof(m_physical_page_flags[set_idx]), 1, idx_end - set_idx);
                const uintptr_t lst_idx = (lst_flag) ? (reinterpret_cast<const u8 *>(lst_flag) - m_physical_page_flags + 1) : idx_end;
                std::memset(std::addressof(m_physical_page_flags[set_idx]), 2, lst_idx - set_idx);
            }
        }

        return 0;
    }

    Span *TlsHeapCentral::AllocatePagesImpl(size_t num_pages) {
        if (num_pages >= m_span_table.total_pages / 4) {
            this->MakeFreeSpan(std::numeric_limits<size_t>::max());
        }

        Span *span = this->SearchFreeSpan(num_pages);
        if (span == nullptr) {
            span = this->MakeFreeSpan(num_pages);
            if (span == nullptr) {
                return nullptr;
            }
        }

        AMS_ASSERT(span->status == Span::Status_InFreeList);

        if (num_pages == span->num_pages) {
            if (m_use_virtual_memory && this->AllocatePhysical(span->start.p, span->num_pages * TlsHeapStatic::PageSize) != 0) {
                return nullptr;
            } else {
                this->RemoveFromFreeBlockList(span);
                span->status = Span::Status_InUse;
                span->id     = 0;
                span->aux.large_clear.zero = 0;
                return span;
            }
        } else {
            /* Save the extents of the free span we found. */
            auto * const prev_ptr   = span->start.p;
            const size_t prev_pages = span->num_pages;

            /* Allocate a new span struct. */
            Span *new_span = this->AllocateSpanStruct();
            if (new_span == nullptr) {
                return nullptr;
            }
            auto new_span_guard = SCOPE_GUARD { this->FreeSpanToSpanPage(new_span); };

            /* Allocating the new span potentially invalidates the span we were looking at, so find the span for it in the table. */
            span = GetSpanFromPointer(std::addressof(m_span_table), prev_ptr);
            const size_t cur_pages = span->num_pages;

            /* If the span was partially allocated, we need to find a new one that's big enough. */
            if (cur_pages != prev_pages) {
                span = this->SearchFreeSpan(num_pages);
                if (span == nullptr) {
                    return nullptr;
                }
            }

            /* If the span is big enough to split (span->num_pages > num_pages), we want to split it. */
            /* span->num_pages > num_pages is true if the span wasn't partially allocated (cur_pages == prev_pages) */
            /* OR if the new free span we found has num_pages > num_pages. Note that we know span->num_pages >= num_pages */
            /* so this > condition can be expressed as span->num_pages != num_pages. */
            if (cur_pages == prev_pages || num_pages != span->num_pages) {
                /* We're going to use the new span for our split. */
                new_span_guard.Cancel();

                return this->SplitSpan(span, num_pages, new_span);
            } else if (m_use_virtual_memory && this->AllocatePhysical(span->start.p, span->num_pages * TlsHeapStatic::PageSize) != 0) {
                return nullptr;
            } else {
                this->RemoveFromFreeBlockList(span);
                span->status = Span::Status_InUse;
                span->id     = 0;
                span->aux.large_clear.zero = 0;
                return span;
            }
        }
    }

    Span *TlsHeapCentral::AllocatePagesWithBigAlignImpl(size_t num_pages, size_t align) {
        if (num_pages >= m_span_table.total_pages / 4) {
            this->MakeFreeSpan(std::numeric_limits<size_t>::max());
        }

        Span *before_span = this->AllocateSpanStruct();
        if (!before_span) {
            return nullptr;
        }

        Span *after_span = this->AllocateSpanStruct();
        if (!after_span) {
            this->FreeSpanToSpanPage(before_span);
            return nullptr;
        }

        AMS_ASSERT(align <= TlsHeapStatic::PageSize);
        AMS_ASSERT(util::IsPowerOfTwo(align));

        Span *span = this->SearchFreeSpan(num_pages);
        if (span == nullptr) {
            span = this->MakeFreeSpan(num_pages);
            if (span == nullptr) {
                this->FreeSpanToSpanPage(before_span);
                this->FreeSpanToSpanPage(after_span);
                return nullptr;
            }
        }

        AMS_ASSERT(span->status == Span::Status_InFreeList);

        const uintptr_t aligned_start = util::AlignUp(span->start.u, align);
        if (m_use_virtual_memory && this->AllocatePhysical(reinterpret_cast<void *>(aligned_start), num_pages * TlsHeapStatic::PageSize) != 0) {
            this->FreeSpanToSpanPage(before_span);
            this->FreeSpanToSpanPage(after_span);
            return nullptr;
        } else if (aligned_start == span->start.u) {
            /* We don't need the before span, but we do need the after span. */
            this->FreeSpanToSpanPage(before_span);

            after_span->start.u     = aligned_start + num_pages * TlsHeapStatic::PageSize;
            after_span->num_pages   = span->num_pages - num_pages;
            after_span->page_class  = 0;
            after_span->status      = Span::Status_InFreeList;
            after_span->id          = 0;

            this->RemoveFromFreeBlockList(span);

            span->status = Span::Status_InUse;
            span->aux.large_clear.zero = 0;
            ChangeRangeOfSpan(std::addressof(m_span_table), span, aligned_start, num_pages);

            RegisterSpan(std::addressof(m_span_table), after_span);
            this->MergeIntoFreeList(after_span);

            return span;
        } else {
            /* We need the before span. */
            before_span->start.u    = span->start.u;
            before_span->num_pages  = TlsHeapStatic::GetPageIndex(aligned_start - span->start.u);
            before_span->page_class = 0;
            before_span->status     = Span::Status_InFreeList;
            before_span->id         = 0;

            const size_t after_pages = span->num_pages - before_span->num_pages - num_pages;
            if (after_pages) {
                /* We need the after span. */
                after_span->start.u     = aligned_start + num_pages * TlsHeapStatic::PageSize;
                after_span->num_pages   = after_pages;
                after_span->page_class  = 0;
                after_span->status      = Span::Status_InFreeList;
                after_span->id          = 0;

                this->RemoveFromFreeBlockList(span);

                span->status = Span::Status_InUse;
                span->aux.large_clear.zero = 0;

                ChangeRangeOfSpan(std::addressof(m_span_table), span, aligned_start, num_pages);

                RegisterSpan(std::addressof(m_span_table), before_span);
                RegisterSpan(std::addressof(m_span_table), after_span);
                this->MergeIntoFreeList(before_span);
                this->MergeIntoFreeList(after_span);

                return span;
            } else {
                /* We don't need the after span. */
                this->FreeSpanToSpanPage(after_span);

                this->RemoveFromFreeBlockList(span);

                span->status = Span::Status_InUse;
                span->aux.large_clear.zero = 0;

                ChangeRangeOfSpan(std::addressof(m_span_table), span, aligned_start, num_pages);

                RegisterSpan(std::addressof(m_span_table), before_span);
                this->MergeIntoFreeList(before_span);

                return span;
            }
        }
    }

    void TlsHeapCentral::FreePagesImpl(Span *span) {
        AMS_ASSERT(span && span->page_class == 0);

        if (span->status == Span::Status_InFreeList) {
            /* Double free error. */
        } else {
            span->status = Span::Status_InFreeList;
            if (m_use_virtual_memory) {
                const uintptr_t start     = span->start.u;
                const uintptr_t end       = span->start.u + (span->num_pages * TlsHeapStatic::PageSize);
                uintptr_t start_alignup   = TlsHeapStatic::AlignUpPhysicalPage(start);
                uintptr_t end_aligndown   = TlsHeapStatic::AlignDownPhysicalPage(end);

                this->MergeIntoFreeList(span);

                const uintptr_t new_start = span->start.u;
                const uintptr_t new_end   = span->start.u + (span->num_pages * TlsHeapStatic::PageSize);

                if (start != start_alignup && new_start + TlsHeapStatic::PhysicalPageSize <= start_alignup) {
                    start_alignup -= TlsHeapStatic::PhysicalPageSize;
                }
                if (end != end_aligndown && end_aligndown + TlsHeapStatic::PhysicalPageSize <= new_end) {
                    end_aligndown += TlsHeapStatic::PhysicalPageSize;
                }

                if (start_alignup < end_aligndown) {
                    const auto err = this->FreePhysical(reinterpret_cast<void *>(start_alignup), end_aligndown - start_alignup);
                    AMS_ASSERT(err == 0);
                    AMS_UNUSED(err);
                }
            } else {
                this->MergeIntoFreeList(span);
            }
        }
    }

    void *TlsHeapCentral::CacheSmallMemoryImpl(size_t cls, size_t align, bool for_system) {
        AMS_ASSERT(cls != 0 && cls < TlsHeapStatic::NumClassInfo);

        Span *span = ListGetNext(std::addressof(m_smallmem_lists[cls]));
        while (true) {
            if (for_system) {
                while (span && span->status != Span::Status_InUseSystem) {
                    span = ListGetNext(span);
                }
            } else {
                while (span && (span->status == Span::Status_InUseSystem || span->id)) {
                    span = ListGetNext(span);
                }
            }

            if (!span) {
                break;
            }

            void *mem = AllocateSmallMemory(span);
            if (mem) {
                if (!span->aux.small.objects) {
                    ListRemoveSelf(span);
                }
                return mem;
            }

            AMS_ASSERT(!span->aux.small.objects);

            Span *old = span;
            span = ListGetNext(span);
            ListRemoveSelf(old);
        }


        Span *new_span = this->AllocatePagesImpl(TlsHeapStatic::GetNumPages(cls));
        if (new_span) {
            SpanToSmallMemorySpan(std::addressof(m_span_table), new_span, cls);
            ListInsertAfter(std::addressof(m_smallmem_lists[cls]), new_span);
            InitSmallMemorySpan(new_span, cls, for_system, 0);

            void *mem = AllocateSmallMemory(new_span);
            if (!new_span->aux.small.objects) {
                ListRemoveSelf(new_span);
            }
            return mem;
        } else {
            for (size_t cur_cls = cls; cur_cls < TlsHeapStatic::NumClassInfo; cur_cls++) {
                if (align == 0 || util::IsAligned(TlsHeapStatic::GetChunkSize(cur_cls), align)) {
                    span = ListGetNext(std::addressof(m_smallmem_lists[cur_cls]));
                    if (for_system) {
                        while (span && span->status != Span::Status_InUseSystem) {
                            span = ListGetNext(span);
                        }
                    } else {
                        while (span && (span->status == Span::Status_InUseSystem)) {
                            span = ListGetNext(span);
                        }
                    }
                    if (!span) {
                        continue;
                    }

                    void *mem = AllocateSmallMemory(span);
                    if (!mem) {
                        continue;
                    }

                    if (!span->aux.small.objects) {
                        ListRemoveSelf(span);
                    }
                    return mem;
                }
            }
            return nullptr;
        }
    }

    errno_t TlsHeapCentral::UncacheSmallMemoryImpl(void *ptr) {
        Span *span = GetSpanFromPointer(std::addressof(m_span_table), ptr);
        if (span && span->page_class) {
            if (!span->aux.small.objects) {
                ListInsertAfter(std::addressof(m_smallmem_lists[span->page_class]), span);
            }

            ReleaseSmallMemory(span, ptr);

            if (!span->object_count) {
                span->aux.small.objects = nullptr;
                ListRemoveSelf(span);
                AMS_ASSERT(span->page_class != 0);
                SmallMemorySpanToSpan(std::addressof(m_span_table), span);
                this->FreePagesImpl(span);
            }

            return 0;
        } else {
            AMS_ASSERT(span);
            AMS_ASSERT(span->page_class == 0);
            return EFAULT;
        }
    }

    size_t TlsHeapCentral::CacheSmallMemoryListImpl(TlsHeapCache *cache, size_t *cls, size_t count, void **p, s32 cpu_id, size_t align) {
        AMS_ASSERT(*cls != 0 && *cls < TlsHeapStatic::NumClassInfo);

        Span::SmallMemory head = {};
        Span::SmallMemory *hptr = std::addressof(head);

        Span *span = ListGetNext(std::addressof(m_smallmem_lists[*cls]));
        size_t n = 0;

        while (span) {
            if (span->status != Span::Status_InUseSystem && span->id == cpu_id) {
                MangledSmallMemory memlist;
                if (size_t num = AllocateSmallMemory(span, cache, count - n, std::addressof(memlist)); num != 0) {
                    hptr->next = memlist.from;
                    hptr = memlist.to;
                    memlist.to->next = nullptr;
                    n += num;
                    if (n >= count) {
                        if (!span->aux.small.objects) {
                            ListRemoveSelf(span);
                        }
                        *p = head.next;
                        return n;
                    }
                }

                AMS_ASSERT(span->aux.small.objects);

                Span *old = span;
                span = ListGetNext(span);
                ListRemoveSelf(old);
            } else {
                span = ListGetNext(span);
            }
        }

        Span *new_span = this->AllocatePagesImpl(TlsHeapStatic::GetNumPages(*cls));
        if (new_span) {
            SpanToSmallMemorySpan(std::addressof(m_span_table), new_span, *cls);
            ListInsertAfter(std::addressof(m_smallmem_lists[*cls]), new_span);
            InitSmallMemorySpan(new_span, *cls, false, cpu_id);

            MangledSmallMemory memlist;
            size_t num = AllocateSmallMemory(new_span, cache, count - n, std::addressof(memlist));
            AMS_ASSERT(num > 0);

            hptr->next = memlist.from;
            hptr = memlist.to;
            memlist.to->next = nullptr;
            n += num;

            if (!new_span->aux.small.objects) {
                ListRemoveSelf(new_span);
            }

            *p = head.next;
            return n;
        } else if (head.next) {
            *p = head.next;
            return n;
        } else {
            for (size_t cur_cls = *cls; cur_cls < TlsHeapStatic::NumClassInfo; cur_cls++) {
                if (align == 0 || util::IsAligned(TlsHeapStatic::GetChunkSize(cur_cls), align)) {
                    span = ListGetNext(std::addressof(m_smallmem_lists[cur_cls]));

                    while (span && (span->status == Span::Status_InUseSystem)) {
                        span = ListGetNext(span);
                    }

                    if (!span) {
                        continue;
                    }

                    void *mem = AllocateSmallMemory(span);
                    if (!mem) {
                        continue;
                    }

                    if (!span->aux.small.objects) {
                        ListRemoveSelf(span);
                    }

                    reinterpret_cast<Span::SmallMemory *>(mem)->next = nullptr;
                    *p = cache->ManglePointer(mem);
                    *cls = cur_cls;
                    return 1;
                }
            }

           *p = nullptr;
            return 0;
        }
    }

    errno_t TlsHeapCentral::WalkAllocatedPointersImpl(HeapWalkCallback callback, void *user_data) {
        errno_t err = ENOENT;

        for (Span *span = GetSpanFromPointer(std::addressof(m_span_table), this); span != nullptr; span = GetNextSpan(std::addressof(m_span_table), span)) {
            if (span->status != Span::Status_InUse) {
                continue;
            }

            const size_t size = span->num_pages * TlsHeapStatic::PageSize;

            if (span->page_class != 0) {
                static_assert(util::IsAligned(TlsHeapStatic::PageSize, BITSIZEOF(FreeListAvailableWord)));
                FreeListAvailableWord flags[TlsHeapStatic::PageSize / BITSIZEOF(FreeListAvailableWord)];
                std::memset(flags, 0, sizeof(flags));

                const size_t chunk_size = TlsHeapStatic::GetChunkSize(span->page_class);
                const size_t n = size / chunk_size;

                AMS_ASSERT(n <= TlsHeapStatic::PageSize);
                if (n <= TlsHeapStatic::PageSize) {
                    for (Span::SmallMemory *sm = span->aux.small.objects; sm != nullptr; sm = sm->next) {
                        const size_t idx = (reinterpret_cast<uintptr_t>(sm) - span->start.u) / chunk_size;
                        flags[FreeListAvailableIndex(idx)] |= FreeListAvailableMask(idx);
                    }

                    for (size_t i = 0; i < n; i++) {
                        if (!(flags[FreeListAvailableIndex(i)] & FreeListAvailableMask(i))) {
                            if (s32 res = this->CallWalkCallback(callback, reinterpret_cast<void *>(span->start.u + chunk_size * i), chunk_size, user_data); res != 0) {
                                if (res >= 0) {
                                    err = res;
                                    break;
                                } else {
                                    err = 0;
                                }
                            }
                        }
                    }
                } else {
                    return EIO;
                }
            } else {
                if (s32 res = this->CallWalkCallback(callback, span->start.p, size, user_data); res != 0) {
                    if (res >= 0) {
                        err = res;
                        break;
                    } else {
                        err = 0;
                    }
                }
            }
        }

        return err;
    }

    errno_t TlsHeapCentral::GetMappedMemStatsImpl(size_t *out_free_size, size_t *out_max_allocatable_size) {
        if (!m_use_virtual_memory) {
            return EOPNOTSUPP;
        }

        /* TODO: Is this worth supporting? */
        AMS_UNUSED(out_free_size, out_max_allocatable_size);
        return EOPNOTSUPP;
    }

    errno_t TlsHeapCentral::GetMemStatsImpl(TlsHeapMemStats *out) {
        size_t max_allocatable_size = 0;
        size_t free_size            = 0;
        size_t system_size          = 0;
        size_t allocated_size       = 0;
        size_t num_free_regions     = 0;
        size_t num_free_spans       = 0;
        size_t wip_allocatable_size = 0;

        Span *span = GetSpanFromPointer(std::addressof(m_span_table), this);
        while (span) {
            const size_t size = span->num_pages * TlsHeapStatic::PageSize;

            if (span->status == Span::Status_InUse || span->status == Span::Status_InUseSystem) {
                /* Found a usable span, so end our contiguous run. */
                max_allocatable_size = std::max(max_allocatable_size, wip_allocatable_size);
                wip_allocatable_size = 0;

                if (span->status == Span::Status_InUseSystem) {
                    system_size += size;
                } else if (span->page_class && span->aux.small.objects) {
                    const size_t chunk_size = TlsHeapStatic::GetChunkSize(span->page_class);
                    const size_t used_size = chunk_size * span->object_count;
                    allocated_size += used_size;
                    free_size      += size - used_size;
                } else {
                    allocated_size += size;
                }
            } else {
                /* Free span. */
                free_size += size;
                num_free_spans++;
                if (wip_allocatable_size == 0) {
                    num_free_regions++;
                }
                wip_allocatable_size += size;
            }

            span = GetNextSpan(std::addressof(m_span_table), span);
        }

        max_allocatable_size = std::max(max_allocatable_size, wip_allocatable_size);

        bool sp_full = true;
        for (SpanPage *sp = ListGetNext(std::addressof(m_spanpage_list)); sp != nullptr; sp = ListGetNext(sp)) {
            if (sp->info.is_sticky == 0 && CanAllocateSpan(sp)) {
                sp_full = false;
                break;
            }
        }

        if (num_free_spans == 1 && allocated_size == 0) {
            sp_full = false;
        }

        if (sp_full && num_free_regions < 2 && max_allocatable_size >= TlsHeapStatic::PageSize) {
            max_allocatable_size -= TlsHeapStatic::PageSize;
        }

        if (max_allocatable_size < 3 * TlsHeapStatic::PageSize) {
            max_allocatable_size = 0;
        }

        out->max_allocatable_size = max_allocatable_size;
        out->free_size            = free_size;
        out->allocated_size       = allocated_size;
        out->system_size          = system_size;

        return 0;
    }

    void TlsHeapCentral::DumpImpl(DumpMode dump_mode, int fd, bool json) {
        AMS_UNUSED(dump_mode, fd, json);
        AMS_ABORT("Not yet implemented");
    }

}
