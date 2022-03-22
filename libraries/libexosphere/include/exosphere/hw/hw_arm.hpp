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
#include <vapours.hpp>

namespace ams::hw::arch::arm {

#ifdef __BPMP__
    constexpr inline size_t DataCacheLineSize = 0x20;
    constexpr inline size_t DataCacheSize = 32_KB;

    ALWAYS_INLINE void DataSynchronizationBarrier() {
        /* ... */
    }

    ALWAYS_INLINE void DataSynchronizationBarrierInnerShareable() {
        /* ... */
    }

    ALWAYS_INLINE void DataMemoryBarrier() {
        /* ... */
    }

    ALWAYS_INLINE void InstructionSynchronizationBarrier() {
        /* ... */
    }

    void InitializeDataCache();
    void FinalizeDataCache();

    void InvalidateEntireDataCache();
    void StoreEntireDataCache();
    void FlushEntireDataCache();

    void InvalidateDataCacheLine(void *ptr);
    void StoreDataCacheLine(void *ptr);
    void FlushDataCacheLine(void *ptr);

    void InvalidateDataCache(void *ptr, size_t size);
    void StoreDataCache(const void *ptr, size_t size);
    void FlushDataCache(const void *ptr, size_t size);
#else
    #error "Unknown ARM board for ams::hw"
#endif

}