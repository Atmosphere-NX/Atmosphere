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
#include <stratosphere.hpp>
#include "mem_impl_heap_platform.hpp"
#include "mem_impl_heap_tls_heap_static.hpp"
#include "mem_impl_heap_tls_heap_cache.hpp"

namespace ams::mem::impl::heap {

    /* Simple intrusive list. */
    template<typename T>
    struct ListHeader {
        T *list_next;
    };

    template<typename T>
    struct ListElement : public ListHeader<T> {
        T *list_prev;
    };

    template<typename T>
    constexpr inline void ListClearLink(ListHeader<T> *l) {
        l->list_next = nullptr;
    }

    template<typename T>
    constexpr inline void ListClearLink(ListElement<T> *l) {
        l->list_next = nullptr;
        l->list_prev = nullptr;
    }

    template<typename T>
    constexpr inline T *ListGetNext(const ListHeader<T> *l) {
        return l->list_next;
    }

    template<typename T>
    constexpr inline T *ListGetNext(const ListElement<T> *l) {
        return l->list_next;
    }

    template<typename T>
    constexpr inline T *ListGetPrev(const ListElement<T> *l) {
        return l->list_prev;
    }

    template<typename T>
    constexpr inline void ListInsertAfter(ListHeader<T> *hdr, T *e) {
        e->list_next = hdr->list_next;
        e->list_prev = static_cast<T *>(hdr);

        if (hdr->list_next != nullptr) {
            hdr->list_next->list_prev = e;
        }
        hdr->list_next = e;
    }

    template<typename T>
    constexpr inline void ListRemoveSelf(T *e) {
        if (e->list_next != nullptr) {
            e->list_next->list_prev = e->list_prev;
        }
        if (e->list_prev != nullptr) {
            e->list_prev->list_next = e->list_next;
        }
        e->list_next = nullptr;
        e->list_prev = nullptr;
    }

    struct Span : public ListElement<Span> {
        struct SmallMemory {
            SmallMemory *next;
        };

        enum Status : u8 {
            Status_NotUsed     = 0,
            Status_InUse       = 1,
            Status_InFreeList  = 2,
            Status_InUseSystem = 3,
        };

        u16 object_count;
        u8 page_class;
        u8 status;
        s32 id;
        union {
            uintptr_t u;
            void *p;
            SmallMemory *sm;
            char *cp;
        } start;
        uintptr_t num_pages;
        union {
            struct {
                SmallMemory *objects;
                u64 is_allocated[8];
            } small;
            struct {
                u8 color[3];
                char name[0x10];
            } large;
            struct {
                u32 zero;
            } large_clear;
        } aux;
    };

    struct SpanPage : public ListElement<SpanPage> {
        struct Info {
            u64 alloc_bitmap;
            u16 free_count;
            u8 is_sticky;
            Span span_of_spanpage;
        } info;
        Span spans[(TlsHeapStatic::PageSize - sizeof(Info) - sizeof(ListElement<SpanPage>)) / sizeof(Span)];

        static constexpr size_t MaxSpanCount = sizeof(spans) / sizeof(spans[0]);
    };
    static_assert(sizeof(SpanPage) <= TlsHeapStatic::PageSize);

    static constexpr ALWAYS_INLINE bool CanAllocateSpan(const SpanPage *span_page) {
        return span_page->info.alloc_bitmap != ~(decltype(span_page->info.alloc_bitmap){});
    }

    struct SpanTable {
        uintptr_t total_pages;
        Span **page_to_span;
        u8 *pageclass_cache;
    };

    struct TlsHeapMemStats {
        size_t allocated_size;
        size_t free_size;
        size_t system_size;
        size_t max_allocatable_size;
    };

    ALWAYS_INLINE Span *GetSpanFromPointer(const SpanTable *table, const void *ptr) {
        const size_t idx = TlsHeapStatic::GetPageIndex(reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(table));
        if (idx < table->total_pages) {
            return table->page_to_span[idx];
        } else {
            return nullptr;
        }
    }

    ALWAYS_INLINE SpanPage *GetSpanPage(Span *span) {
        return reinterpret_cast<SpanPage *>(TlsHeapStatic::AlignDownPage(reinterpret_cast<uintptr_t>(span)));
    }

    ALWAYS_INLINE Span *GetSpanPageSpan(SpanPage *span_page) {
        return std::addressof(span_page->info.span_of_spanpage);
    }

    ALWAYS_INLINE Span *GetPrevSpan(const SpanTable *span_table, const Span *span) {
        return GetSpanFromPointer(span_table, reinterpret_cast<const void *>(span->start.u - 1));
    }

