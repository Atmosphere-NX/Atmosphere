/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>

/* EFS0 */
static constexpr u32 EmummcStorageMagic = 0x30534645;
static constexpr size_t EmummcMaxDirLength = 0x7F;

struct EmummcBaseConfig {
    u32 magic;
    u32 type;
    u32 id;
    u32 fs_version;
};

struct EmummcPartitionConfig {
    u64 start_sector;
};

struct EmummcFileConfig {
    char path[EmummcMaxDirLength+1];
};

struct ExoEmummcConfig {
    EmummcBaseConfig base_cfg;
    union {
        EmummcPartitionConfig partition_cfg;
        EmummcFileConfig file_cfg;
    };
    char emu_dir_path[EmummcMaxDirLength+1];
};

enum EmummcType {
    EmummcType_Emmc = 0,
    EmummcType_Sd,
    EmummcType_SdFile,

    EmummcType_Max,
};

static bool g_IsEmummc = false;
static bool g_HasCached = false;
static Mutex g_Mutex;
static ExoEmummcConfig g_exo_emummc_config;

static void _CacheValues(void)
{
    if (__atomic_load_n(&g_HasCached, __ATOMIC_SEQ_CST))
        return;

    mutexLock(&g_Mutex);

    if (g_HasCached) {
        mutexUnlock(&g_Mutex);
        return;
    }

    static struct {
        char file_path[EmummcMaxDirLength+1];
        char nintendo_path[EmummcMaxDirLength+1];
    } __attribute__((aligned(0x1000))) paths;

    {
        SecmonArgs args = {0};
        args.X[0] = 0xF0000404; /* smcAmsGetEmunandConfig */
        args.X[1] = 0; /* NAND */
        args.X[2] = reinterpret_cast<u64>(&paths); /* path output */
        R_ASSERT(svcCallSecureMonitor(&args));
        if (args.X[0] != 0) {
            std::abort();
        }
        std::memcpy(&g_exo_emummc_config, &args.X[1], sizeof(args) - sizeof(args.X[0]));
    }

    const EmummcType emummc_type = static_cast<EmummcType>(g_exo_emummc_config.base_cfg.type);

/* Ignore format warnings. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    switch (emummc_type) {
        case EmummcType_SdFile:
            std::snprintf(g_exo_emummc_config.file_cfg.path, sizeof(g_exo_emummc_config.file_cfg.path), "/%s", paths.file_path);
            break;
        default:
            break;
    }

    std::snprintf(g_exo_emummc_config.emu_dir_path, sizeof(g_exo_emummc_config.emu_dir_path), "/%s", paths.nintendo_path);

    g_IsEmummc = g_exo_emummc_config.base_cfg.magic == EmummcStorageMagic && emummc_type != EmummcType_Emmc;

    /* Default Nintendo redirection path. */
    if (g_IsEmummc) {
        if (std::strcmp(g_exo_emummc_config.emu_dir_path, "/") == 0) {
            std::snprintf(g_exo_emummc_config.emu_dir_path, sizeof(g_exo_emummc_config.emu_dir_path), "/emummc/Nintendo_%04x", g_exo_emummc_config.base_cfg.id);
        }
    }
#pragma GCC diagnostic pop

    __atomic_store_n(&g_HasCached, true, __ATOMIC_SEQ_CST);

    mutexUnlock(&g_Mutex);
}


/* Get whether emummc is active. */
bool IsEmummc() {
    _CacheValues();
    return g_IsEmummc;
}

/* Get Nintendo redirection path. */
const char *GetEmummcNintendoDirPath() {
    _CacheValues();
    if (!g_IsEmummc) {
        return nullptr;
    }
    return g_exo_emummc_config.emu_dir_path;
}

/* Get Emummc folderpath, NULL if not file-based. */
const char *GetEmummcFilePath() {
    _CacheValues();
    if (!g_IsEmummc || g_exo_emummc_config.base_cfg.type != EmummcType_SdFile) {
        return nullptr;
    }
    return g_exo_emummc_config.file_cfg.path;
}
