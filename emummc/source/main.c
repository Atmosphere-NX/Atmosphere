/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
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

#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "nx/svc.h"
#include "nx/smc.h"
#include "soc/clock.h"
#include "soc/i2c.h"
#include "emuMMC/emummc.h"
#include "emuMMC/emummc_ctx.h"
#include "FS/FS_offsets.h"
#include "utils/fatal.h"

// Prototypes
void __init();
void __initheap(void);
void setup_hooks(void);
void setup_nintendo_paths(void);
void __libc_init_array(void);
void hook_function(uintptr_t source, uintptr_t target);

void *__stack_top;
uintptr_t text_base;
char inner_heap[INNER_HEAP_SIZE];
size_t inner_heap_size = INNER_HEAP_SIZE;
extern char _start;
extern char __argdata__;

// Nintendo Path
// TODO
static char nintendo_path[0x80] = "Nintendo";

// 1.0.0 requires special path handling because it has separate album and contents paths.
#define FS_100_ALBUM_PATH    0
#define FS_100_CONTENTS_PATH 1
static char nintendo_path_album_100[0x100] = "/Nintendo/Album";
static char nintendo_path_contents_100[0x100] = "/Nintendo/Contents";

// FS offsets
static const fs_offsets_t *fs_offsets;

// Defined by linkerscript
#define INJECTED_SIZE ((uintptr_t)&__argdata__ - (uintptr_t)&_start)
#define INJECT_OFFSET(type, offset) (type)(text_base + INJECTED_SIZE + offset)

#define GENERATE_ADD(register, register_target, value) (0x91000000 | value << 10 | register << 5 | register_target)
#define GENERATE_ADRP(register, page_addr) (0x90000000 | ((((page_addr) >> 12) & 0x3) << 29) | ((((page_addr) >> 12) & 0x1FFFFC) << 3) | ((register) & 0x1F))
#define GENERATE_BRANCH(source, destination) (0x14000000 | ((((destination) - (source)) >> 2) & 0x3FFFFFF))
#define GENERATE_NOP() (0xD503201F)

#define INJECT_HOOK(offset, destination) hook_function(INJECT_OFFSET(uintptr_t, offset), (uintptr_t)&destination)
#define INJECT_HOOK_RELATIVE(offset, relative_destination) hook_function(INJECT_OFFSET(uintptr_t, offset), INJECT_OFFSET(uintptr_t, offset) + relative_destination)
#define INJECT_NOP(offset) write_nop(INJECT_OFFSET(uintptr_t, offset))

// emuMMC
extern _sdmmc_accessor_gc sdmmc_accessor_gc;
extern _sdmmc_accessor_sd sdmmc_accessor_sd;
extern _sdmmc_accessor_nand sdmmc_accessor_nand;
extern _lock_mutex lock_mutex;
extern _unlock_mutex unlock_mutex;
extern void *sd_mutex;
extern void *nand_mutex;
extern volatile int *active_partition;
extern volatile Handle *sdmmc_das_handle;

// Storage
volatile __attribute__((aligned(0x1000))) emuMMC_ctx_t emuMMC_ctx = {
    .magic = EMUMMC_STORAGE_MAGIC,
    .id = 0,
    .fs_ver = FS_VER_MAX,

    // SD Default Metadata
    .SD_Type = emuMMC_SD,
    .SD_StoragePartitionOffset = 0,

    // EMMC Default Metadata
    .EMMC_Type = emuMMC_EMMC,
    .EMMC_StoragePartitionOffset = 0,

    // File Default Path
    .storagePath = "",
};

// TODO: move into another file
typedef struct
{
    void *_0x0;
    void *_0x8;
    void *_0x10;
    void *_0x18;
    void *_0x20;
    void *_0x28;
    void *_0x30;
    void *_0x38;
    void *_0x40;
    Result (*set_min_v_clock_rate)(void *, uint32_t, uint32_t);
} nn_clkrst_session_vt_t;

typedef struct
{
    nn_clkrst_session_vt_t *vt;
} nn_clkrst_session_t;

Result clkrst_set_min_v_clock_rate(nn_clkrst_session_t **_this, uint32_t clk_rate)
{
    Result rc = (*_this)->vt->set_min_v_clock_rate((void *)*_this, clk_rate, clk_rate);

    if (rc == 0x6C0 || rc == 0)
    { // TODO #define
        return 0;
    }
    if (rc != 0xAC0)
    { // TODO #define
        fatal_abort(Fatal_BadResult);
    }

    return rc;
}

