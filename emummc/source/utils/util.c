/*
* Copyright (c) 2018 naehrwert
* Copyright (C) 2018 CTCaer
* Copyright (C) 2019 M4xw
* Copyright (c) 2019 Atmosphere-NX
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

#include "util.h"
#include "fatal.h"
#include "types.h"
#include "../nx/counter.h"
#include "../nx/svc.h"
#include "../soc/t210.h"

typedef struct _io_mapping_t
{
	u64 phys;
	u64 virt;
	u64 size;
} io_mapping_t;
static io_mapping_t io_mapping_list[10] = {0}; // Max 10 Mappings
#define IO_MAPPING_COUNT (sizeof(io_mapping_list) / sizeof(io_mapping_t))

static inline uintptr_t _GetIoMapping(u64 io_addr, u64 io_size)
{
    u64 vaddr;
    u64 aligned_addr = (io_addr & ~0xFFFul);
    u64 aligned_size = io_size + (io_addr - aligned_addr);

    if (emuMMC_ctx.fs_ver >= FS_VER_10_0_0) {
        u64 out_size;
        if (svcQueryIoMapping(&vaddr, &out_size, aligned_addr, aligned_size) != 0) {
            fatal_abort(Fatal_IoMapping);
        }
    } else {
        if (svcLegacyQueryIoMapping(&vaddr, aligned_addr, aligned_size) != 0) {
            fatal_abort(Fatal_IoMappingLegacy);
        }
    }

    return (uintptr_t)(vaddr + (io_addr - aligned_addr));
}

intptr_t QueryIoMapping(u64 addr, u64 size)
{
	for (int i = 0; i < IO_MAPPING_COUNT; i++)
	{
		if (io_mapping_list[i].phys == addr && io_mapping_list[i].size == size)
		{
			return io_mapping_list[i].virt;
		}
	}

	u64 ioMap = _GetIoMapping(addr, size);

	for (int i = 0; i < IO_MAPPING_COUNT; i++)
	{
		if (io_mapping_list[i].phys == 0 && io_mapping_list[i].virt == 0 && io_mapping_list[i].size == 0) // First empty
		{
			io_mapping_list[i].virt = ioMap;
			io_mapping_list[i].phys = addr;
			io_mapping_list[i].size = size;
			break;
		}
	}

	return (intptr_t)ioMap;
}

u64 get_tmr_s()
{
	return armTicksToNs(armGetSystemTick()) / 1e+9;
}

u64 get_tmr_ms()
{
	return armTicksToNs(armGetSystemTick()) / 1000000;
}

u64 get_tmr_us()
{
	return armTicksToNs(armGetSystemTick()) / 1000;
}

// TODO: Figure if Sleep or Busy loop
void msleep(u64 milliseconds)
{
	u64 now = get_tmr_ms();
	while (((u64)get_tmr_ms() - now) < milliseconds)
		;
	//svcSleepThread(1000000 * milliseconds);
}

// TODO: Figure if Sleep or Busy loop
void usleep(u64 microseconds)
{
	u64 now = get_tmr_us();
	while (((u64)get_tmr_us() - now) < microseconds)
		;
	//svcSleepThread(1000 * microseconds);
}

void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops)
{
	for (u32 i = 0; i < num_ops; i++)
		base[ops[i].off] = ops[i].val;
}
