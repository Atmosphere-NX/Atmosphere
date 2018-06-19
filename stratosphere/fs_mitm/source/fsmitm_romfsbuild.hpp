#pragma once
#include <switch.h>
#include <variant>

#include "fsmitm_romstorage.hpp"

#define ROMFS_ENTRY_EMPTY 0xFFFFFFFF
#define ROMFS_FILEPARTITION_OFS 0x200

/* Types for RomFS Meta construction. */
enum class RomFSDataSource {
    BaseRomFS,
    FileRomFS,
    LooseFile,
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

class RomFSSourceInfo {
    using InfoVariant = std::variant<RomFSBaseSourceInfo, RomFSFileSourceInfo, RomFSLooseSourceInfo, RomFSMemorySourceInfo>;

    static InfoVariant MakeInfoVariantFromOffset(u64 offset, RomFSDataSource t) {
        switch(t) {
            case RomFSDataSource::BaseRomFS:
                return RomFSBaseSourceInfo { offset };

            case RomFSDataSource::FileRomFS:
                return RomFSFileSourceInfo { offset };

            default:
                fatalSimple(0xF601);
        }
    }

    static InfoVariant MakeInfoVariantFromPointer(const void *arg, RomFSDataSource t) {
        switch(t) {
            case RomFSDataSource::LooseFile:
                return RomFSLooseSourceInfo { (decltype(RomFSLooseSourceInfo::path))arg };

            case RomFSDataSource::Memory:
                return RomFSMemorySourceInfo { (decltype(RomFSMemorySourceInfo::data))arg };

            default:
                fatalSimple(0xF601);
        }
    }

    struct InfoCleanupHelper {
        void operator()(RomFSBaseSourceInfo& info) {
        }

        void operator()(RomFSFileSourceInfo& info) {
        }

        void operator()(RomFSLooseSourceInfo& info) {
            delete info.path;
        }

        void operator()(RomFSMemorySourceInfo& info) {
            delete info.data;
        }
    };

    struct GetTypeHelper {
        RomFSDataSource operator()(const RomFSBaseSourceInfo& info) const {
            return RomFSDataSource::BaseRomFS;
        }

        RomFSDataSource operator()(const RomFSFileSourceInfo& info) const {
            return RomFSDataSource::FileRomFS;
        }

        RomFSDataSource operator()(const RomFSLooseSourceInfo& info) const {
            return RomFSDataSource::LooseFile;
        }

        RomFSDataSource operator()(const RomFSMemorySourceInfo& info) const {
            return RomFSDataSource::Memory;
        }
    };

public:
    u64 virtual_offset;
    u64 size;

    InfoVariant info;

    RomFSSourceInfo(u64 v_o, u64 s, u64 offset, RomFSDataSource t) : virtual_offset(v_o), size(s), info(MakeInfoVariantFromOffset(offset, t)) {
    }

    RomFSSourceInfo(u64 v_o, u64 s, const void *arg, RomFSDataSource t) : virtual_offset(v_o), size(s), info(MakeInfoVariantFromPointer(arg, t)) {
    }

    ~RomFSSourceInfo() {
        std::visit(InfoCleanupHelper{}, info);
    }

    static bool Compare(RomFSSourceInfo *a, RomFSSourceInfo *b) {
        return (a->virtual_offset < b->virtual_offset);
    }

    RomFSDataSource GetType() const {
        return std::visit(GetTypeHelper{}, info);
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

struct RomFSBuildDirectoryContext {
    char path[FS_MAX_PATH];
    u32 cur_path_ofs;
    u32 path_len;
    u32 entry_offset;
    RomFSBuildDirectoryContext *parent;
    RomFSBuildDirectoryContext *child;
    RomFSBuildDirectoryContext *sibling;
    RomFSBuildFileContext *file;
    RomFSBuildDirectoryContext *next;
};

struct RomFSBuildFileContext {
    char path[FS_MAX_PATH];
    u32 cur_path_ofs;
    u32 path_len;
    u32 entry_offset;
    u64 offset;
    u64 size;
    RomFSBuildDirectoryContext *parent;
    RomFSBuildFileContext *sibling;
    RomFSBuildFileContext *next;
    RomFSDataSource source;
    u64 orig_offset;
};

class RomFSBuildContext {
    private:
        u64 title_id;
        RomFSBuildDirectoryContext *root;
        RomFSBuildFileContext *files;
        u64 num_dirs;
        u64 num_files;
        u64 dir_table_size;
        u64 file_table_size;
        u64 dir_hash_table_size;
        u64 file_hash_table_size;
        u64 file_partition_size;
        
        FsDirectoryEntry dir_entry;
        RomFSDataSource cur_source_type;
        
        void VisitDirectory(FsFileSystem *filesys, RomFSBuildDirectoryContext *parent);
        void VisitDirectory(RomFSBuildDirectoryContext *parent, u32 parent_offset, void *dir_table, size_t dir_table_size, void *file_table, size_t file_table_size);
    
        bool AddDirectory(RomFSBuildDirectoryContext *parent_dir_ctx, RomFSBuildDirectoryContext *dir_ctx, RomFSBuildDirectoryContext **out_dir_ctx);
        bool AddFile(RomFSBuildDirectoryContext *parent_dir_ctx, RomFSBuildFileContext *file_ctx);
    public:
        RomFSBuildContext(u64 tid) : title_id(tid), root(NULL), files(NULL), num_dirs(0), num_files(0), dir_table_size(0), file_table_size(0), dir_hash_table_size(0), file_hash_table_size(0), file_partition_size(0) {
            this->root = new RomFSBuildDirectoryContext({0}); 
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
