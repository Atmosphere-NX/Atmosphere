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
#include <mesosphere/kern_k_slab_heap.hpp>

namespace ams::kern::init {
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


    struct KSlabResourceCounts {
        size_t num_KProcess;
        size_t num_KThread;
        size_t num_KEvent;
        size_t num_KInterruptEvent;
        size_t num_KPort;
        size_t num_KSharedMemory;
        size_t num_KTransferMemory;
        size_t num_KCodeMemory;
        size_t num_KDeviceAddressSpace;
        size_t num_KSession;
        size_t num_KLightSession;
        size_t num_KObjectName;
        size_t num_KResourceLimit;
        size_t num_KDebug;
        size_t num_KIoPool;
        size_t num_KIoRegion;
        size_t num_KSessionRequestMappings;
    };

    NOINLINE void InitializeSlabResourceCounts();
    const KSlabResourceCounts &GetSlabResourceCounts();

    size_t CalculateTotalSlabHeapSize();
    NOINLINE void InitializeSlabHeaps();

}
