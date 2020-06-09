/*
 * Copyright (c) 2018 naehrwert
 * Copyright (C) 2018 CTCaer
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

#ifndef _UTIL_H_
#define _UTIL_H_

#include "types.h"
#include "../emuMMC/emummc_ctx.h"

intptr_t QueryIoMapping(u64 addr, u64 size);
#define byte_swap_32(num) (((num >> 24) & 0xff) | ((num << 8) & 0xff0000) | \
						((num >> 8 )& 0xff00) | ((num << 24) & 0xff000000))

typedef struct _cfg_op_t
{
	u32 off;
	u32 val;
} cfg_op_t;

u64 get_tmr_us();
u64 get_tmr_ms();
u64 get_tmr_s();
void usleep(u64 ticks);
void msleep(u64 milliseconds);
void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops);

static inline void *armGetTls(void) {
    void *ret;
    __asm__ __volatile__("MRS %x[data], TPIDRRO_EL0" : [data]"=r"(ret));
    return ret;
}

extern volatile emuMMC_ctx_t emuMMC_ctx;

#endif
