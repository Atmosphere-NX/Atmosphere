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
#include <vapours/prfile2/prfile2_common.hpp>
#include <vapours/prfile2/prfile2_cache.hpp>
#include <vapours/prfile2/prfile2_critical_section.hpp>
#include <vapours/prfile2/prfile2_entry.hpp>
#include <vapours/prfile2/prfile2_fat.hpp>

namespace ams::prfile2 {

    constexpr inline const auto MaxVolumes = 5;

    struct Cursor {
        u64 position;
        u32 sector;
        u32 file_sector_index;
        u16 offset_in_sector;
    };

    struct DirectoryCursor {
        u32 physical_entry_index;
        u32 logical_entry_index;
        u32 logical_seek_index;
    };

    struct File;

    struct FileLock {
        u16 mode;
        u16 count;
        u16 waiting_count;
        File *owner;
        u32 resource;
    };

    struct SystemFileDescriptor {
        u32 status;
        FatFileDescriptor ffd;
        DirectoryEntry dir_entry;
        FileLock lock;
        u16 num_handlers;
        SystemFileDescriptor *next_sfd;
    };

    struct SystemDirectoryDescriptor {
        u32 status;
        u16 num_handlers;
        FatFileDescriptor ffd;
        DirectoryEntry dir_entry;
        /* ... */
        u64 context_id;
        /* ... */
    };

    struct File {
        u32 status;
        /* TODO OpenMode */ u32 open_mode;
        SystemFileDescriptor *sfd;
        FatHint hint;
        LastAccess last_access;
        pf::Error last_error;
        Cursor cursor;
        u16 lock_count;
    };

    struct Directory {
        u32 stat;
        SystemDirectoryDescriptor *sdd;
        FatHint hint;
        DirectoryCursor cursor;
        /* ... */
    };

    struct BiosParameterBlock {
        u16 bytes_per_sector;
        u32 num_reserved_sectors;
        u16 num_root_dir_entries;
        u32 sectors_per_cluster;
        u8 num_fats;
        u32 total_sectors;
        u32 sectors_per_fat;
        u32 root_dir_cluster;
        u16 fs_info_sector;
        u16 backup_boot_sector;
        u16 ext_flags;
        u16 ext_flags_;
        u8 media;
        FatFormat fat_fmt;
        u8 log2_bytes_per_sector;
        u8 log2_sectors_per_cluster;
        u8 num_active_fats;
        u8 num_active_fats_;
        u16 num_root_dir_sectors;
        u32 active_fat_sector;
        u32 active_fat_sector_;
        u32 first_root_dir_sector;
        u32 first_data_sector;
        u32 num_clusters;
        /* ... */
    };

    using VolumeCallback = void (*)();

    struct VolumeContext {
        u64 context_id;
        u32 volume_id;
        DirectoryEntry dir_entries[MaxVolumes];
        pf::Error last_error;
        pf::Error last_driver_error[MaxVolumes];
        pf::Error last_unk_error[MaxVolumes];
        /* ... */
        VolumeContext *next_used_context;
        union {
            VolumeContext *prev_used_context;
            VolumeContext *next_free_context;
        };
    };

    namespace pdm {

        struct Partition;

    }

    struct Volume {
        BiosParameterBlock bpb;
        u32 num_free_clusters;
        u32 num_free_clusters_;
        u32 last_free_cluster;
        u32 last_free_cluster_;
        SystemFileDescriptor sfds[MaximumOpenFileCountSystem];
        File ufds[MaximumOpenFileCountUser];
        SystemDirectoryDescriptor sdds[MaximumOpenDirectoryCountSystem];
        Directory udds[MaximumOpenDirectoryCountUser];
        u32 num_open_files;
        u32 num_open_directories;
        /* ... */
        SectorCache cache;
        VolumeContext *context;
        u64 context_id;
        DirectoryTail tail_entry;
        u32 volume_config;
        u32 file_config;
        u32 flags;
        pf::DriveCharacter drive_char;
        CriticalSection critical_section;
        u16 fsi_flag;
        ClusterLinkForVolume cluster_link;
        pf::Error last_error;
        pf::Error last_driver_error;
        /* ... */
        HandleType partition_handle;
        VolumeCallback callback;
        const u8 *format_param;
        /* ... */
        /* TODO: ExtensionTable extension_table; */
        /* TODO: ExtensionData ext; */
        /* ... */

        template<size_t Ix>
        constexpr ALWAYS_INLINE bool GetFlagsBit() const {
            constexpr u32 Mask = (1u << Ix);
            return (this->flags & Mask) != 0;
        }

        template<size_t Ix>
        constexpr ALWAYS_INLINE void SetFlagsBit(bool en) {
            constexpr u32 Mask = (1u << Ix);
            if (en) {
                this->flags |= Mask;
            } else {
                this->flags &= ~Mask;
            }
        }

        constexpr bool IsAttached() const { return this->GetFlagsBit<0>(); }
        constexpr void SetAttached(bool en) { this->SetFlagsBit<0>(en); }

        constexpr bool IsMounted() const { return this->GetFlagsBit<1>(); }
        constexpr void SetMounted(bool en) { this->SetFlagsBit<1>(en); }

        constexpr bool IsDiskInserted() const { return this->GetFlagsBit<2>(); }
        constexpr void SetDiskInserted(bool en) { this->SetFlagsBit<2>(en); }

        constexpr bool IsFlag12() const { return this->GetFlagsBit<12>(); }
        constexpr void SetFlag12(bool en) { this->SetFlagsBit<12>(en); }
    };

    struct VolumeSet {
        bool initialized;
        u32 num_attached_drives;
        u32 num_mounted_volumes;
        u32 config;
        void *param;
        /* TODO: CodeSet codeset; */
        u32 setting;
        CriticalSection critical_section;
        VolumeContext default_context;
        VolumeContext contexts[MaxVolumes];
        VolumeContext *used_context_head;
        VolumeContext *used_context_tail;
        VolumeContext *free_context_head;
        Volume volumes[MaxVolumes];
    };

}

namespace ams::prfile2::vol {

    pf::Error Initialize(u32 config, void *param);

    pf::Error Attach(pf::DriveTable *drive_table);

    VolumeContext *RegisterContext(u64 *out_context_id);
    pf::Error UnregisterContext();

}
