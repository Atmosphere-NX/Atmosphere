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
#include <stratosphere.hpp>
#include "../amsmitm_fs_utils.hpp"
#include "fsmitm_romfs.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace romfs {

        namespace {

            constexpr u32 EmptyEntry = 0xFFFFFFFF;
            constexpr size_t FilePartitionOffset = 0x200;

            struct Header {
                s64 header_size;
                s64 dir_hash_table_ofs;
                s64 dir_hash_table_size;
                s64 dir_table_ofs;
                s64 dir_table_size;
                s64 file_hash_table_ofs;
                s64 file_hash_table_size;
                s64 file_table_ofs;
                s64 file_table_size;
                s64 file_partition_ofs;
            };
            static_assert(std::is_pod<Header>::value && sizeof(Header) == 0x50);

            struct DirectoryEntry {
                u32 parent;
                u32 sibling;
                u32 child;
                u32 file;
                u32 hash;
                u32 name_size;
                char name[];
            };
            static_assert(std::is_pod<DirectoryEntry>::value && sizeof(DirectoryEntry) == 0x18);

            struct FileEntry {
                u32 parent;
                u32 sibling;
                s64 offset;
                s64 size;
                u32 hash;
                u32 name_size;
                char name[];
            };
            static_assert(std::is_pod<FileEntry>::value && sizeof(FileEntry) == 0x20);

            constexpr inline DirectoryEntry *GetDirectoryEntry(void *dir_table, u32 offset) {
                return reinterpret_cast<DirectoryEntry *>(reinterpret_cast<uintptr_t>(dir_table) + offset);
            }

            constexpr inline FileEntry *GetFileEntry(void *file_table, u32 offset) {
                return reinterpret_cast<FileEntry *>(reinterpret_cast<uintptr_t>(file_table) + offset);
            }

            constexpr inline const DirectoryEntry *GetDirectoryEntry(const void *dir_table, u32 offset) {
                return reinterpret_cast<const DirectoryEntry *>(reinterpret_cast<uintptr_t>(dir_table) + offset);
            }

            constexpr inline const FileEntry *GetFileEntry(const void *file_table, u32 offset) {
                return reinterpret_cast<const FileEntry *>(reinterpret_cast<uintptr_t>(file_table) + offset);
            }

            constexpr inline u32 CalculatePathHash(u32 parent, const char *_path, u32 start, size_t path_len) {
                const unsigned char *path = reinterpret_cast<const unsigned char *>(_path);
                u32 hash = parent ^ 123456789;
                for (size_t i = 0; i < path_len; i++) {
                    hash = (hash >> 5) | (hash << 27);
                    hash ^= path[start + i];
                }
                return hash;
            }

            constexpr inline size_t GetHashTableSize(size_t num_entries) {
                if (num_entries < 3) {
                    return 3;
                } else if (num_entries < 19) {
                    return num_entries | 1;
                } else {
                    size_t count = num_entries;
                    while ((count %  2 == 0) ||
                           (count %  3 == 0) ||
                           (count %  5 == 0) ||
                           (count %  7 == 0) ||
                           (count % 11 == 0) ||
                           (count % 13 == 0) ||
                           (count % 17 == 0))
                   {
                       count++;
                   }
                   return count;
                }
            }

            os::Mutex g_fs_romfs_path_lock;
            char g_fs_romfs_path_buffer[fs::EntryNameLengthMax + 1];

            NOINLINE void OpenFileSystemRomfsDirectory(FsDir *out, ncm::ProgramId program_id, BuildDirectoryContext *parent, fs::OpenDirectoryMode mode, FsFileSystem *fs) {
                std::scoped_lock lk(g_fs_romfs_path_lock);
                parent->GetPath(g_fs_romfs_path_buffer);
                R_ABORT_UNLESS(mitm::fs::OpenAtmosphereRomfsDirectory(out, program_id, g_fs_romfs_path_buffer, mode, fs));
            }

        }

        Builder::Builder(ncm::ProgramId pr_id) : program_id(pr_id), num_dirs(0), num_files(0), dir_table_size(0), file_table_size(0), dir_hash_table_size(0), file_hash_table_size(0), file_partition_size(0) {
            auto res = this->directories.emplace(std::make_unique<BuildDirectoryContext>(BuildDirectoryContext::RootTag{}));
            AMS_ABORT_UNLESS(res.second);
            this->root = res.first->get();
            this->num_dirs = 1;
            this->dir_table_size = 0x18;
        }

        void Builder::AddDirectory(BuildDirectoryContext **out, BuildDirectoryContext *parent_ctx, std::unique_ptr<BuildDirectoryContext> child_ctx) {
            /* Set parent context member. */
            child_ctx->parent = parent_ctx;

            /* Check if the directory already exists. */
            auto existing = this->directories.find(child_ctx);
            if (existing != this->directories.end()) {
                *out = existing->get();
                return;
            }

            /* Add a new directory. */
            this->num_dirs++;
            this->dir_table_size += sizeof(DirectoryEntry) + util::AlignUp(child_ctx->path_len, 4);

            *out = child_ctx.get();
            this->directories.emplace(std::move(child_ctx));
        }

        void Builder::AddFile(BuildDirectoryContext *parent_ctx, std::unique_ptr<BuildFileContext> file_ctx) {
            /* Set parent context member. */
            file_ctx->parent = parent_ctx;

            /* Check if the file already exists. */
            if (this->files.find(file_ctx) != this->files.end()) {
                return;
            }

            /* Add a new file. */
            this->num_files++;
            this->file_table_size += sizeof(FileEntry) + util::AlignUp(file_ctx->path_len, 4);
            this->files.emplace(std::move(file_ctx));
        }

        void Builder::VisitDirectory(FsFileSystem *fs, BuildDirectoryContext *parent) {
            FsDir dir;

            /* Get number of child directories. */
            s64 num_child_dirs = 0;
            {
                OpenFileSystemRomfsDirectory(&dir, this->program_id, parent, OpenDirectoryMode_Directory, fs);
                ON_SCOPE_EXIT { fsDirClose(&dir); };
                R_ABORT_UNLESS(fsDirGetEntryCount(&dir, &num_child_dirs));
            }
            AMS_ABORT_UNLESS(num_child_dirs >= 0);

            {
                BuildDirectoryContext **child_dirs = reinterpret_cast<BuildDirectoryContext **>(std::malloc(sizeof(BuildDirectoryContext *) * num_child_dirs));
                ON_SCOPE_EXIT { std::free(child_dirs); };
                AMS_ABORT_UNLESS(child_dirs != nullptr);
                s64 cur_child_dir_ind = 0;

                {
                    OpenFileSystemRomfsDirectory(&dir, this->program_id, parent, OpenDirectoryMode_All, fs);
                    ON_SCOPE_EXIT { fsDirClose(&dir); };

                    s64 read_entries = 0;
                    while (true) {
                        R_ABORT_UNLESS(fsDirRead(&dir, &read_entries, 1, &this->dir_entry));
                        if (read_entries != 1) {
                            break;
                        }

                        AMS_ABORT_UNLESS(this->dir_entry.type == FsDirEntryType_Dir || this->dir_entry.type == FsDirEntryType_File);
                        if (this->dir_entry.type == FsDirEntryType_Dir) {
                            BuildDirectoryContext *real_child = nullptr;
                            this->AddDirectory(&real_child, parent, std::make_unique<BuildDirectoryContext>(this->dir_entry.name, strlen(this->dir_entry.name)));
                            AMS_ABORT_UNLESS(real_child != nullptr);
                            child_dirs[cur_child_dir_ind++] = real_child;
                            AMS_ABORT_UNLESS(cur_child_dir_ind <= num_child_dirs);
                        } else /* if (this->dir_entry.type == FsDirEntryType_File) */ {
                            this->AddFile(parent, std::make_unique<BuildFileContext>(this->dir_entry.name, strlen(this->dir_entry.name), this->dir_entry.file_size, 0, this->cur_source_type));
                        }
                    }
                }

                AMS_ABORT_UNLESS(num_child_dirs == cur_child_dir_ind);
                for (s64 i = 0; i < num_child_dirs; i++) {
                    this->VisitDirectory(fs, child_dirs[i]);
                }
            }

        }

        void Builder::VisitDirectory(BuildDirectoryContext *parent, u32 parent_offset, const void *dir_table, size_t dir_table_size, const void *file_table, size_t file_table_size) {
            const DirectoryEntry *parent_entry = GetDirectoryEntry(dir_table, parent_offset);
            if (parent_entry->file != EmptyEntry) {
                const FileEntry *cur_file = GetFileEntry(file_table, parent_entry->file);
                while (true) {
                    this->AddFile(parent, std::make_unique<BuildFileContext>(cur_file->name, cur_file->name_size, cur_file->size, cur_file->offset, this->cur_source_type));
                    if (cur_file->sibling == EmptyEntry) {
                        break;
                    }
                    cur_file = GetFileEntry(file_table, cur_file->sibling);
                }
            }
            if (parent_entry->child != EmptyEntry) {
                const DirectoryEntry *cur_child = GetDirectoryEntry(dir_table, parent_entry->child);
                u32 cur_child_offset = parent_entry->child;
                while (true) {
                    BuildDirectoryContext *real_child = nullptr;
                    this->AddDirectory(&real_child, parent, std::make_unique<BuildDirectoryContext>(cur_child->name, cur_child->name_size));
                    AMS_ABORT_UNLESS(real_child != nullptr);

                    this->VisitDirectory(real_child, cur_child_offset, dir_table, dir_table_size, file_table, file_table_size);

                    if (cur_child->sibling == EmptyEntry) {
                        break;
                    }
                    cur_child_offset = cur_child->sibling;
                    cur_child = GetDirectoryEntry(dir_table, cur_child_offset);
                }
            }
        }


        void Builder::AddSdFiles() {
            /* Open Sd Card filesystem. */
            FsFileSystem sd_filesystem;
            R_ABORT_UNLESS(fsOpenSdCardFileSystem(&sd_filesystem));
            ON_SCOPE_EXIT { fsFsClose(&sd_filesystem); };

            /* If there is no romfs folder on the SD, don't bother continuing. */
            {
                FsDir dir;
                if (R_FAILED(mitm::fs::OpenAtmosphereRomfsDirectory(&dir, this->program_id, this->root->path.get(), OpenDirectoryMode_Directory, &sd_filesystem))) {
                    return;
                }
                fsDirClose(&dir);
            }

            this->cur_source_type = DataSourceType::LooseSdFile;
            this->VisitDirectory(&sd_filesystem, this->root);
        }

        void Builder::AddStorageFiles(ams::fs::IStorage *storage, DataSourceType source_type) {
            Header header;
            R_ABORT_UNLESS(storage->Read(0, &header, sizeof(Header)));
            AMS_ABORT_UNLESS(header.header_size == sizeof(Header));

            /* Read tables. */
            void *tables = std::malloc(header.dir_table_size + header.file_table_size);
            ON_SCOPE_EXIT { std::free(tables); };
            void *dir_table  = tables;
            void *file_table = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(tables) + header.dir_table_size);
            R_ABORT_UNLESS(storage->Read(header.dir_table_ofs, dir_table, size_t(header.dir_table_size)));
            R_ABORT_UNLESS(storage->Read(header.file_table_ofs, file_table, size_t(header.file_table_size)));

            this->cur_source_type = source_type;
            this->VisitDirectory(this->root, 0x0, dir_table, size_t(header.dir_table_size), file_table, size_t(header.file_table_size));
        }

        void Builder::Build(std::vector<SourceInfo> *out_infos) {
            /* Clear output. */
            out_infos->clear();

            /* Open an SD card filesystem. */
            FsFileSystem sd_filesystem;
            R_ABORT_UNLESS(fsOpenSdCardFileSystem(&sd_filesystem));
            ON_SCOPE_EXIT { fsFsClose(&sd_filesystem); };

            /* Calculate hash table sizes. */
            const size_t num_dir_hash_table_entries  = GetHashTableSize(this->num_dirs);
            const size_t num_file_hash_table_entries = GetHashTableSize(this->num_files);
            this->dir_hash_table_size  = sizeof(u32) * num_dir_hash_table_entries;
            this->file_hash_table_size = sizeof(u32) * num_file_hash_table_entries;

            /* Allocate metadata, make pointers. */
            Header *header = reinterpret_cast<Header *>(std::malloc(sizeof(Header)));
            std::memset(header, 0x00, sizeof(*header));

            /* Open metadata file. */
            const size_t metadata_size = this->dir_hash_table_size + this->dir_table_size + this->file_hash_table_size + this->file_table_size;
            FsFile metadata_file;
            R_ABORT_UNLESS(mitm::fs::CreateAndOpenAtmosphereSdFile(&metadata_file, this->program_id, "romfs_metadata.bin", metadata_size));

            /* Ensure later hash tables will have correct defaults. */
            static_assert(EmptyEntry == 0xFFFFFFFF);

            /* Emplace metadata source info. */
            out_infos->emplace_back(0, sizeof(*header), DataSourceType::Memory, header);

            /* Process Files. */
            {
                u32 entry_offset = 0;
                BuildFileContext *cur_file = nullptr;
                BuildFileContext *prev_file = nullptr;
                for (const auto &it : this->files) {
                    cur_file = it.get();

                    /* By default, pad to 0x10 alignment. */
                    this->file_partition_size = util::AlignUp(this->file_partition_size, 0x10);

                    /* Check if extra padding is present in original source, preserve it to make our life easier. */
                    const bool is_storage_or_file = cur_file->source_type == DataSourceType::Storage || cur_file->source_type == DataSourceType::File;
                    if (prev_file != nullptr && prev_file->source_type == cur_file->source_type && is_storage_or_file) {
                        const s64 expected = this->file_partition_size - prev_file->offset + prev_file->orig_offset;
                        if (expected != cur_file->orig_offset) {
                            AMS_ABORT_UNLESS(expected <= cur_file->orig_offset);
                            this->file_partition_size += cur_file->orig_offset - expected;
                        }
                    }

                    /* Calculate offsets. */
                    cur_file->offset = this->file_partition_size;
                    this->file_partition_size += cur_file->size;
                    cur_file->entry_offset = entry_offset;
                    entry_offset += sizeof(FileEntry) + util::AlignUp(cur_file->path_len, 4);

                    /* Save current file as prev for next iteration. */
                    prev_file = cur_file;
                }
                /* Assign deferred parent/sibling ownership. */
                for (auto it = this->files.rbegin(); it != this->files.rend(); it++) {
                    cur_file = it->get();
                    cur_file->sibling = cur_file->parent->file;
                    cur_file->parent->file = cur_file;
                }
            }

            /* Process Directories. */
            {
                u32 entry_offset = 0;
                BuildDirectoryContext *cur_dir = nullptr;
                for (const auto &it : this->directories) {
                    cur_dir = it.get();
                    cur_dir->entry_offset = entry_offset;
                    entry_offset += sizeof(DirectoryEntry) + util::AlignUp(cur_dir->path_len, 4);
                }
                /* Assign deferred parent/sibling ownership. */
                for (auto it = this->directories.rbegin(); it != this->directories.rend(); it++) {
                    cur_dir = it->get();
                    if (cur_dir == this->root) {
                        continue;
                    }
                    cur_dir->sibling = cur_dir->parent->child;
                    cur_dir->parent->child = cur_dir;
                }
            }

            /* Populate file tables. */
            {
                void *ft_buf = std::malloc(this->file_table_size);
                void *fht_buf = std::malloc(this->file_hash_table_size);
                ON_SCOPE_EXIT { std::free(fht_buf); std::free(ft_buf); };

                u32 *file_hash_table = reinterpret_cast<u32 *>(fht_buf);
                FileEntry *file_table = reinterpret_cast<FileEntry *>(ft_buf);
                std::memset(file_hash_table, 0xFF, this->file_hash_table_size);

                for (const auto &it : this->files) {
                    BuildFileContext *cur_file = it.get();
                    FileEntry *cur_entry = GetFileEntry(file_table, cur_file->entry_offset);

                    /* Set entry fields. */
                    cur_entry->parent = cur_file->parent->entry_offset;
                    cur_entry->sibling = (cur_file->sibling == nullptr) ? EmptyEntry : cur_file->sibling->entry_offset;
                    cur_entry->offset = cur_file->offset;
                    cur_entry->size = cur_file->size;

                    /* Insert into hash table. */
                    const u32 name_size = cur_file->path_len;
                    const size_t hash_ind = CalculatePathHash(cur_entry->parent, cur_file->path.get(), 0, name_size) % num_file_hash_table_entries;
                    cur_entry->hash = file_hash_table[hash_ind];
                    file_hash_table[hash_ind] = cur_file->entry_offset;

                    /* Set name. */
                    cur_entry->name_size = name_size;
                    if (name_size) {
                        std::memcpy(cur_entry->name, cur_file->path.get(), name_size);
                        for (size_t i = name_size; i < util::AlignUp(name_size, 4); i++) {
                            cur_entry->name[i] = 0;
                        }
                    }

                    /* Emplace a source. */
                    switch (cur_file->source_type) {
                        case DataSourceType::Storage:
                        case DataSourceType::File:
                            {
                                /* Try to compact if possible. */
                                auto &back = out_infos->back();
                                if (back.source_type == cur_file->source_type) {
                                    back.size = cur_file->offset + FilePartitionOffset + cur_file->size - back.virtual_offset;
                                } else {
                                    out_infos->emplace_back(cur_file->offset + FilePartitionOffset, cur_file->size, cur_file->source_type, cur_file->orig_offset + FilePartitionOffset);
                                }
                            }
                            break;
                        case DataSourceType::LooseSdFile:
                            {
                                char *new_path = new char[cur_file->GetPathLength() + 1];
                                cur_file->GetPath(new_path);
                                out_infos->emplace_back(cur_file->offset + FilePartitionOffset, cur_file->size, cur_file->source_type, new_path);
                            }
                            break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }

                /* Write to file. */
                R_ABORT_UNLESS(fsFileWrite(&metadata_file, this->dir_hash_table_size + this->dir_table_size, file_hash_table, this->file_hash_table_size, FsWriteOption_None));
                R_ABORT_UNLESS(fsFileWrite(&metadata_file, this->dir_hash_table_size + this->dir_table_size + this->file_hash_table_size, file_table, this->file_table_size, FsWriteOption_None));
            }

            /* Populate directory tables. */
            {
                void *dt_buf = std::malloc(this->dir_table_size);
                void *dht_buf = std::malloc(this->dir_hash_table_size);
                ON_SCOPE_EXIT { std::free(dht_buf); std::free(dt_buf); };

                u32 *dir_hash_table = reinterpret_cast<u32 *>(dht_buf);
                DirectoryEntry *dir_table = reinterpret_cast<DirectoryEntry *>(dt_buf);
                std::memset(dir_hash_table, 0xFF, this->dir_hash_table_size);

                for (const auto &it : this->directories) {
                    BuildDirectoryContext *cur_dir = it.get();
                    DirectoryEntry *cur_entry = GetDirectoryEntry(dir_table, cur_dir->entry_offset);

                    /* Set entry fields. */
                    cur_entry->parent = cur_dir == this->root ? 0 : cur_dir->parent->entry_offset;
                    cur_entry->sibling = (cur_dir->sibling == nullptr) ? EmptyEntry : cur_dir->sibling->entry_offset;
                    cur_entry->child   = (cur_dir->child   == nullptr) ? EmptyEntry : cur_dir->child->entry_offset;
                    cur_entry->file    = (cur_dir->file    == nullptr) ? EmptyEntry : cur_dir->file->entry_offset;

                    /* Insert into hash table. */
                    const u32 name_size = cur_dir->path_len;
                    const size_t hash_ind = CalculatePathHash(cur_entry->parent, cur_dir->path.get(), 0, name_size) % num_dir_hash_table_entries;
                    cur_entry->hash = dir_hash_table[hash_ind];
                    dir_hash_table[hash_ind] = cur_dir->entry_offset;

                    /* Set name. */
                    cur_entry->name_size = name_size;
                    if (name_size) {
                        std::memcpy(cur_entry->name, cur_dir->path.get(), name_size);
                        for (size_t i = name_size; i < util::AlignUp(name_size, 4); i++) {
                            cur_entry->name[i] = 0;
                        }
                    }
                }

                /* Write to file. */
                R_ABORT_UNLESS(fsFileWrite(&metadata_file, 0, dir_hash_table, this->dir_hash_table_size, FsWriteOption_None));
                R_ABORT_UNLESS(fsFileWrite(&metadata_file, this->dir_hash_table_size, dir_table, this->dir_table_size, FsWriteOption_None));
            }

            /* Delete maps. */
            this->root = nullptr;
            this->directories.clear();
            this->files.clear();

            /* Set header fields. */
            header->header_size = sizeof(*header);
            header->file_hash_table_size = this->file_hash_table_size;
            header->file_table_size = this->file_table_size;
            header->dir_hash_table_size = this->dir_hash_table_size;
            header->dir_table_size = this->dir_table_size;
            header->file_partition_ofs = FilePartitionOffset;
            header->dir_hash_table_ofs = util::AlignUp(FilePartitionOffset + this->file_partition_size, 4);
            header->dir_table_ofs = header->dir_hash_table_ofs + header->dir_hash_table_size;
            header->file_hash_table_ofs = header->dir_table_ofs + header->dir_table_size;
            header->file_table_ofs = header->file_hash_table_ofs + header->file_hash_table_size;

            /* Save metadata to the SD card, to save on memory space. */
            {
                R_ABORT_UNLESS(fsFileFlush(&metadata_file));
                out_infos->emplace_back(header->dir_hash_table_ofs, metadata_size, DataSourceType::Metadata, new RemoteFile(metadata_file));
            }
        }

    }

}
