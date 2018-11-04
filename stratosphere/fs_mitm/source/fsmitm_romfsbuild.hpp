/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch.h>
#include <map>

#include "fsmitm_romstorage.hpp"

#include "debug.hpp"

#define ROMFS_ENTRY_EMPTY 0xFFFFFFFF
#define ROMFS_FILEPARTITION_OFS 0x200

#define ROMFS_METADATA_FILE_PATH "romfs_metadata.bin"

/* Types for RomFS Meta construction. */
enum class RomFSDataSource {
    BaseRomFS,
    FileRomFS,
    LooseFile,
    MetaData,
    Memory,
};

struct RomFSBaseSourceInfo {
    u64 offset;
};

struct RomFSFileSourceInfo {
    u64 offset;
};

struct RomFSLooseSourceInfo {
    const char *path;
};

struct RomFSMemorySourceInfo {
    const u8 *data;
};

struct RomFSMetaDataSourceInfo {

};

struct RomFSSourceInfo {
    u64 virtual_offset;
    u64 size;
    union {
        RomFSBaseSourceInfo base_source_info;
        RomFSFileSourceInfo file_source_info;
        RomFSLooseSourceInfo loose_source_info;
        RomFSMemorySourceInfo memory_source_info;
        RomFSMemorySourceInfo metadata_source_info;
    };
    RomFSDataSource type;
    
    RomFSSourceInfo(u64 v_o, u64 s, u64 offset, RomFSDataSource t) : virtual_offset(v_o), size(s), type(t) {
        switch (this->type) {
            case RomFSDataSource::BaseRomFS:
                this->base_source_info.offset = offset;
                break;
            case RomFSDataSource::FileRomFS:
                this->file_source_info.offset = offset;
                break;
            case RomFSDataSource::LooseFile:
            case RomFSDataSource::MetaData:
            case RomFSDataSource::Memory:
            default:
                fatalSimple(0xF601);
        }
    }
    
    RomFSSourceInfo(u64 v_o, u64 s, const void *arg, RomFSDataSource t) : virtual_offset(v_o), size(s), type(t) {
        switch (this->type) {
            case RomFSDataSource::LooseFile:
                this->loose_source_info.path = (decltype(this->loose_source_info.path))arg;
                break;
            case RomFSDataSource::Memory:
                this->memory_source_info.data = (decltype(this->memory_source_info.data))arg;
                break;
            case RomFSDataSource::MetaData:
            case RomFSDataSource::BaseRomFS:
            case RomFSDataSource::FileRomFS:
            default:
                fatalSimple(0xF601);
        }
    }
    
    RomFSSourceInfo(u64 v_o, u64 s, RomFSDataSource t) : virtual_offset(v_o), size(s), type(t) {
        switch (this->type) {
            case RomFSDataSource::MetaData:
                break;
            case RomFSDataSource::LooseFile:
            case RomFSDataSource::Memory:
            case RomFSDataSource::BaseRomFS:
            case RomFSDataSource::FileRomFS:
            default:
                fatalSimple(0xF601);
        }
    }
    
    void Cleanup() {
        switch (this->type) {
            case RomFSDataSource::BaseRomFS:
            case RomFSDataSource::FileRomFS:
            case RomFSDataSource::MetaData:
                break;
            case RomFSDataSource::LooseFile:
                delete this->loose_source_info.path;
                break;
            case RomFSDataSource::Memory:
                delete this->memory_source_info.data;
                break;
            default:
                fatalSimple(0xF601);
        }
    }
    
    static bool Compare(RomFSSourceInfo *a, RomFSSourceInfo *b) {
        return (a->virtual_offset < b->virtual_offset);
    }
};

/* Types for building a RomFS. */
struct RomFSHeader {
    u64 header_size;
    u64 dir_hash_table_ofs;
    u64 dir_hash_table_size;
    u64 dir_table_ofs;
    u64 dir_table_size;
    u64 file_hash_table_ofs;
    u64 file_hash_table_size;
    u64 file_table_ofs;
    u64 file_table_size;
    u64 file_partition_ofs;
};

static_assert(sizeof(RomFSHeader) == 0x50, "Incorrect RomFS Header definition!");

struct RomFSDirectoryEntry {
    u32 parent;
    u32 sibling;
    u32 child;
    u32 file;
    u32 hash;
    u32 name_size;
    char name[];
};

