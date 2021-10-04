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
#include <stratosphere.hpp>
#include "../mem_impl_platform.hpp"

namespace ams::mem::impl::heap {

    using Prot = mem::impl::Prot;

    inline errno_t AllocateVirtualMemory(void **ptr, size_t size) {
        return ::ams::mem::impl::virtual_alloc(ptr, size);
    }

    inline errno_t FreeVirtualMemory(void *ptr, size_t size) {
        return ::ams::mem::impl::virtual_free(ptr, size);
    }

    inline errno_t AllocatePhysicalMemory(void *ptr, size_t size) {
        return ::ams::mem::impl::physical_alloc(ptr, size, static_cast<Prot>(Prot_read | Prot_write));
    }

    inline errno_t FreePhysicalMemory(void *ptr, size_t size) {
        return ::ams::mem::impl::physical_free(ptr, size);
    }

}