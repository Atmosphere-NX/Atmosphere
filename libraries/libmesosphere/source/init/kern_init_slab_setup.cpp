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
#include <mesosphere.hpp>

namespace ams::kern::init {

    /* For macro convenience. */
    using KSessionRequestMappings = KSessionRequest::SessionMappings::DynamicMappings;
    using KThreadLockInfo         = KThread::LockWithPriorityInheritanceInfo;

    #define SLAB_COUNT(CLASS) g_slab_resource_counts.num_##CLASS

    #define FOREACH_SLAB_TYPE(HANDLER, ...)                                                                                     \
        HANDLER(KProcess,                (SLAB_COUNT(KProcess)),                                                    ## __VA_ARGS__) \
        HANDLER(KThread,                 (SLAB_COUNT(KThread)),                                                     ## __VA_ARGS__) \
        HANDLER(KEvent,                  (SLAB_COUNT(KEvent)),                                                      ## __VA_ARGS__) \
        HANDLER(KInterruptEvent,         (SLAB_COUNT(KInterruptEvent)),                                             ## __VA_ARGS__) \
        HANDLER(KPort,                   (SLAB_COUNT(KPort)),                                                       ## __VA_ARGS__) \
        HANDLER(KSharedMemory,           (SLAB_COUNT(KSharedMemory)),                                               ## __VA_ARGS__) \
        HANDLER(KSharedMemoryInfo,       (SLAB_COUNT(KSharedMemory) * 8),                                           ## __VA_ARGS__) \
        HANDLER(KTransferMemory,         (SLAB_COUNT(KTransferMemory)),                                             ## __VA_ARGS__) \
        HANDLER(KCodeMemory,             (SLAB_COUNT(KCodeMemory)),                                                 ## __VA_ARGS__) \
        HANDLER(KDeviceAddressSpace,     (SLAB_COUNT(KDeviceAddressSpace)),                                         ## __VA_ARGS__) \
        HANDLER(KSession,                (SLAB_COUNT(KSession)),                                                    ## __VA_ARGS__) \
        HANDLER(KSessionRequest,         (SLAB_COUNT(KSession) * 2),                                                ## __VA_ARGS__) \
        HANDLER(KLightSession,           (SLAB_COUNT(KLightSession)),                                               ## __VA_ARGS__) \
        HANDLER(KThreadLocalPage,        (SLAB_COUNT(KProcess) + (SLAB_COUNT(KProcess) + SLAB_COUNT(KThread)) / 8), ## __VA_ARGS__) \
        HANDLER(KObjectName,             (SLAB_COUNT(KObjectName)),                                                 ## __VA_ARGS__) \
        HANDLER(KResourceLimit,          (SLAB_COUNT(KResourceLimit)),                                              ## __VA_ARGS__) \
        HANDLER(KEventInfo,              (SLAB_COUNT(KThread) + SLAB_COUNT(KDebug)),                                ## __VA_ARGS__) \
        HANDLER(KDebug,                  (SLAB_COUNT(KDebug)),                                                      ## __VA_ARGS__) \
        HANDLER(KIoPool,                 (SLAB_COUNT(KIoPool)),                                                     ## __VA_ARGS__) \
        HANDLER(KIoRegion,               (SLAB_COUNT(KIoRegion)),                                                   ## __VA_ARGS__) \
        HANDLER(KSessionRequestMappings, (SLAB_COUNT(KSessionRequestMappings)),                                     ## __VA_ARGS__) \
        HANDLER(KSecureSystemResource,   (SLAB_COUNT(KProcess)),                                                    ## __VA_ARGS__) \
        HANDLER(KThreadLockInfo,         (SLAB_COUNT(KThread)),                                                     ## __VA_ARGS__)

    namespace {


        #define DEFINE_SLAB_TYPE_ENUM_MEMBER(NAME, COUNT, ...) KSlabType_##NAME,

        enum KSlabType : u32 {
            FOREACH_SLAB_TYPE(DEFINE_SLAB_TYPE_ENUM_MEMBER)
            KSlabType_Count,
        };

        #undef DEFINE_SLAB_TYPE_ENUM_MEMBER

        /* Constexpr counts. */
        constexpr size_t SlabCountKProcess                = 80;
        constexpr size_t SlabCountKThread                 = 800;
        constexpr size_t SlabCountKEvent                  = 900;
        constexpr size_t SlabCountKInterruptEvent         = 100;
        constexpr size_t SlabCountKPort                   = 384;
        constexpr size_t SlabCountKSharedMemory           = 80;
        constexpr size_t SlabCountKTransferMemory         = 200;
        constexpr size_t SlabCountKCodeMemory             = 10;
        constexpr size_t SlabCountKDeviceAddressSpace     = 300;
        constexpr size_t SlabCountKSession                = 1133;
        constexpr size_t SlabCountKLightSession           = 100;
        constexpr size_t SlabCountKObjectName             = 7;
        constexpr size_t SlabCountKResourceLimit          = 5;
        constexpr size_t SlabCountKDebug                  = cpu::NumCores;
        constexpr size_t SlabCountKIoPool                 = 1;
        constexpr size_t SlabCountKIoRegion               = 6;
        constexpr size_t SlabcountKSessionRequestMappings = 40;

        constexpr size_t SlabCountExtraKThread            = (1024 + 256 + 256) - SlabCountKThread;

        namespace test {

            constexpr size_t RequiredSizeForExtraThreadCount = SlabCountExtraKThread * (sizeof(KThread) + (sizeof(KThreadLocalPage) / 8) + sizeof(KEventInfo));
            static_assert(RequiredSizeForExtraThreadCount <= KernelSlabHeapAdditionalSize);

            static_assert(KernelPageBufferHeapSize == 2 * PageSize + (SlabCountKProcess + SlabCountKThread + (SlabCountKProcess + SlabCountKThread) / 8) * PageSize);
            static_assert(KernelPageBufferAdditionalSize == (SlabCountExtraKThread + (SlabCountExtraKThread / 8)) * PageSize);

        }

        /* Global to hold our resource counts. */
        constinit KSlabResourceCounts g_slab_resource_counts = {
            .num_KProcess                = SlabCountKProcess,
            .num_KThread                 = SlabCountKThread,
            .num_KEvent                  = SlabCountKEvent,
            .num_KInterruptEvent         = SlabCountKInterruptEvent,
            .num_KPort                   = SlabCountKPort,
            .num_KSharedMemory           = SlabCountKSharedMemory,
            .num_KTransferMemory         = SlabCountKTransferMemory,
            .num_KCodeMemory             = SlabCountKCodeMemory,
            .num_KDeviceAddressSpace     = SlabCountKDeviceAddressSpace,
            .num_KSession                = SlabCountKSession,
            .num_KLightSession           = SlabCountKLightSession,
            .num_KObjectName             = SlabCountKObjectName,
            .num_KResourceLimit          = SlabCountKResourceLimit,
            .num_KDebug                  = SlabCountKDebug,
            .num_KIoPool                 = SlabCountKIoPool,
            .num_KIoRegion               = SlabCountKIoRegion,
            .num_KSessionRequestMappings = SlabcountKSessionRequestMappings,
        };

        template<typename T>
        NOINLINE KVirtualAddress InitializeSlabHeap(KVirtualAddress address, size_t num_objects) {
            const size_t size = util::AlignUp(sizeof(T) * num_objects, alignof(void *));
            KVirtualAddress start = util::AlignUp(GetInteger(address), alignof(T));

            if (size > 0) {
                const KMemoryRegion *region = KMemoryLayout::Find(start + size - 1);
                MESOSPHERE_ABORT_UNLESS(region != nullptr);
                MESOSPHERE_ABORT_UNLESS(region->IsDerivedFrom(KMemoryRegionType_KernelSlab));
                T::InitializeSlabHeap(GetVoidPointer(start), size);
            }

            return start + size;
        }

    }


    const KSlabResourceCounts &GetSlabResourceCounts() {
        return g_slab_resource_counts;
    }

    void InitializeSlabResourceCounts() {
        /* Note: Nintendo initializes all fields here, but we initialize all constants at compile-time. */
        if (KSystemControl::Init::ShouldIncreaseThreadResourceLimit()) {
            g_slab_resource_counts.num_KThread += SlabCountExtraKThread;
        }
    }

    size_t CalculateSlabHeapGapSize() {
        constexpr size_t KernelSlabHeapGapSize = 2_MB - 356_KB;
        static_assert(KernelSlabHeapGapSize <= KernelSlabHeapGapsSizeMax);
        return KernelSlabHeapGapSize;
    }

    size_t CalculateTotalSlabHeapSize() {
        size_t size = 0;

        #define ADD_SLAB_SIZE(NAME, COUNT, ...) ({                          \
            size += alignof(NAME);                                          \
            size += util::AlignUp(sizeof(NAME) * (COUNT), alignof(void *)); \
        });

        /* Add the size required for each slab. */
        FOREACH_SLAB_TYPE(ADD_SLAB_SIZE)

        #undef ADD_SLAB_SIZE

        /* Add the reserved size. */
        size += CalculateSlabHeapGapSize();

        return size;
    }

    void InitializeSlabHeaps() {
        /* Get the slab region, since that's where we'll be working. */
        const KMemoryRegion &slab_region = KMemoryLayout::GetSlabRegion();
        KVirtualAddress address = slab_region.GetAddress();

        /* Initialize slab type array to be in sorted order. */
        KSlabType slab_types[KSlabType_Count];
        for (size_t i = 0; i < util::size(slab_types); i++) { slab_types[i] = static_cast<KSlabType>(i); }

        /* N shuffles the slab type array with the following simple algorithm. */
        for (size_t i = 0; i < util::size(slab_types); i++) {
            const size_t rnd = KSystemControl::GenerateRandomRange(0, util::size(slab_types) - 1);
            std::swap(slab_types[i], slab_types[rnd]);
        }

        /* Create an array to represent the gaps between the slabs. */
        const size_t total_gap_size = CalculateSlabHeapGapSize();
        size_t slab_gaps[util::size(slab_types)];
        for (size_t i = 0; i < util::size(slab_gaps); i++) {
            /* Note: This is an off-by-one error from Nintendo's intention, because GenerateRandomRange is inclusive. */
            /* However, Nintendo also has the off-by-one error, and it's "harmless", so we will include it ourselves. */
            slab_gaps[i] = KSystemControl::GenerateRandomRange(0, total_gap_size);
        }

        /* Sort the array, so that we can treat differences between values as offsets to the starts of slabs. */
        for (size_t i = 1; i < util::size(slab_gaps); i++) {
            for (size_t j = i; j > 0 && slab_gaps[j-1] > slab_gaps[j]; j--) {
                std::swap(slab_gaps[j], slab_gaps[j-1]);
            }
        }

        /* Track the gaps, so that we can free them to the unused slab tree. */
        KVirtualAddress gap_start = address;
        size_t gap_size = 0;

        for (size_t i = 0; i < util::size(slab_types); i++) {
            /* Add the random gap to the address. */
            const auto cur_gap = (i == 0) ? slab_gaps[0] : slab_gaps[i] - slab_gaps[i - 1];
            address  += cur_gap;
            gap_size += cur_gap;

            #define INITIALIZE_SLAB_HEAP(NAME, COUNT, ...)                  \
                case KSlabType_##NAME:                                      \
                    if (COUNT > 0) {                                        \
                        address = InitializeSlabHeap<NAME>(address, COUNT); \
                    }                                                       \
                    break;

            /* Initialize the slabheap. */
            switch (slab_types[i]) {
                /* For each of the slab types, we want to initialize that heap. */
                FOREACH_SLAB_TYPE(INITIALIZE_SLAB_HEAP)
                /* If we somehow get an invalid type, abort. */
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }

            /* If we've hit the end of a gap, free it. */
            if (gap_start + gap_size != address) {
                FreeUnusedSlabMemory(gap_start, gap_size);
                gap_start = address;
                gap_size  = 0;
            }
        }

        /* Free the end of the slab region. */
        FreeUnusedSlabMemory(gap_start, gap_size + (slab_region.GetEndAddress() - GetInteger(address)));
    }

}

namespace ams::kern {

    void KPageBufferSlabHeap::Initialize(KDynamicPageManager &allocator) {
        /* Get slab resource counts. */
        const auto &counts = init::GetSlabResourceCounts();

        /* If size is correct, account for thread local pages. */
        if (BufferSize == PageSize) {
            s_buffer_count += counts.num_KProcess + counts.num_KThread + (counts.num_KProcess + counts.num_KThread) / 8;
        }

        /* Set our object size. */
        m_obj_size = BufferSize;

        /* Initialize the base allocator. */
        KSlabHeapImpl::Initialize();

        /* Allocate the desired page count. */
        for (size_t i = 0; i < s_buffer_count; ++i) {
            /* Allocate an appropriate buffer. */
            auto * const pb = (BufferSize <= PageSize) ? allocator.Allocate() : allocator.Allocate(BufferSize / PageSize);
            MESOSPHERE_ABORT_UNLESS(pb != nullptr);

            /* Free to our slab. */
            KSlabHeapImpl::Free(pb);
        }
    }

}
