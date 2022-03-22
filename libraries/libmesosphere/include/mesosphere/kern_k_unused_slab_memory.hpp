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
#include <mesosphere/kern_k_typed_address.hpp>

namespace ams::kern {

    /* Utilities to allocate/free memory from the "unused" gaps between slab heaps. */
    /* See KTargetSystem::IsDynamicResourceLimitsEnabled() usage for more context. */
    KVirtualAddress AllocateUnusedSlabMemory(size_t size, size_t alignment);
    void FreeUnusedSlabMemory(KVirtualAddress address, size_t size);

}
