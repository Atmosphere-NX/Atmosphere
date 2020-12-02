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
#include <vapours.hpp>
#include <mesosphere/kern_k_slab_heap.hpp>

namespace ams::kern::init {

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
        size_t num_KAlpha;
        size_t num_KBeta;
    };

    NOINLINE void InitializeSlabResourceCounts();
    const KSlabResourceCounts &GetSlabResourceCounts();

    size_t CalculateTotalSlabHeapSize();
    NOINLINE void InitializeKPageBufferSlabHeap();
    NOINLINE void InitializeSlabHeaps();

}
