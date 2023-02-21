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

namespace ams::kern {

    namespace {

        constinit KLightLock g_io_pool_lock;
        constinit bool g_pool_used[ams::svc::IoPoolType_Count];

        struct IoRegionExtents {
            KPhysicalAddress address;
            size_t size;
        };

        #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)

            #include "board/nintendo/nx/kern_k_io_pool.board.nintendo_nx.inc"

        #elif defined(AMS_SVC_IO_POOL_NOT_SUPPORTED)

            #include "kern_k_io_pool.unsupported.inc"

        #else

            #error "Unknown context for IoPoolType!"

        #endif

        constexpr bool IsValidIoRegionImpl(ams::svc::IoPoolType pool_type, KPhysicalAddress address, size_t size) {
            /* NOTE: It seems likely this depends on pool type, but this isn't confirmable as of now. */
            MESOSPHERE_UNUSED(pool_type);

            /* Check if the address/size falls within any allowable extents. */
            for (const auto &extents : g_io_region_extents) {
                if (extents.size != 0 && extents.address <= address && address + size - 1 <= extents.address + extents.size - 1) {
                    return true;
                }
            }

            return false;
        }

    }

    bool KIoPool::IsValidIoPoolType(ams::svc::IoPoolType pool_type) {
        return IsValidIoPoolTypeImpl(pool_type);
    }

    Result KIoPool::Initialize(ams::svc::IoPoolType pool_type) {
        MESOSPHERE_ASSERT_THIS();

        /* Register the pool type. */
        {
            /* Lock the pool used table. */
            KScopedLightLock lk(g_io_pool_lock);

            /* Check that the pool isn't already used. */
            R_UNLESS(!g_pool_used[pool_type], svc::ResultBusy());

            /* Set the pool as used. */
            g_pool_used[pool_type] = true;
        }

        /* Set our fields. */
        m_pool_type      = pool_type;
        m_is_initialized = true;

        R_SUCCEED();
    }

    void KIoPool::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Lock the pool used table. */
        KScopedLightLock lk(g_io_pool_lock);

        /* Check that the pool is used. */
        MESOSPHERE_ASSERT(g_pool_used[m_pool_type]);

        /* Set the pool as unused. */
        g_pool_used[m_pool_type] = false;
    }

    Result KIoPool::AddIoRegion(KIoRegion *new_region) {
        MESOSPHERE_ASSERT_THIS();

        /* Check that the region is allowed. */
        R_UNLESS(IsValidIoRegionImpl(m_pool_type, new_region->GetAddress(), new_region->GetSize()), svc::ResultInvalidMemoryRegion());

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Check that the desired range isn't already in our pool. */
        {
            /* Get the lowest region with address >= the new region that's already in our tree. */
            auto lowest_after = m_io_region_tree.nfind_key(new_region->GetAddress());
            if (lowest_after != m_io_region_tree.end()) {
                R_UNLESS(!lowest_after->Overlaps(new_region->GetAddress(), new_region->GetSize()), svc::ResultBusy());
            }

            /* There is no region with address >= the new region already in our tree, but we also need to check */
            /* for a region with address < the new region already in our tree. */
            if (lowest_after != m_io_region_tree.begin()) {
                auto highest_before = --lowest_after;
                R_UNLESS(!highest_before->Overlaps(new_region->GetAddress(), new_region->GetSize()), svc::ResultBusy());
            }
        }

        /* Add the region to our pool. */
        m_io_region_tree.insert(*new_region);

        R_SUCCEED();
    }

    void KIoPool::RemoveIoRegion(KIoRegion *region) {
        MESOSPHERE_ASSERT_THIS();

        /* Lock ourselves. */
        KScopedLightLock lk(m_lock);

        /* Remove the region from our tree. */
        m_io_region_tree.erase(m_io_region_tree.iterator_to(*region));
    }

}
