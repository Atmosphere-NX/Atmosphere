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
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "utils.h"

static void copy_forwards(uint8_t *dst, const uint8_t *src, size_t size) {
    for (int i = 0; i < size; ++i) {
        dst[i] = src[i];
    }
}

static void copy_backwards(uint8_t *dst, const uint8_t *src, size_t size) {
    for (int i = size - 1; i >= 0; --i) {
        dst[i] = src[i];
    }
}

void loader_memcpy(void *dst, const void *src, size_t size) {
    copy_forwards(dst, src, size);
}

void loader_memmove(void *dst, const void *src, size_t size) {
    const uintptr_t dst_u = (uintptr_t)dst;
    const uintptr_t src_u = (uintptr_t)src;

    if (dst_u < src_u) {
        copy_forwards(dst, src, size);
    } else if (dst_u > src_u) {
        copy_backwards(dst, src, size);
    } else {
        /* Nothing to do */
    }
}