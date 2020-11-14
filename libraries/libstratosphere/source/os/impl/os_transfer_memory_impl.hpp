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
#include <stratosphere.hpp>

namespace ams::os::impl {

    class TransferMemoryImpl {
        public:
            static Result Create(Handle *out, void *address, size_t size, MemoryPermission perm);
            static void Close(Handle handle);

            static Result Map(void **out, Handle handle, void *address, size_t size, MemoryPermission owner_perm);
            static void Unmap(Handle handle, void *address, size_t size);
    };

}