    ALWAYS_INLINE Span *GetNextSpan(const SpanTable *span_table, const Span *span) {
        return GetSpanFromPointer(span_table, reinterpret_cast<const void *>(span->start.u + span->num_pages * TlsHeapStatic::PageSize));
    }

    class TlsHeapCentral {
        private:
            using FreeListAvailableWord = u64;

            static constexpr size_t FreeListCount = 0x100;
            static constexpr size_t NumFreeListBitmaps = FreeListCount / BITSIZEOF(FreeListAvailableWord);

            static constexpr ALWAYS_INLINE size_t FreeListAvailableIndex(size_t which) {
                return which / BITSIZEOF(FreeListAvailableWord);
            }

            static constexpr ALWAYS_INLINE size_t FreeListAvailableBit(size_t which) {
                return which % BITSIZEOF(FreeListAvailableWord);
            }

            static constexpr ALWAYS_INLINE FreeListAvailableWord FreeListAvailableMask(size_t which) {
                return static_cast<FreeListAvailableWord>(1) << FreeListAvailableBit(which);
            }

            static_assert(NumFreeListBitmaps * BITSIZEOF(FreeListAvailableWord) == FreeListCount);
        private:
            SpanTable span_table;
            u8 *physical_page_flags;
            s32 num_threads;
            s32 static_thread_quota;
            s32 dynamic_thread_quota;
            bool use_virtual_memory;
            os::Mutex lock;
            ListHeader<SpanPage> spanpage_list;
            ListHeader<SpanPage> full_spanpage_list;
            ListHeader<Span> freelists[FreeListCount];
            FreeListAvailableWord freelists_bitmap[NumFreeListBitmaps];
            ListHeader<Span> smallmem_lists[TlsHeapStatic::NumClassInfo];
        public:
            TlsHeapCentral() : lock(true) {
                this->span_table.total_pages = 0;
            }

            errno_t Initialize(void *start, size_t size, bool use_virtual_memory);
            bool IsClean();

            errno_t ReallocateLargeMemory(void *ptr, size_t size, void **p);
            errno_t ShrinkLargeMemory(void *ptr, size_t size);

            void CalculateHeapHash(HeapHash *out);

            errno_t AddThreadCache(TlsHeapCache *cache) {
                std::scoped_lock lk(this->lock);

                /* Add thread and recalculate. */
                this->num_threads++;
                this->dynamic_thread_quota = this->GetTotalHeapSize() / (2 * this->num_threads);

                return 0;
            }

            errno_t RemoveThreadCache(TlsHeapCache *cache) {
                std::scoped_lock lk(this->lock);

                /* Remove thread and recalculate. */
                this->num_threads--;
                this->dynamic_thread_quota = this->GetTotalHeapSize() / (2 * this->num_threads);

                return 0;
            }

            void *CacheLargeMemory(size_t size) {
                std::scoped_lock lk(this->lock);

                const size_t num_pages = util::AlignUp(size, TlsHeapStatic::PageSize) / TlsHeapStatic::PageSize;
                if (Span *span = this->AllocatePagesImpl(num_pages); span != nullptr) {
                    return span->start.p;
                } else {
                    return nullptr;
                }
            }

            void *CacheLargeMemoryWithBigAlign(size_t size, size_t align) {
                std::scoped_lock lk(this->lock);

                const size_t num_pages = util::AlignUp(size, TlsHeapStatic::PageSize) / TlsHeapStatic::PageSize;

                Span *span = nullptr;
                if (align > TlsHeapStatic::PageSize) {
                    span = this->AllocatePagesWithBigAlignImpl(num_pages, align);
                } else {
                    span = this->AllocatePagesImpl(num_pages);
                }

                if (span != nullptr) {
                    return span->start.p;
                } else {
                    return nullptr;
                }
            }

            void *CacheSmallMemory(size_t cls, size_t align = 0) {
                std::scoped_lock lk(this->lock);

                return this->CacheSmallMemoryImpl(cls, align, false);
            }

            void *CacheSmallMemoryForSystem(size_t cls) {
                std::scoped_lock lk(this->lock);

                return this->CacheSmallMemoryImpl(cls, 0, true);
            }

            size_t CacheSmallMemoryList(TlsHeapCache *cache, size_t *cls, size_t count, void **p, size_t align = 0) {
                std::scoped_lock lk(this->lock);

                s32 cpu_id = 0;
                if (*cls < 8) {
                    getcpu(std::addressof(cpu_id));
                }

                return this->CacheSmallMemoryListImpl(cache, cls, count, p, cpu_id, 0);
            }

            bool CheckCachedSize(s32 size) const {
                return size < this->dynamic_thread_quota && size < this->static_thread_quota;
            }

