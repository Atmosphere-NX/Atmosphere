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

namespace ams::dd {

    uintptr_t QueryIoMapping(uintptr_t phys_addr, size_t size);

    u32 ReadRegister(uintptr_t phys_addr);
    void WriteRegister(uintptr_t phys_addr, u32 value);
    u32 ReadWriteRegister(uintptr_t phys_addr, u32 value, u32 mask);

    /* Convenience Helper. */

    inline uintptr_t GetIoMapping(uintptr_t phys_addr, size_t size) {
        const uintptr_t io_mapping = QueryIoMapping(phys_addr, size);
        AMS_ABORT_UNLESS(io_mapping);
        return io_mapping;
    }

}
