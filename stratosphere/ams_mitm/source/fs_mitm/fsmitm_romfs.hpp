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

#pragma once
#include <stratosphere.hpp>

namespace ams::mitm::fs::romfs {

    enum class DataSourceType {
        Storage,
        File,
        LooseSdFile,
        Metadata,
        Memory,
    };

    struct SourceInfo {
        s64 virtual_offset;
        s64 size;
        union {
            struct {
                s64 offset;
            } storage_source_info;
            struct {
                s64 offset;
            } file_source_info;
            struct {
                char *path;
            } loose_source_info;
            struct {
                ams::fs::fsa::IFile *file;
            } metadata_source_info;
            struct {
                u8 *data;
            } memory_source_info;
        };
        DataSourceType source_type;
        bool cleaned_up;

        SourceInfo(s64 v_o, s64 sz, DataSourceType type, s64 p_o) : virtual_offset(v_o), size(sz), source_type(type), cleaned_up(false) {
            switch (this->source_type) {
                case DataSourceType::Storage:
                    this->storage_source_info.offset = p_o;
                    break;
                case DataSourceType::File:
                    this->file_source_info.offset = p_o;
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        SourceInfo(s64 v_o, s64 sz, DataSourceType type, void *arg) : virtual_offset(v_o), size(sz), source_type(type), cleaned_up(false) {
            switch (this->source_type) {
                case DataSourceType::LooseSdFile:
                    this->loose_source_info.path = static_cast<char *>(arg);
                    break;
                case DataSourceType::Metadata:
                    this->metadata_source_info.file = static_cast<ams::fs::fsa::IFile *>(arg);
                    break;
                case DataSourceType::Memory:
                    this->memory_source_info.data = static_cast<u8 *>(arg);
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void Cleanup() {
            AMS_ASSERT(!this->cleaned_up);
            this->cleaned_up = true;

            switch (this->source_type) {
                case DataSourceType::Storage:
                case DataSourceType::File:
                    break;
                case DataSourceType::Metadata:
                    delete this->metadata_source_info.file;
                    break;
                case DataSourceType::LooseSdFile:
                    delete[] this->loose_source_info.path;
                    break;
                case DataSourceType::Memory:
                    std::free(static_cast<void *>(this->memory_source_info.data));
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    };

    constexpr inline bool operator<(const SourceInfo &lhs, const SourceInfo &rhs) {
        return lhs.virtual_offset < rhs.virtual_offset;
    }

    constexpr inline bool operator<(const SourceInfo &lhs, const s64 rhs) {
        return lhs.virtual_offset <= rhs;
    }

    struct BuildFileContext;

    struct BuildDirectoryContext {
        NON_COPYABLE(BuildDirectoryContext);
        NON_MOVEABLE(BuildDirectoryContext);

        std::unique_ptr<char[]> path;
        BuildDirectoryContext *parent;
        BuildDirectoryContext *child;
        BuildDirectoryContext *sibling;
        BuildFileContext *file;
        u32 cur_path_ofs;
        u32 path_len;
        u32 entry_offset;

        struct RootTag{};

        BuildDirectoryContext(RootTag) : parent(nullptr), child(nullptr), sibling(nullptr), file(nullptr), cur_path_ofs(0), path_len(0), entry_offset(0) {
            this->path = std::make_unique<char[]>(1);
        }

        BuildDirectoryContext(const char *parent_path, size_t parent_path_len, const char *entry_name, size_t entry_name_len) : parent(nullptr), child(nullptr), sibling(nullptr), file(nullptr), entry_offset(0) {
            this->cur_path_ofs = parent_path_len + 1;
            this->path_len = this->cur_path_ofs + entry_name_len;
            this->path = std::unique_ptr<char[]>(new char[this->path_len + 1]);
            std::memcpy(this->path.get(), parent_path, parent_path_len);
            this->path[parent_path_len] = '/';
            std::memcpy(this->path.get() + parent_path_len + 1, entry_name, entry_name_len);
            this->path[this->path_len] = 0;
        }
    };

    struct BuildFileContext {
        NON_COPYABLE(BuildFileContext);
        NON_MOVEABLE(BuildFileContext);

        std::unique_ptr<char[]> path;
        BuildDirectoryContext *parent;
        BuildFileContext *sibling;
        s64 offset;
        s64 size;
        s64 orig_offset;
        u32 cur_path_ofs;
        u32 path_len;
        u32 entry_offset;
        DataSourceType source_type;

        BuildFileContext(const char *parent_path, size_t parent_path_len, const char *entry_name, size_t entry_name_len, s64 sz, s64 o_o, DataSourceType type) : parent(nullptr), sibling(nullptr), offset(0), size(sz), orig_offset(o_o), entry_offset(0), source_type(type) {
            this->cur_path_ofs = parent_path_len + 1;
            this->path_len = this->cur_path_ofs + entry_name_len;
            this->path = std::unique_ptr<char[]>(new char[this->path_len + 1]);
            std::memcpy(this->path.get(), parent_path, parent_path_len);
            this->path[parent_path_len] = '/';
            std::memcpy(this->path.get() + parent_path_len + 1, entry_name, entry_name_len);
            this->path[this->path_len] = 0;
        }
    };

    struct Builder {
        NON_COPYABLE(Builder);
        NON_MOVEABLE(Builder);
        private:
            struct PathCompare {
                static constexpr inline int Compare(const char *a, const char *b) {
                    unsigned char c1{}, c2{};
                    while ((c1 = *a++) == (c2 = *b++)) {
                        if (c1 == '\x00') {
                            return 0;
                        }
                    }
                    return (c1 - c2);
                }

                constexpr bool operator()(const char *a, const char *b) const {
                    return PathCompare::Compare(a, b) < 0;
                }
            };

            template<typename T>
            using PathMap = std::map<const char *, std::unique_ptr<T>, PathCompare>;
        private:
            ncm::ProgramId program_id;
            BuildDirectoryContext *root;
            PathMap<BuildDirectoryContext> directories;
            PathMap<BuildFileContext> files;
            size_t num_dirs;
            size_t num_files;
            size_t dir_table_size;
            size_t file_table_size;
            size_t dir_hash_table_size;
            size_t file_hash_table_size;
            size_t file_partition_size;

            ams::fs::DirectoryEntry dir_entry;
            DataSourceType cur_source_type;
        private:
            void VisitDirectory(FsFileSystem *fs, BuildDirectoryContext *parent);
            void VisitDirectory(BuildDirectoryContext *parent, u32 parent_offset, const void *dir_table, size_t dir_table_size, const void *file_table, size_t file_table_size);

            void AddDirectory(BuildDirectoryContext **out, BuildDirectoryContext *parent_ctx, std::unique_ptr<BuildDirectoryContext> file_ctx);
            void AddFile(BuildDirectoryContext *parent_ctx, std::unique_ptr<BuildFileContext> file_ctx);
        public:
            Builder(ncm::ProgramId pr_id);

            void AddSdFiles();
            void AddStorageFiles(ams::fs::IStorage *storage, DataSourceType source_type);

            void Build(std::vector<SourceInfo> *out_infos);
    };

}