void __initheap(void)
{
    void *addr = inner_heap;
    size_t size = inner_heap_size;

    /* Newlib Heap Management */
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = (char *)addr;
    fake_heap_end = (char *)addr + size;
}

void hook_function(uintptr_t source, uintptr_t target)
{
    u32 branch_opcode = GENERATE_BRANCH(source, target);
    smcWriteAddress32((void *)source, branch_opcode);
}

void write_nop(uintptr_t source)
{
    smcWriteAddress32((void *)source, GENERATE_NOP());
}

void write_adrp_add(int reg, uintptr_t pc, uintptr_t add_rel_offset, intptr_t destination)
{
    uintptr_t add_opcode_location = pc + add_rel_offset;

    intptr_t offset = (destination & 0xFFFFF000) - (pc & 0xFFFFF000);
    uint32_t opcode_adrp = GENERATE_ADRP(reg, offset);
    uint32_t opcode_add = GENERATE_ADD(reg, reg, (destination & 0x00000FFF));

    smcWriteAddress32((void *)pc, opcode_adrp);
    smcWriteAddress32((void *)add_opcode_location, opcode_add);
}

void setup_hooks(void)
{
    // rtld
    INJECT_HOOK_RELATIVE(fs_offsets->rtld, fs_offsets->rtld_destination);
    // sdmmc_wrapper_read hook
    INJECT_HOOK(fs_offsets->sdmmc_wrapper_read, sdmmc_wrapper_read);
    // sdmmc_wrapper_write hook
    INJECT_HOOK(fs_offsets->sdmmc_wrapper_write, sdmmc_wrapper_write);
    // sdmmc_wrapper_controller_close hook
    INJECT_HOOK(fs_offsets->sdmmc_accessor_controller_close, sdmmc_wrapper_controller_close);

    // On 8.0.0+, we need to hook the regulator setup, because
    // otherwise it will abort because we have already turned it on.
    if (emuMMC_ctx.fs_ver >= FS_VER_8_0_0)
    {
        INJECT_HOOK(fs_offsets->clkrst_set_min_v_clock_rate, clkrst_set_min_v_clock_rate);
    }
}

void populate_function_pointers(void)
{
    // Accessor getters
    sdmmc_accessor_gc = INJECT_OFFSET(_sdmmc_accessor_gc, fs_offsets->sdmmc_accessor_gc);
    sdmmc_accessor_sd = INJECT_OFFSET(_sdmmc_accessor_sd, fs_offsets->sdmmc_accessor_sd);
    sdmmc_accessor_nand = INJECT_OFFSET(_sdmmc_accessor_nand, fs_offsets->sdmmc_accessor_nand);

    // MutexLock functions
    lock_mutex = INJECT_OFFSET(_lock_mutex, fs_offsets->lock_mutex);
    unlock_mutex = INJECT_OFFSET(_unlock_mutex, fs_offsets->unlock_mutex);

    // Other
    sd_mutex = INJECT_OFFSET(void *, fs_offsets->sd_mutex);
    nand_mutex = INJECT_OFFSET(void *, fs_offsets->nand_mutex);
    active_partition = INJECT_OFFSET(volatile int *, fs_offsets->active_partition);
    sdmmc_das_handle = INJECT_OFFSET(volatile Handle *, fs_offsets->sdmmc_das_handle);
}

void write_nops(void)
{
    // On 7.0.0+, we need to attach to device address space ourselves.
    // This patches an abort that happens when Nintendo's code sees SD
    // is already attached
    if (emuMMC_ctx.fs_ver >= FS_VER_7_0_0)
    {
        INJECT_NOP(fs_offsets->sd_das_init);
    }
}

