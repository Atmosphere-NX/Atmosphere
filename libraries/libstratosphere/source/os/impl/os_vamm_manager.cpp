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
#include "os_vamm_manager.hpp"

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_vamm_manager_impl.os.horizon.hpp"
#elif defined(ATMOSPHERE_OS_WINDOWS)
    #include "os_vamm_manager_impl.os.windows.hpp"
#elif defined(ATMOSPHERE_OS_LINUX)
    #include "os_vamm_manager_impl.os.linux.hpp"
#elif defined(ATMOSPHERE_OS_MACOS)
    #include "os_vamm_manager_impl.os.macos.hpp"
#else
    #error "Unknown OS for VammManagerImpl"
#endif


namespace ams::os::impl {

    namespace {

        class AddressRegion : public util::IntrusiveRedBlackTreeBaseNode<AddressRegion> {
            private:
                uintptr_t m_address;
                size_t m_size;
            public:
                ALWAYS_INLINE AddressRegion(uintptr_t a, size_t s) : m_address(a), m_size(s) { /* ... */ }

                constexpr ALWAYS_INLINE uintptr_t GetAddressBegin() const { return m_address; }
                constexpr ALWAYS_INLINE uintptr_t GetAddressEnd() const { return m_address + m_size; }
                constexpr ALWAYS_INLINE size_t GetSize() const { return m_size; }

                constexpr ALWAYS_INLINE bool IsContained(uintptr_t address) const {
                    if (address < m_address) {
                        return false;
                    } else if (address < this->GetAddressEnd()) {
                        return true;
                    } else {
                        return false;
                    }
                }

                constexpr ALWAYS_INLINE bool IsContained(uintptr_t address, size_t size) const {
                    const uintptr_t end = address + size;

                    if (!(address <= end)) {
                        return false;
                    }
                    if (!(this->GetAddressBegin() <= address)) {
                        return false;
                    }
                    if (!(end <= this->GetAddressEnd())) {
                        return false;
                    }

                    return true;
                }
        };

        struct AddressRegionCompare {
            using RedBlackKeyType = uintptr_t;

            static constexpr ALWAYS_INLINE int Compare(const RedBlackKeyType a, const RedBlackKeyType &b) {
                if (a < b) {
                    return -1;
                } else if (a > b) {
                    return 1;
                } else {
                    return 0;
                }
            }

            static constexpr ALWAYS_INLINE int Compare(const RedBlackKeyType &a, const AddressRegion &b) {
                return Compare(a, b.GetAddressBegin());
            }

            static constexpr ALWAYS_INLINE int Compare(const AddressRegion &a, const AddressRegion &b) {
                return Compare(a.GetAddressBegin(), b.GetAddressBegin());
            }
        };

        using AddressRegionTree = util::IntrusiveRedBlackTreeBaseTraits<AddressRegion>::TreeType<AddressRegionCompare>;

        class DynamicUnitHeap {
            private:
                static constexpr size_t PhysicalMemoryUnitSize = MemoryPageSize;
            private:
                uintptr_t m_start;
                uintptr_t m_limit;
                uintptr_t m_end;
                util::TypedStorage<lmem::HeapCommonHead> m_head;
                lmem::HeapHandle m_heap;
            public:
                DynamicUnitHeap(uintptr_t address, size_t size, size_t unit_size) : m_start(address), m_end(address + size) {
                    /* Allocate the start of our buffer. */
                    VammManagerImpl::AllocatePhysicalMemoryImpl(m_start, PhysicalMemoryUnitSize);

                    /* Set our current limit. */
                    m_limit = m_start + PhysicalMemoryUnitSize;

                    /* Initialize our heap. */
                    m_heap = lmem::CreateUnitHeap(reinterpret_cast<void *>(m_start), PhysicalMemoryUnitSize, unit_size, lmem::CreateOption_None, alignof(u64), util::GetPointer(m_head));
                }

                void *Allocate() {
                    void *alloc = lmem::AllocateFromUnitHeap(m_heap);
                    if (alloc == nullptr) {
                        this->Extend();
                        alloc = lmem::AllocateFromUnitHeap(m_heap);
                    }
                    return alloc;
                }

                void Free(void *p) {
                    lmem::FreeToUnitHeap(m_heap, p);
                }
            private:
                void Extend() {
                    AMS_ABORT_UNLESS(m_limit < m_end);

                    if (R_SUCCEEDED(VammManagerImpl::AllocatePhysicalMemoryImpl(m_limit, PhysicalMemoryUnitSize))) {
                        m_limit += PhysicalMemoryUnitSize;
                        lmem::ExtendUnitHeap(m_heap, PhysicalMemoryUnitSize);
                    }
                }
        };

    }