static_assert(sizeof(RomFSDirectoryEntry) == 0x18, "Incorrect RomFSDirectoryEntry definition!");

struct RomFSFileEntry {
    u32 parent;
    u32 sibling;
    u64 offset;
    u64 size;
    u32 hash;
    u32 name_size;
    char name[];
};

static_assert(sizeof(RomFSFileEntry) == 0x20, "Incorrect RomFSFileEntry definition!");

struct RomFSBuildFileContext;

/* Used as comparator for std::map<char *, RomFSBuild*Context> */
struct build_ctx_cmp {
    bool operator()(const char *a, const char *b) const {
        return strcmp(a, b) < 0;
    }
};

struct RomFSBuildDirectoryContext {
    char *path;
    u32 cur_path_ofs;
    u32 path_len;
    u32 entry_offset = 0;
    RomFSBuildDirectoryContext *parent = NULL;
    RomFSBuildDirectoryContext *child = NULL;
    RomFSBuildDirectoryContext *sibling = NULL;
    RomFSBuildFileContext *file = NULL;
};

struct RomFSBuildFileContext {
    char *path;
    u32 cur_path_ofs;
    u32 path_len;
    u32 entry_offset = 0;
    u64 offset = 0;
    u64 size = 0;
    RomFSBuildDirectoryContext *parent = NULL;
    RomFSBuildFileContext *sibling = NULL;
    RomFSDataSource source{0};
    u64 orig_offset = 0;
};

class RomFSBuildContext {
    private:
        u64 title_id;
        RomFSBuildDirectoryContext *root;
        std::map<char *, RomFSBuildDirectoryContext *, build_ctx_cmp> directories;
        std::map<char *, RomFSBuildFileContext *, build_ctx_cmp> files;
        u64 num_dirs = 0;
        u64 num_files = 0;
        u64 dir_table_size = 0;
        u64 file_table_size = 0;
        u64 dir_hash_table_size = 0;
        u64 file_hash_table_size = 0;
        u64 file_partition_size = 0;
        
        FsDirectoryEntry dir_entry;
        RomFSDataSource cur_source_type;
        
        void VisitDirectory(FsFileSystem *filesys, RomFSBuildDirectoryContext *parent);
        void VisitDirectory(RomFSBuildDirectoryContext *parent, u32 parent_offset, void *dir_table, size_t dir_table_size, void *file_table, size_t file_table_size);
    
        bool AddDirectory(RomFSBuildDirectoryContext *parent_dir_ctx, RomFSBuildDirectoryContext *dir_ctx, RomFSBuildDirectoryContext **out_dir_ctx);
        bool AddFile(RomFSBuildDirectoryContext *parent_dir_ctx, RomFSBuildFileContext *file_ctx);
    public:
        RomFSBuildContext(u64 tid) : title_id(tid) {
            this->root = new RomFSBuildDirectoryContext({0});
            this->root->path = new char[1];
            this->root->path[0] = '\x00';
            this->directories.insert({this->root->path, this->root});
            this->num_dirs = 1;
            this->dir_table_size = 0x18;
        }
        
        void MergeSdFiles();
        void MergeRomStorage(IROStorage *storage, RomFSDataSource source);
        
        /* This finalizes the context. */
        void Build(std::vector<RomFSSourceInfo> *out_infos);
};


static inline RomFSDirectoryEntry *romfs_get_direntry(void *directories, uint32_t offset) {
    return (RomFSDirectoryEntry *)((uintptr_t)directories + offset);
}

static inline RomFSFileEntry *romfs_get_fentry(void *files, uint32_t offset) {
    return (RomFSFileEntry *)((uintptr_t)files + offset);
}

static inline uint32_t romfs_calc_path_hash(uint32_t parent, const unsigned char *path, uint32_t start, size_t path_len) {
    uint32_t hash = parent ^ 123456789;
    for (uint32_t i = 0; i < path_len; i++) {
        hash = (hash >> 5) | (hash << 27);
        hash ^= path[start + i];
    }
        
    return hash;
}

static inline uint32_t romfs_get_hash_table_count(uint32_t num_entries) {
    if (num_entries < 3) {
        return 3;
    } else if (num_entries < 19) {
        return num_entries | 1;
    }
    uint32_t count = num_entries;
    while (count % 2 == 0 || count % 3 == 0 || count % 5 == 0 || count % 7 == 0 || count % 11 == 0 || count % 13 == 0 || count % 17 == 0) {
        count++;
    }
    return count;
}