static void load_emummc_ctx(void)
{
    exo_emummc_config_t config;
    static struct
    {
        char storage_path[sizeof(emuMMC_ctx.storagePath)];
        char nintendo_path[sizeof(nintendo_path)];
    } __attribute__((aligned(0x1000))) paths;

    int x = smcGetEmummcConfig(EXO_EMUMMC_MMC_NAND, &config, &paths);
    if (x != 0)
    {
        fatal_abort(Fatal_GetConfig);
    }

    if (config.base_cfg.magic == EMUMMC_STORAGE_MAGIC)
    {
        emuMMC_ctx.magic = config.base_cfg.magic;
        emuMMC_ctx.id = config.base_cfg.id;
        emuMMC_ctx.EMMC_Type = (enum emuMMC_Type)config.base_cfg.type;
        emuMMC_ctx.fs_ver = (enum FS_VER)config.base_cfg.fs_version;
        if (emuMMC_ctx.EMMC_Type == emuMMC_SD)
        {
            emuMMC_ctx.EMMC_StoragePartitionOffset = config.partition_cfg.start_sector;
        }
        else if (emuMMC_ctx.EMMC_Type == emuMMC_SD_File)
        {
            memcpy((void *)emuMMC_ctx.storagePath, paths.storage_path, sizeof(emuMMC_ctx.storagePath) - 1);
            emuMMC_ctx.storagePath[sizeof(emuMMC_ctx.storagePath) - 1] = 0;
        }
        memcpy(nintendo_path, paths.nintendo_path, sizeof(nintendo_path) - 1);
        nintendo_path[sizeof(nintendo_path) - 1] = 0;
        if (strcmp(nintendo_path, "") == 0)
        {
            snprintf(nintendo_path, sizeof(nintendo_path), "emummc/Nintendo_%04x", emuMMC_ctx.id);
        }
    }
    else
    {
        fatal_abort(Fatal_GetConfig);
    }
}

void setup_nintendo_paths(void)
{
    if (emuMMC_ctx.fs_ver > FS_VER_1_0_0)
    {
        for (int i = 0; fs_offsets->nintendo_paths[i].adrp_offset; i++)
        {
            intptr_t nintendo_path_location = (intptr_t)&nintendo_path;
            uintptr_t fs_adrp_opcode_location = INJECT_OFFSET(uintptr_t, fs_offsets->nintendo_paths[i].adrp_offset);
            write_adrp_add(fs_offsets->nintendo_paths[i].opcode_reg, fs_adrp_opcode_location, fs_offsets->nintendo_paths[i].add_rel_offset, nintendo_path_location);
        }
    }
    else
    {
        // 1.0.0 needs special handling because it uses two paths.
        // Do album path
        {
            snprintf(nintendo_path_album_100, sizeof(nintendo_path_album_100), "/%s/Album", nintendo_path);
            intptr_t nintendo_album_path_location = (intptr_t)&nintendo_path_album_100;
            uintptr_t fs_adrp_opcode_location = INJECT_OFFSET(uintptr_t, fs_offsets->nintendo_paths[FS_100_ALBUM_PATH].adrp_offset);
            write_adrp_add(fs_offsets->nintendo_paths[FS_100_ALBUM_PATH].opcode_reg, fs_adrp_opcode_location, fs_offsets->nintendo_paths[FS_100_ALBUM_PATH].add_rel_offset, nintendo_album_path_location);
        }

        // Do contents path
        {
            snprintf(nintendo_path_contents_100, sizeof(nintendo_path_contents_100), "/%s/Contents", nintendo_path);
            intptr_t nintendo_contents_path_location = (intptr_t)&nintendo_path_contents_100;
            uintptr_t fs_adrp_opcode_location = INJECT_OFFSET(uintptr_t, fs_offsets->nintendo_paths[FS_100_CONTENTS_PATH].adrp_offset);
            write_adrp_add(fs_offsets->nintendo_paths[FS_100_CONTENTS_PATH].opcode_reg, fs_adrp_opcode_location, fs_offsets->nintendo_paths[FS_100_CONTENTS_PATH].add_rel_offset, nintendo_contents_path_location);
        }
    }
}

// inject main func
void __init()
{
    // Call constructors.
    __libc_init_array();

    MemoryInfo meminfo;
    u32 pageinfo;
    svcQueryMemory(&meminfo, &pageinfo, (u64)&_start);

    text_base = meminfo.addr;

    load_emummc_ctx();

    fs_offsets = get_fs_offsets(emuMMC_ctx.fs_ver);

    setup_hooks();
    populate_function_pointers();
    write_nops();
    setup_nintendo_paths();

    clock_enable_i2c5();
    i2c_init();
}
