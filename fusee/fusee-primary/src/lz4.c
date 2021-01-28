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
#include "lz4.h"
#include "utils.h"

typedef struct {
    const uint8_t *src;
    size_t src_size;
    size_t src_offset;
    uint8_t *dst;
    size_t dst_size;
    size_t dst_offset;
} lz4_ctx;

static uint8_t lz4_ctx_read_byte(lz4_ctx *ctx) {
    return ctx->src[ctx->src_offset++];
}

static bool lz4_ctx_can_read(const lz4_ctx *ctx) {
    return ctx->src_offset < ctx->src_size;
}

static size_t lz4_ctx_get_copy_size(lz4_ctx *ctx, uint8_t ctrl) {
    size_t size = ctrl;

    if (ctrl >= 0xF) {
        do {
            while (!lz4_ctx_can_read(ctx));
            ctrl = lz4_ctx_read_byte(ctx);
            size += ctrl;
        } while (ctrl == 0xFF);
    }

    return size;
}

static void lz4_ctx_copy(lz4_ctx *ctx, size_t size) {
    /* Perform the copy. */
    loader_memcpy(ctx->dst + ctx->dst_offset, ctx->src + ctx->src_offset, size);

    ctx->dst_offset += size;
    ctx->src_offset += size;
}

static void lz4_ctx_uncompress(lz4_ctx *ctx) {
    while (true) {
        /* Read a control byte. */
        const uint8_t control = lz4_ctx_read_byte(ctx);

        /* Copy what it specifies we should copy. */
        lz4_ctx_copy(ctx, lz4_ctx_get_copy_size(ctx, control >> 4));

        /* If we've exceeded size, we're done. */
        if (ctx->src_offset >= ctx->src_size) {
            break;
        }

        /* Read the wide copy offset. */
        uint16_t wide_offset = lz4_ctx_read_byte(ctx);
        while (!lz4_ctx_can_read(ctx));
        wide_offset |= (lz4_ctx_read_byte(ctx) << 8);

        /* Determine the copy size. */
        const size_t wide_copy_size = lz4_ctx_get_copy_size(ctx, control & 0xF);

        /* Copy bytes. */
        const size_t end_offset = ctx->dst_offset + wide_copy_size + 4;
        for (size_t cur_offset = ctx->dst_offset; cur_offset < end_offset; ctx->dst_offset = (++cur_offset)) {
            while (!(wide_offset <= cur_offset));

            ctx->dst[cur_offset] = ctx->dst[cur_offset - wide_offset];
        }
    }
}

void lz4_uncompress(void *dst, size_t dst_size, const void *src, size_t src_size) {
    /* Create and execute a decompressor. */
    lz4_ctx ctx = { src, src_size, 0, dst, dst_size, 0 };
    lz4_ctx_uncompress(&ctx);
}