            void Dump(DumpMode dump_mode, int fd, bool json) {
                std::scoped_lock lk(this->lock);
                return this->DumpImpl(dump_mode, fd, json);
            }

            size_t GetAllocationSize(const void *ptr) {
                if (TlsHeapStatic::IsPageAligned(ptr)) {
                    Span *span = nullptr;
                    {
                        std::scoped_lock lk(this->lock);
                        span = GetSpanFromPointer(std::addressof(this->span_table), ptr);
                    }
                    if (span != nullptr) {
                        return span->num_pages * TlsHeapStatic::PageSize;
                    } else {
                        AMS_ASSERT(span != nullptr);
                        return 0;
                    }
                } else {
                    /* TODO: Handle error? */
                    return 0;
                }
            }

            size_t GetClassFromPointer(const void *ptr) {
                std::atomic_thread_fence(std::memory_order_acquire);

                const size_t idx = (reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(this)) / TlsHeapStatic::PageSize;
                if (idx < this->span_table.total_pages) {
                    if (ptr != nullptr) {
                        std::scoped_lock lk(this->lock);
                        Span *span = GetSpanFromPointer(std::addressof(this->span_table), ptr);
                        if (span != nullptr) {
                            AMS_ASSERT(span->page_class == this->span_table.pageclass_cache[idx]);
                        } else {
                            AMS_ASSERT(span != nullptr);
                        }
                    }
                    return this->span_table.pageclass_cache[idx];
                } else {
                    /* TODO: Handle error? */
                    return 0xFFFFFFFF;
                }
            }

            errno_t GetColor(const void *ptr, int *out) {
                if (out == nullptr) {
                    return EINVAL;
                }

                std::scoped_lock lk(this->lock);
                if (Span *span = GetSpanFromPointer(std::addressof(this->span_table), ptr); span != nullptr && !span->page_class) {
                    *out = (span->aux.large.color[0] << 0) | (span->aux.large.color[1] << 0) | (span->aux.large.color[2] << 16);
                    return 0;
                } else {
                    return EINVAL;
                }
            }

            errno_t SetColor(const void *ptr, int color) {
                std::scoped_lock lk(this->lock);
                if (Span *span = GetSpanFromPointer(std::addressof(this->span_table), ptr); span != nullptr && !span->page_class) {
                    span->aux.large.color[0] = (color >>  0) & 0xFF;
                    span->aux.large.color[1] = (color >>  8) & 0xFF;
                    span->aux.large.color[2] = (color >> 16) & 0xFF;
                    return 0;
                } else {
                    return EINVAL;
                }
            }

            errno_t GetMappedMemStats(size_t *out_free_size, size_t *out_max_allocatable_size) {
                std::scoped_lock lk(this->lock);

                return this->GetMappedMemStatsImpl(out_free_size, out_max_allocatable_size);
            }

            errno_t GetMemStats(TlsHeapMemStats *out) {
                std::scoped_lock lk(this->lock);

                return this->GetMemStatsImpl(out);
            }

            errno_t GetName(const void *ptr, char *dst, size_t dst_size) {
                std::scoped_lock lk(this->lock);
                if (Span *span = GetSpanFromPointer(std::addressof(this->span_table), ptr); span != nullptr && !span->page_class) {
                    strlcpy(dst, span->aux.large.name, dst_size);
                    return 0;
                } else {
                    return EINVAL;
                }
            }

            errno_t SetName(const void *ptr, const char *name) {
                std::scoped_lock lk(this->lock);
                if (Span *span = GetSpanFromPointer(std::addressof(this->span_table), ptr); span != nullptr && !span->page_class) {
                    strlcpy(span->aux.large.name, name, sizeof(span->aux.large.name));
                    return 0;
                } else {
                    return EINVAL;
                }
            }

            size_t GetTotalHeapSize() const {
                return this->span_table.total_pages * TlsHeapStatic::PageSize;
            }

            errno_t UncacheLargeMemory(void *ptr) {
                if (TlsHeapStatic::IsPageAligned(ptr)) {
                    std::scoped_lock lk(this->lock);
                    if (Span *span = GetSpanFromPointer(std::addressof(this->span_table), ptr); span != nullptr) {
                        this->FreePagesImpl(span);
                        return 0;
                    } else {
                        return EFAULT;
                    }
                } else {
                    return EFAULT;
                }
            }

            errno_t UncacheSmallMemory(void *ptr) {
                std::scoped_lock lk(this->lock);
                return this->UncacheSmallMemoryImpl(ptr);
            }

            errno_t UncacheSmallMemoryList(TlsHeapCache *cache, void *ptr) {
                std::scoped_lock lk(this->lock);

                while (true) {
                    if (ptr == nullptr) {
                        return 0;
                    }
                    ptr = cache->ManglePointer(ptr);
                    void *next = *reinterpret_cast<void **>(ptr);
                    if (auto err = this->UncacheSmallMemoryImpl(ptr); err != 0) {
                        return err;
                    }
                    ptr = next;
                }
            }

