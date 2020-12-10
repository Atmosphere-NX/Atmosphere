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
#include <mesosphere.hpp>

namespace ams::kern::init {

    #define SLAB_COUNT(CLASS) g_slab_resource_counts.num_##CLASS

    #define FOREACH_SLAB_TYPE(HANDLER, ...)                                                                                     \
        HANDLER(KProcess,            (SLAB_COUNT(KProcess)),                                                    ## __VA_ARGS__) \
        HANDLER(KThread,             (SLAB_COUNT(KThread)),                                                     ## __VA_ARGS__) \
        HANDLER(KLinkedListNode,     (SLAB_COUNT(KThread)),                                                     ## __VA_ARGS__) \
        HANDLER(KEvent,              (SLAB_COUNT(KEvent)),                                                      ## __VA_ARGS__) \
        HANDLER(KInterruptEvent,     (SLAB_COUNT(KInterruptEvent)),                                             ## __VA_ARGS__) \
        HANDLER(KInterruptEventTask, (SLAB_COUNT(KInterruptEvent)),                                             ## __VA_ARGS__) \
        HANDLER(KPort,               (SLAB_COUNT(KPort)),                                                       ## __VA_ARGS__) \
        HANDLER(KSharedMemory,       (SLAB_COUNT(KSharedMemory)),                                               ## __VA_ARGS__) \
        HANDLER(KSharedMemoryInfo,   (SLAB_COUNT(KSharedMemory) * 8),                                           ## __VA_ARGS__) \
        HANDLER(KTransferMemory,     (SLAB_COUNT(KTransferMemory)),                                             ## __VA_ARGS__) \
        HANDLER(KCodeMemory,         (SLAB_COUNT(KCodeMemory)),                                                 ## __VA_ARGS__) \
        HANDLER(KDeviceAddressSpace, (SLAB_COUNT(KDeviceAddressSpace)),                                         ## __VA_ARGS__) \
        HANDLER(KSession,            (SLAB_COUNT(KSession)),                                                    ## __VA_ARGS__) \
        HANDLER(KSessionRequest,     (SLAB_COUNT(KSession) * 2),                                                ## __VA_ARGS__) \
        HANDLER(KLightSession,       (SLAB_COUNT(KLightSession)),                                               ## __VA_ARGS__) \
        HANDLER(KThreadLocalPage,    (SLAB_COUNT(KProcess) + (SLAB_COUNT(KProcess) + SLAB_COUNT(KThread)) / 8), ## __VA_ARGS__) \
        HANDLER(KObjectName,         (SLAB_COUNT(KObjectName)),                                                 ## __VA_ARGS__) \
        HANDLER(KResourceLimit,      (SLAB_COUNT(KResourceLimit)),                                              ## __VA_ARGS__) \
        HANDLER(KEventInfo,          (SLAB_COUNT(KThread) + SLAB_COUNT(KDebug)),                                ## __VA_ARGS__) \
        HANDLER(KDebug,              (SLAB_COUNT(KDebug)),                                                      ## __VA_ARGS__) \
        HANDLER(KAlpha,              (SLAB_COUNT(KAlpha)),                                                      ## __VA_ARGS__) \
        HANDLER(KBeta,               (SLAB_COUNT(KBeta)),                                                       ## __VA_ARGS__)

    namespace {


        #define DEFINE_SLAB_TYPE_ENUM_MEMBER(NAME, COUNT, ...) KSlabType_##NAME,

        enum KSlabType : u32 {
            FOREACH_SLAB_TYPE(DEFINE_SLAB_TYPE_ENUM_MEMBER)
            KSlabType_Count,
        };

        #undef DEFINE_SLAB_TYPE_ENUM_MEMBER

        /* Constexpr counts. */
        constexpr size_t SlabCountKProcess              = 80;
        constexpr size_t SlabCountKThread               = 800;
        constexpr size_t SlabCountKEvent                = 700;
        constexpr size_t SlabCountKInterruptEvent       = 100;
        constexpr size_t SlabCountKPort                 = 256;
        constexpr size_t SlabCountKSharedMemory         = 80;
        constexpr size_t SlabCountKTransferMemory       = 200;
        constexpr size_t SlabCountKCodeMemory           = 10;
        constexpr size_t SlabCountKDeviceAddressSpace   = 300;
        constexpr size_t SlabCountKSession              = 933;
        constexpr size_t SlabCountKLightSession         = 100;
        constexpr size_t SlabCountKObjectName           = 7;
        constexpr size_t SlabCountKResourceLimit        = 5;
        constexpr size_t SlabCountKDebug                = cpu::NumCores;
        constexpr size_t SlabCountKAlpha                = 1;
        constexpr size_t SlabCountKBeta                 = 6;

        constexpr size_t SlabCountExtraKThread          = 160;

        namespace test {

            constexpr size_t RequiredSizeForExtraThreadCount = SlabCountExtraKThread * (sizeof(KThread) + sizeof(KLinkedListNode) + (sizeof(KThreadLocalPage) / 8) + sizeof(KEventInfo));
            static_assert(RequiredSizeForExtraThreadCount <= KernelSlabHeapAdditionalSize);

        }

        /* Global to hold our resource counts. */
        KSlabResourceCounts g_slab_resource_counts = {
            .num_KProcess               = SlabCountKProcess,
            .num_KThread                = SlabCountKThread,
            .num_KEvent                 = SlabCountKEvent,
            .num_KInterruptEvent        = SlabCountKInterruptEvent,
            .num_KPort                  = SlabCountKPort,
            .num_KSharedMemory          = SlabCountKSharedMemory,
            .num_KTransferMemory        = SlabCountKTransferMemory,
            .num_KCodeMemory            = SlabCountKCodeMemory,
            .num_KDeviceAddressSpace    = SlabCountKDeviceAddressSpace,
            .num_KSession               = SlabCountKSession,
            .num_KLightSession          = SlabCountKLightSession,
            .num_KObjectName            = SlabCountKObjectName,
            .num_KResourceLimit         = SlabCountKResourceLimit,
            .num_KDebug                 = SlabCountKDebug,
            .num_KAlpha                 = SlabCountKAlpha,
            .num_KBeta                  = SlabCountKBeta,
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
        return (kern::GetTargetFirmware() >= TargetFirmware_10_0_0) ? KernelSlabHeapGapsSize : KernelSlabHeapGapsSizeDeprecated;
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

    void InitializeKPageBufferSlabHeap() {
        const auto &counts = GetSlabResourceCounts();
        const size_t num_pages = counts.num_KProcess + counts.num_KThread + (counts.num_KProcess + counts.num_KThread) / 8;
        const size_t slab_size = num_pages * PageSize;

        /* Reserve memory from the system resource limit. */
        MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, slab_size));

        /* Allocate memory for the slab. */
        constexpr auto AllocateOption = KMemoryManager::EncodeOption(KMemoryManager::Pool_System, KMemoryManager::Direction_FromFront);
        const KVirtualAddress slab_address = Kernel::GetMemoryManager().AllocateAndOpenContinuous(num_pages, 1, AllocateOption);
        MESOSPHERE_ABORT_UNLESS(slab_address != Null<KVirtualAddress>);

        /* Initialize the slabheap. */
        KPageBuffer::InitializeSlabHeap(GetVoidPointer(slab_address), slab_size);
    }

    void InitializeSlabHeaps() {
        /* Get the start of the slab region, since that's where we'll be working. */
        KVirtualAddress address = KMemoryLayout::GetSlabRegionAddress();

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

        for (size_t i = 0; i < util::size(slab_types); i++) {
            /* Add the random gap to the address. */
            address += (i == 0) ? slab_gaps[0] : slab_gaps[i] - slab_gaps[i - 1];

            #define INITIALIZE_SLAB_HEAP(NAME, COUNT, ...)              \
                case KSlabType_##NAME:                                  \
                    address = InitializeSlabHeap<NAME>(address, COUNT); \
                    break;

            /* Initialize the slabheap. */
            switch (slab_types[i]) {
                /* For each of the slab types, we want to initialize that heap. */
                FOREACH_SLAB_TYPE(INITIALIZE_SLAB_HEAP)
                /* If we somehow get an invalid type, abort. */
                MESOSPHERE_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

}