    class AddressRegionManager {
        private:
            static constexpr size_t UnitHeapRegionSize = 1_GB - 2_MB;
        private:
            uintptr_t m_start;
            size_t m_size;
            AddressRegionTree m_tree;
            DynamicUnitHeap m_heap;
        public:
            AddressRegionManager(uintptr_t start, size_t size) : m_start(start), m_size(size), m_heap(start, UnitHeapRegionSize, sizeof(AddressRegion)) {
                /* Insert a block in the tree for our heap. */
                m_tree.insert(*(new (m_heap.Allocate()) AddressRegion(m_start, UnitHeapRegionSize)));

                /* Insert a zero-size block in the tree at the end of our heap. */
                m_tree.insert(*(new (m_heap.Allocate()) AddressRegion(m_start + size, 0)));
            }

            AddressAllocationResult Allocate(AddressRegion **out, size_t size) {
                /* Allocate a region. */
                void *p = m_heap.Allocate();
                if (p == nullptr) {
                    return AddressAllocationResult_OutOfMemory;
                }

                /* Determine alignment for the specified size. */
                const size_t align = SelectAlignment(size);

                /* Iterate, looking for an appropriate region. */
                auto *region = std::addressof(m_tree.back());
                while (true) {
                    /* Get the previous region. */
                    auto *prev = region->GetPrev();
                    if (prev == nullptr) {
                        break;
                    }

                    /* Get the space between prev and the current region. */
                    const uintptr_t space_start = prev->GetAddressEnd() + MemoryPageSize;
                    const uintptr_t space_end   = region->GetAddressBegin() - MemoryPageSize;
                    const size_t space_size     = space_end - space_start;

                    /* If there's enough space in the region, consider it further. */
                    if (space_size >= size) {
                        /* Determine the allocation region extents. */
                        const uintptr_t alloc_start = util::AlignUp(space_start, align);
                        const uintptr_t alloc_end   = alloc_start + size;

                        /* If the allocation works, use it. */
                        if (alloc_end <= space_end) {
                            auto *address_region = new (p) AddressRegion(alloc_start, size);
                            m_tree.insert(*address_region);
                            *out = address_region;
                            return AddressAllocationResult_Success;
                        }
                    }

                    /* Otherwise, continue. */
                    region = prev;
                }

                /* We ran out of space to allocate. */
                return AddressAllocationResult_OutOfSpace;
            }

            void Free(AddressRegion *region) {
                m_tree.erase(m_tree.iterator_to(*region));
                m_heap.Free(region);
            }

            bool IsAlreadyAllocated(uintptr_t address, size_t size) const {
                /* Find the first region >= our address. */
                auto region = std::addressof(*(m_tree.nfind_key(address)));
                if (region == nullptr) {
                    return false;
                }

                /* If the address matches, return whether the region is contained. */
                if (region->GetAddressBegin() == address) {
                    return size <= region->GetSize();
                }

                /* Otherwise, check the previous entry. */
                if (region = region->GetPrev(); region == nullptr) {
                    return false;
                }

                return region->IsContained(address, size);
            }

            AddressRegion *Find(uintptr_t address) const {
                return std::addressof(*(m_tree.find_key(address)));
            }
        private:
            static constexpr size_t SelectAlignment(size_t size) {
                if (size < 4_MB) {
                    if (size < 2_MB) {
                        return 64_KB;
                    } else {
                        return 2_MB;
                    }
                } else {
                    if (size < 32_MB) {
                        return 4_MB;
                    } else if (size < 1_GB) {
                        return 32_MB;
                    } else {
                        return 1_GB;
                    }
                }
            }
    };

    namespace {

        constinit util::TypedStorage<AddressRegionManager> g_address_region_manager_storage = {};

    }

    VammManager::VammManager() : m_lock(), m_region_manager(nullptr) {
        /* Get the reserved region. */
        VammManagerImpl::GetReservedRegionImpl(std::addressof(m_reserved_region_start), std::addressof(m_reserved_region_size));
    }

    void VammManager::InitializeIfEnabled() {
        /* Acquire exclusive/writer access. */
        std::scoped_lock lk(m_lock);

        /* Initialize, if we haven't already. */
        if (m_region_manager == nullptr && IsVirtualAddressMemoryEnabled()) {
            m_region_manager = util::ConstructAt(g_address_region_manager_storage, m_reserved_region_start, m_reserved_region_size);
        }
    }

