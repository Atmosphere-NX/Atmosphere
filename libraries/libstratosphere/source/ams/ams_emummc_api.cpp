/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>

namespace ams::emummc {

    namespace {

        /* Convenience Definitions. */
        constexpr u32 StorageMagic = util::FourCC<'E','F','S','0'>::Code;
        constexpr size_t MaxDirLen = 0x7F;

        /* Types. */
        struct BaseConfig {
            u32 magic;
            u32 type;
            u32 id;
            u32 fs_version;
        };

        struct PartitionConfig {
            u64 start_sector;
        };

        struct FileConfig {
            char path[MaxDirLen + 1];
        };

        struct ExosphereConfig {
            BaseConfig base_cfg;
            union {
                PartitionConfig partition_cfg;
                FileConfig file_cfg;
            };
            char emu_dir_path[MaxDirLen + 1];
        };

        enum Storage : u32 {
            Storage_Emmc,
            Storage_Sd,
            Storage_SdFile,

            Storage_Count,
        };

        /* Globals. */
        constinit os::SdkMutex g_lock;
        constinit ExosphereConfig g_exo_config;
        constinit bool g_is_emummc;
        constinit bool g_has_cached;

        /* Helpers. */
        void CacheValues() {
            std::scoped_lock lk(g_lock);

            if (g_has_cached) {
                return;
            }

            /* Retrieve and cache values. */
            {

                typename std::aligned_storage<2 * (MaxDirLen + 1), os::MemoryPageSize>::type path_storage;

                struct {
                    char file_path[MaxDirLen + 1];
                    char nintendo_path[MaxDirLen + 1];
                } *paths = reinterpret_cast<decltype(paths)>(std::addressof(path_storage));

                /* Retrieve paths from secure monitor. */
                AMS_ABORT_UNLESS(spl::smc::AtmosphereGetEmummcConfig(std::addressof(g_exo_config), paths, 0) == spl::smc::Result::Success);

                const Storage storage = static_cast<Storage>(g_exo_config.base_cfg.type);
                g_is_emummc = g_exo_config.base_cfg.magic == StorageMagic && storage != Storage_Emmc;

                /* Format paths. */
                if (storage == Storage_SdFile) {
                    util::SNPrintf(g_exo_config.file_cfg.path, sizeof(g_exo_config.file_cfg.path), "/%s", paths->file_path);
                }

                util::SNPrintf(g_exo_config.emu_dir_path, sizeof(g_exo_config.emu_dir_path), "/%s", paths->nintendo_path);

                /* If we're emummc, implement default nintendo redirection path. */
                if (g_is_emummc && std::strcmp(g_exo_config.emu_dir_path, "/") == 0) {
                    util::SNPrintf(g_exo_config.emu_dir_path, sizeof(g_exo_config.emu_dir_path), "/emummc/Nintendo_%04x", g_exo_config.base_cfg.id);
                }
            }

            g_has_cached = true;
        }

    }

    /* Get whether emummc is active. */
    bool IsActive() {
        CacheValues();
        return g_is_emummc;
    }

    /* Get the active emummc id. */
    u32 GetActiveId() {
        CacheValues();
        return g_exo_config.base_cfg.id;
    }

    /* Get Nintendo redirection path. */
    const char *GetNintendoDirPath() {
        CacheValues();
        if (!g_is_emummc) {
            return nullptr;
        }
        return g_exo_config.emu_dir_path;
    }

    /* Get Emummc folderpath, NULL if not file-based. */
    const char *GetFilePath() {
        CacheValues();
        if (!g_is_emummc || g_exo_config.base_cfg.type != Storage_SdFile) {
            return nullptr;
        }
        return g_exo_config.file_cfg.path;
    }

}
