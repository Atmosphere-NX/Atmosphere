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
#include <stratosphere/os/os_memory_permission.hpp>

namespace ams::os {

    struct TransferMemoryType;

    Result CreateTransferMemory(TransferMemoryType *tmem, void *address, size_t size, MemoryPermission perm);

    Result AttachTransferMemory(TransferMemoryType *tmem, size_t size, Handle handle, bool managed);
    Handle DetachTransferMemory(TransferMemoryType *tmem);

    void DestroyTransferMemory(TransferMemoryType *tmem);

    Result MapTransferMemory(void **out, TransferMemoryType *tmem, MemoryPermission owner_perm);
    void UnmapTransferMemory(TransferMemoryType *tmem);

}
