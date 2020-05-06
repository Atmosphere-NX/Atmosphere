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

namespace ams::mem::impl::heap {

    class TlsHeapStatic {
        public:
            struct ClassInfo {
                u16 num_pages;
                u16 chunk_size;
            };

            static constexpr size_t NumClassInfo = 57;

            static constexpr size_t MaxSizeWithClass = 0xC00;
            static constexpr size_t ChunkGranularity = 0x10;
            static constexpr size_t PageSize = 4_KB;
            static constexpr size_t PhysicalPageSize = 256_KB;
        public:
            static constexpr inline std::array<ClassInfo, NumClassInfo> ClassInfos = {
                ClassInfo{ .num_pages = 0, .chunk_size = 0x000, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x010, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x020, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x030, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x040, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x050, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x060, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x070, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x080, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x090, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x0A0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x0B0, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x0C0, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x0D0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x0E0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x0F0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x100, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x110, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x120, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x130, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x140, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x150, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x160, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x170, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x180, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x190, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x1A0, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x1B0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x1C0, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x1D0, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x1E0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x200, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x210, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x220, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x240, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x260, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x270, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x280, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x2A0, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x2D0, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x2E0, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x300, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x330, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x360, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x380, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x3B0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x400, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x450, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x490, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x4C0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x550, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x600, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0x660, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x6D0, },
                ClassInfo{ .num_pages = 1, .chunk_size = 0x800, },
                ClassInfo{ .num_pages = 3, .chunk_size = 0x990, },
                ClassInfo{ .num_pages = 2, .chunk_size = 0xAA0, },
            };

            static constexpr inline std::array<size_t, MaxSizeWithClass / ChunkGranularity> SizeToClass = [] {
                std::array<size_t, MaxSizeWithClass / ChunkGranularity> arr = {};
                arr[0] = 1;
                for (size_t i = 1; i < arr.size(); i++) {
                    const size_t cur_size = i * ChunkGranularity;
                    for (size_t j = 0; j < ClassInfos.size(); j++) {
                        if (ClassInfos[j].chunk_size >= cur_size) {
                            arr[i] = j;
                            break;
                        }
                    }
                }
                return arr;
            }();
        public:
            static constexpr ALWAYS_INLINE size_t GetClassFromSize(size_t size) {
                AMS_ASSERT(size <= MaxSize);
                const size_t idx = util::AlignUp(size, ChunkGranularity) / ChunkGranularity;
                if (idx < MaxSizeWithClass / ChunkGranularity) {
                    return SizeToClass[idx];
                } else {
                    return 0;
                }
            }

            static constexpr ALWAYS_INLINE size_t GetRealSizeFromSizeAndAlignment(size_t size, size_t align) {
                AMS_ASSERT(size <= MaxSize);
                const size_t idx = util::AlignUp(size, ChunkGranularity) / ChunkGranularity;
                if (size == 0 || idx >= MaxSizeWithClass / ChunkGranularity) {
                    return size;
                }
                const auto cls = SizeToClass[idx];
                if (!cls) {
                    return PageSize;
                }
                AMS_ASSERT(align != 0);
                const size_t mask = align - 1;
                for (auto i = cls; i < ClassInfos.size(); i++) {
                    if ((ClassInfos[i].chunk_size & mask) == 0) {
                        return ClassInfos[i].chunk_size;
                    }
                }
                return PageSize;
            }

            static constexpr ALWAYS_INLINE bool IsPageAligned(uintptr_t ptr) {
                return util::IsAligned(ptr, PageSize);
            }

            static ALWAYS_INLINE bool IsPageAligned(const void *ptr) {
                return IsPageAligned(reinterpret_cast<uintptr_t>(ptr));
            }

            static constexpr ALWAYS_INLINE size_t GetPageIndex(uintptr_t ptr) {
                return ptr / PageSize;
            }

            static constexpr ALWAYS_INLINE size_t GetPhysicalPageIndex(uintptr_t ptr) {
                return ptr / PhysicalPageSize;
            }

            static constexpr ALWAYS_INLINE uintptr_t AlignUpPage(uintptr_t ptr) {
                return util::AlignUp(ptr, PageSize);
            }

            template<typename T>
            static ALWAYS_INLINE T *AlignUpPage(T *ptr) {
                static_assert(util::is_pod<T>::value);
                static_assert(util::IsAligned(PageSize, alignof(T)));
                return reinterpret_cast<T *>(AlignUpPage(reinterpret_cast<uintptr_t>(ptr)));
            }

            static constexpr ALWAYS_INLINE uintptr_t AlignDownPage(uintptr_t ptr) {
                return util::AlignDown(ptr, PageSize);
            }

            template<typename T>
            static ALWAYS_INLINE T *AlignDownPage(T *ptr) {
                static_assert(util::is_pod<T>::value);
                static_assert(util::IsAligned(PageSize, alignof(T)));
                return reinterpret_cast<T *>(AlignDownPage(reinterpret_cast<uintptr_t>(ptr)));
            }

            static constexpr ALWAYS_INLINE uintptr_t AlignUpPhysicalPage(uintptr_t ptr) {
                return util::AlignUp(ptr, PhysicalPageSize);
            }

            template<typename T>
            static ALWAYS_INLINE T *AlignUpPhysicalPage(T *ptr) {
                static_assert(util::is_pod<T>::value);
                static_assert(util::IsAligned(PhysicalPageSize, alignof(T)));
                return reinterpret_cast<T *>(AlignUpPhysicalPage(reinterpret_cast<uintptr_t>(ptr)));
            }

            static constexpr ALWAYS_INLINE uintptr_t AlignDownPhysicalPage(uintptr_t ptr) {
                return util::AlignDown(ptr, PhysicalPageSize);
            }

            template<typename T>
            static ALWAYS_INLINE T *AlignDownPhysicalPage(T *ptr) {
                static_assert(util::is_pod<T>::value);
                static_assert(util::IsAligned(PhysicalPageSize, alignof(T)));
                return reinterpret_cast<T *>(AlignDownPhysicalPage(reinterpret_cast<uintptr_t>(ptr)));
            }

            static constexpr ALWAYS_INLINE size_t GetChunkSize(size_t cls) {
                return ClassInfos[cls].chunk_size;
            }

            static constexpr ALWAYS_INLINE size_t GetNumPages(size_t cls) {
                return ClassInfos[cls].num_pages;
            }
    };

}