    Result VammManager::AllocateAddressRegion(uintptr_t *out, size_t size) {
        /* Allocate an address. */
        uintptr_t address;
        {
            /* Lock access to our region manager. */
            std::scoped_lock lk(m_lock);
            AMS_ASSERT(m_region_manager != nullptr);

            /* Allocate an address region. */
            AddressRegion *region;
            switch (m_region_manager->Allocate(std::addressof(region), size)) {
                case AddressAllocationResult_Success:
                    address = region->GetAddressBegin();
                    break;
                case AddressAllocationResult_OutOfSpace:
                    R_THROW(os::ResultOutOfVirtualAddressSpace());
                default:
                    R_THROW(os::ResultOutOfMemory());
            }
        }

        /* Set the output. */
        *out = address;
        R_SUCCEED();
    }

    Result VammManager::AllocateMemory(uintptr_t *out, size_t size) {
        /* Allocate an address. */
        uintptr_t address;
        {
            /* Lock access to our region manager. */
            std::scoped_lock lk(m_lock);
            AMS_ASSERT(m_region_manager != nullptr);

            /* Allocate an address region. */
            AddressRegion *region;
            switch (m_region_manager->Allocate(std::addressof(region), size)) {
                case AddressAllocationResult_Success:
                    address = region->GetAddressBegin();
                    break;
                case AddressAllocationResult_OutOfSpace:
                    R_THROW(os::ResultOutOfVirtualAddressSpace());
                default:
                    R_THROW(os::ResultOutOfMemory());
            }
            ON_RESULT_FAILURE { m_region_manager->Free(region); };

            /* Allocate memory at the region. */
            R_TRY(VammManagerImpl::AllocatePhysicalMemoryImpl(address, size));
        }

        /* Set the output. */
        *out = address;
        R_SUCCEED();
    }

    Result VammManager::AllocateMemoryPages(uintptr_t address, size_t size) {
        /* Acquire read access to our region manager. */
        std::shared_lock lk(m_lock);
        AMS_ASSERT(m_region_manager != nullptr);

        /* Check that the region was previously allocated by a call to AllocateAddressRegion. */
        R_UNLESS(m_region_manager->IsAlreadyAllocated(address, size), os::ResultInvalidParameter());

        /* Allocate the memory. */
        R_RETURN(VammManagerImpl::AllocatePhysicalMemoryImpl(address, size));
    }

    Result VammManager::FreeAddressRegion(uintptr_t address) {
        /* Lock access to our region manager. */
        std::scoped_lock lk(m_lock);
        AMS_ASSERT(m_region_manager != nullptr);

        /* Verify the region can be freed. */
        auto *region = m_region_manager->Find(address);
        R_UNLESS(region != nullptr, os::ResultInvalidParameter());

        /* Free any memory present at the address. */
        R_TRY(VammManagerImpl::FreePhysicalMemoryImpl(address, region->GetSize()));

        /* Free the region. */
        m_region_manager->Free(region);
        R_SUCCEED();
    }

    Result VammManager::FreeMemoryPages(uintptr_t address, size_t size) {
        /* Acquire read access to our region manager. */
        std::shared_lock lk(m_lock);
        AMS_ASSERT(m_region_manager != nullptr);

        /* Check that the region was previously allocated by a call to AllocateAddressRegion. */
        R_UNLESS(m_region_manager->IsAlreadyAllocated(address, size), os::ResultInvalidParameter());

        /* Free the memory. */
        R_RETURN(VammManagerImpl::FreePhysicalMemoryImpl(address, size));
    }

    VirtualAddressMemoryResourceUsage VammManager::GetVirtualAddressMemoryResourceUsage() {
        const size_t assigned_size = VammManagerImpl::GetExtraSystemResourceAssignedSize();
        const size_t used_size     = VammManagerImpl::GetExtraSystemResourceUsedSize();

        /* Decide on an actual used size. */
        const size_t reported_used_size = std::min<size_t>(assigned_size, used_size + 512_KB);

        return VirtualAddressMemoryResourceUsage {
            .assigned_size = assigned_size,
            .used_size     = reported_used_size,
        };
    }

    bool VammManager::IsVirtualAddressMemoryEnabled() {
        return VammManagerImpl::IsVirtualAddressMemoryEnabled();
    }

}
