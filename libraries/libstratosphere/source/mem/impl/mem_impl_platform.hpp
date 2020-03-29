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

namespace ams::mem::impl {

    enum Prot {
        Prot_none  = (0 << 0),
        Prot_read  = (1 << 0),
        Prot_write = (1 << 1),
        Prot_exec  = (1 << 2),
    };

    errno_t virtual_alloc(void **ptr, size_t size);
    errno_t virtual_free(void *ptr, size_t size);
    errno_t physical_alloc(void *ptr, size_t size, Prot prot);
    errno_t physical_free(void *ptr, size_t size);

    size_t strlcpy(char *dst, const char *src, size_t size);

    errno_t gen_random(void *dst, size_t dst_size);

    errno_t epochtime(s64 *dst);

    errno_t getcpu(s32 *out);

}