            errno_t WalkAllocatedPointers(HeapWalkCallback callback, void *user_data) {
                /* Explicitly handle locking, as we will release the lock during callback. */
                this->lock.lock();
                ON_SCOPE_EXIT { this->lock.unlock(); };

                return this->WalkAllocatedPointersImpl(callback, user_data);
            }
        private:
            SpanPage *AllocateSpanPage();
            Span *AllocateSpanFromSpanPage(SpanPage *sp);

            Span *SplitSpan(Span *span, size_t num_pages, Span *new_span);
            void MergeFreeSpans(Span *span, Span *span_to_merge, uintptr_t start);

            bool DestroySpanPageIfEmpty(SpanPage *sp, bool full);
            Span *GetFirstSpan() const;
            Span *MakeFreeSpan(size_t num_pages);
            Span *SearchFreeSpan(size_t num_pages) const;

            void FreeSpanToSpanPage(Span *span, SpanPage *sp);
            void FreeSpanToSpanPage(Span *span);

            void MergeIntoFreeList(Span *&span);

            errno_t AllocatePhysical(void *start, size_t size);
            errno_t FreePhysical(void *start, size_t size);
        private:
            Span *AllocatePagesImpl(size_t num_pages);
            Span *AllocatePagesWithBigAlignImpl(size_t num_pages, size_t align);
            void FreePagesImpl(Span *span);

            void *CacheSmallMemoryImpl(size_t cls, size_t align, bool for_system);
            errno_t UncacheSmallMemoryImpl(void *ptr);

            size_t CacheSmallMemoryListImpl(TlsHeapCache *cache, size_t *cls, size_t count, void **p, s32 cpu_id, size_t align);

            errno_t WalkAllocatedPointersImpl(HeapWalkCallback callback, void *user_data);

            errno_t GetMappedMemStatsImpl(size_t *out_free_size, size_t *out_max_allocatable_size);
            errno_t GetMemStatsImpl(TlsHeapMemStats *out);

            void DumpImpl(DumpMode dump_mode, int fd, bool json);
        private:
            size_t FreeListFirstNonEmpty(size_t start) const {
                if (start < FreeListCount) {
                    for (size_t i = FreeListAvailableIndex(start); i < util::size(this->freelists_bitmap); i++) {
                        const FreeListAvailableWord masked = this->freelists_bitmap[i] & ~(FreeListAvailableMask(start) - 1);
                        if (masked) {
                            const size_t b = __builtin_ctzll(masked);
                            const size_t res = i * BITSIZEOF(FreeListAvailableWord) + b;
                            AMS_ASSERT(res < FreeListCount);
                            return res;
                        }
                        start = (i + 1) * BITSIZEOF(FreeListAvailableWord);
                    }
                }
                return FreeListCount;
            }

            ALWAYS_INLINE void AddToFreeBlockList(Span *span) {
                AMS_ASSERT(GetSpanPageSpan(GetSpanPage(span)) != span);
                AMS_ASSERT(span->status == Span::Status_InFreeList);
                const size_t which = std::min(span->num_pages, FreeListCount) - 1;
                ListInsertAfter(std::addressof(this->freelists[which]), span);
                this->freelists_bitmap[FreeListAvailableIndex(which)] |= FreeListAvailableMask(which);
            }

            ALWAYS_INLINE void RemoveFromFreeBlockList(Span *span) {
                const size_t which = std::min(span->num_pages, FreeListCount) - 1;
                ListRemoveSelf(span);
                if (!ListGetNext(std::addressof(this->freelists[which]))) {
                    this->freelists_bitmap[FreeListAvailableIndex(which)] &= ~FreeListAvailableMask(which);
                }
            }

            Span *AllocateSpanStruct() {
                SpanPage *sp = ListGetNext(std::addressof(this->spanpage_list));
                while (sp && (sp->info.is_sticky || !CanAllocateSpan(sp))) {
                    sp = ListGetNext(sp);
                }

                if (sp == nullptr) {
                    sp = this->AllocateSpanPage();
                }

                if (sp != nullptr) {
                    return this->AllocateSpanFromSpanPage(sp);
                } else {
                    return nullptr;
                }
            }

            s32 CallWalkCallback(HeapWalkCallback callback, void *ptr, size_t size, void *user_data) {
                this->lock.unlock();
                int res = callback(ptr, size, user_data);
                this->lock.lock();
                if (res) {
                    return 0;
                } else {
                    return -1;
                }
            }
    };


}