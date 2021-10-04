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
#include <mesosphere/kern_common.hpp>

#ifdef ATMOSPHERE_ARCH_ARM64
    #include <mesosphere/arch/arm64/init/kern_k_init_page_table.hpp>

    namespace ams::kern::init {
        using ams::kern::arch::arm64::PageTableEntry;
        using ams::kern::arch::arm64::init::KInitialPageTable;
        using ams::kern::arch::arm64::init::KInitialPageAllocator;
    }
#else
    #error "Unknown architecture for KInitialPageTable"
#endif
