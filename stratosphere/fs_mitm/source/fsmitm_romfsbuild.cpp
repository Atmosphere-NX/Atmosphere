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
 
#include <switch.h>
#include <string.h>
#include <stratosphere.hpp>
#include "fsmitm_utils.hpp"
#include "fsmitm_romfsbuild.hpp"

#include "debug.hpp"

void RomFSBuildContext::VisitDirectory(FsFileSystem *filesys, RomFSBuildDirectoryContext *parent) {
    FsDir dir;
    Result rc;
    
    std::vector<RomFSBuildDirectoryContext *> child_dirs;
    
    /* Open the current parent directory. */
    if (R_FAILED((rc = Utils::OpenRomFSDir(filesys, this->title_id, parent->path, &dir)))) {
        fatalSimple(rc);
    }
    
    u64 read_entries;
    while (R_SUCCEEDED((rc = fsDirRead(&dir, 0, &read_entries, 1, &this->dir_entry))) && read_entries == 1) {
        if (this->dir_entry.type == ENTRYTYPE_DIR) {
            RomFSBuildDirectoryContext *child = new RomFSBuildDirectoryContext({0});
            /* Set child's path. */
            child->cur_path_ofs = parent->path_len + 1;
            child->path_len = child->cur_path_ofs + strlen(this->dir_entry.name);
            child->path = new char[child->path_len + 1];
            strcpy(child->path, parent->path);
            if (child->path_len > FS_MAX_PATH - 1) {
                fatalSimple(0xF601);
            }
            strcat(child->path + parent->path_len, "/");
            strcat(child->path + parent->path_len, this->dir_entry.name);
                                    
            if (!this->AddDirectory(parent, child, NULL)) {
                delete child->path;
                delete child;
            } else {
                child_dirs.push_back(child);
            }
        } else if (this->dir_entry.type == ENTRYTYPE_FILE) {
            RomFSBuildFileContext *child = new RomFSBuildFileContext({0});
            /* Set child's path. */
            child->cur_path_ofs = parent->path_len + 1;
            child->path_len = child->cur_path_ofs + strlen(this->dir_entry.name);
            child->path = new char[child->path_len + 1];
            strcpy(child->path, parent->path);
            if (child->path_len > FS_MAX_PATH - 1) {
                fatalSimple(0xF601);
            }
            strcat(child->path + parent->path_len, "/");
            strcat(child->path + parent->path_len, this->dir_entry.name);
            
            child->source = this->cur_source_type;
            
            child->size = this->dir_entry.fileSize;
                                    
            if (!this->AddFile(parent, child)) {
                delete child->path;
                delete child;
            }
        } else {
            fatalSimple(rc);
        }
    }
    fsDirClose(&dir);
    
    for (auto &child : child_dirs) {
        this->VisitDirectory(filesys, child);
    }
}

void RomFSBuildContext::MergeSdFiles() {
    FsFileSystem sd_filesystem;
    FsDir dir;
    if (!Utils::IsSdInitialized()) {
        return;
    }
    if (R_FAILED((Utils::OpenSdDirForAtmosphere(this->title_id, "/romfs", &dir)))) {
        return;
    }
    fsDirClose(&dir);
    if (R_FAILED(fsMountSdcard(&sd_filesystem))) {
        return;
    }
    this->cur_source_type = RomFSDataSource::LooseFile;
    this->VisitDirectory(&sd_filesystem, this->root);
    fsFsClose(&sd_filesystem);
}

void RomFSBuildContext::VisitDirectory(RomFSBuildDirectoryContext *parent, u32 parent_offset, void *dir_table, size_t dir_table_size, void *file_table, size_t file_table_size) {
    RomFSDirectoryEntry *parent_entry = romfs_get_direntry(dir_table, parent_offset);
    if (parent_entry->file != ROMFS_ENTRY_EMPTY) {
        RomFSFileEntry *cur_file = romfs_get_fentry(file_table, parent_entry->file);
        while (cur_file != NULL) {
            RomFSBuildFileContext *child = new RomFSBuildFileContext({0});
            /* Set child's path. */
            child->cur_path_ofs = parent->path_len + 1;
            child->path_len = child->cur_path_ofs + cur_file->name_size;
            child->path = new char[child->path_len + 1];
            strcpy(child->path, parent->path);
            if (child->path_len > FS_MAX_PATH - 1) {
                fatalSimple(0xF601);
            }
            strcat(child->path + parent->path_len, "/");
            strncat(child->path + parent->path_len, cur_file->name, cur_file->name_size);
            child->size = cur_file->size;
            
            child->source = this->cur_source_type;
            child->orig_offset = cur_file->offset;
            if (!this->AddFile(parent, child)) {
                delete child->path;
                delete child;
            }
            if (cur_file->sibling == ROMFS_ENTRY_EMPTY) {
                cur_file = NULL;
            } else {
                cur_file = romfs_get_fentry(file_table, cur_file->sibling);
            }
        }
    }
    if (parent_entry->child != ROMFS_ENTRY_EMPTY) {
        RomFSDirectoryEntry *cur_child = romfs_get_direntry(dir_table, parent_entry->child);
        u32 cur_child_offset = parent_entry->child;
        while (cur_child != NULL) {
            RomFSBuildDirectoryContext *child = new RomFSBuildDirectoryContext({0});
            /* Set child's path. */
            child->cur_path_ofs = parent->path_len + 1;
            child->path_len = child->cur_path_ofs + cur_child->name_size;
            child->path = new char[child->path_len + 1];
            strcpy(child->path, parent->path);
            if (child->path_len > FS_MAX_PATH - 1) {
                fatalSimple(0xF601);
            }
            strcat(child->path + parent->path_len, "/");
            strncat(child->path + parent->path_len, cur_child->name, cur_child->name_size);
            
            RomFSBuildDirectoryContext *real = NULL;
            if (!this->AddDirectory(parent, child, &real)) {
                delete child->path;
                delete child;
            }
            if (real == NULL) {
                fatalSimple(0xF601);
            }
            
            this->VisitDirectory(real, cur_child_offset, dir_table, dir_table_size, file_table, file_table_size);
            
            if (cur_child->sibling == ROMFS_ENTRY_EMPTY) {
                cur_child = NULL;
            } else {
                cur_child_offset = cur_child->sibling;
                cur_child = romfs_get_direntry(dir_table, cur_child->sibling);
            }
        }
    }
}

void RomFSBuildContext::MergeRomStorage(IROStorage *storage, RomFSDataSource source) {
    Result rc;
    RomFSHeader header;
    if (R_FAILED((rc = storage->Read(&header, sizeof(header), 0)))) {
        fatalSimple(rc);
    }
    if (header.header_size != sizeof(header)) {
        /* what */
        return;
    }
    
    /* Read tables. */
    auto dir_table = std::make_unique<u8[]>(header.dir_table_size);
    auto file_table = std::make_unique<u8[]>(header.file_table_size);
    if (R_FAILED((rc = storage->Read(dir_table.get(), header.dir_table_size, header.dir_table_ofs)))) {
        fatalSimple(rc);
    }
    if (R_FAILED((rc = storage->Read(file_table.get(), header.file_table_size, header.file_table_ofs)))) {
        fatalSimple(rc);
    }
    
    this->cur_source_type = source;
    this->VisitDirectory(this->root, 0x0, dir_table.get(), (size_t)header.dir_table_size, file_table.get(), (size_t)header.file_table_size);
}

bool RomFSBuildContext::AddDirectory(RomFSBuildDirectoryContext *parent_dir_ctx, RomFSBuildDirectoryContext *dir_ctx, RomFSBuildDirectoryContext **out_dir_ctx) {
    /* Check whether it's already in the known directories. */
    auto existing = this->directories.find(dir_ctx->path);
    if (existing != this->directories.end()) {
        if (out_dir_ctx) {
            *out_dir_ctx = existing->second;
        }
        return false;
    }
    
    /* Add a new directory. */
    this->num_dirs++;
    this->dir_table_size += sizeof(RomFSDirectoryEntry) + ((dir_ctx->path_len - dir_ctx->cur_path_ofs + 3) & ~3);
    dir_ctx->parent = parent_dir_ctx;
    this->directories.insert({dir_ctx->path, dir_ctx});
    
    if (out_dir_ctx) {
        *out_dir_ctx = dir_ctx;
    }
    return true;
}

bool RomFSBuildContext::AddFile(RomFSBuildDirectoryContext *parent_dir_ctx, RomFSBuildFileContext *file_ctx) {
    /* Check whether it's already in the known files. */
    auto existing = this->files.find(file_ctx->path);
    if (existing != this->files.end()) {
        return false;
    }
    
    /* Add a new file. */
    this->num_files++;
    this->file_table_size += sizeof(RomFSFileEntry) + ((file_ctx->path_len - file_ctx->cur_path_ofs + 3) & ~3);
    file_ctx->parent = parent_dir_ctx;
    this->files.insert({file_ctx->path, file_ctx});
    
    return true;
}

void RomFSBuildContext::Build(std::vector<RomFSSourceInfo> *out_infos) {
    RomFSBuildFileContext *cur_file;
    RomFSBuildDirectoryContext *cur_dir;
    u32 entry_offset;
    
    u32 dir_hash_table_entry_count  = romfs_get_hash_table_count(this->num_dirs);
    u32 file_hash_table_entry_count = romfs_get_hash_table_count(this->num_files);
    this->dir_hash_table_size = 4 * dir_hash_table_entry_count;
    this->file_hash_table_size = 4 * file_hash_table_entry_count;
    
    /* Assign metadata pointers */
    RomFSHeader *header = new RomFSHeader({0});
    u8 *metadata = new u8[this->dir_hash_table_size + this->dir_table_size + this->file_hash_table_size + this->file_table_size];
    u32 *dir_hash_table = (u32 *)((uintptr_t)metadata);
    RomFSDirectoryEntry *dir_table = (RomFSDirectoryEntry *)((uintptr_t)dir_hash_table + this->dir_hash_table_size);
    u32 *file_hash_table = (u32 *)((uintptr_t)dir_table + this->dir_table_size);
    RomFSFileEntry *file_table = (RomFSFileEntry *)((uintptr_t)file_hash_table + this->file_hash_table_size);
        
    /* Clear out hash tables. */
    for (u32 i = 0; i < dir_hash_table_entry_count; i++) {
        dir_hash_table[i] = ROMFS_ENTRY_EMPTY;
    }
    for (u32 i = 0; i < file_hash_table_entry_count; i++) {
        file_hash_table[i] = ROMFS_ENTRY_EMPTY;
    }
    
    out_infos->clear();
    out_infos->emplace_back(0, sizeof(*header), header, RomFSDataSource::Memory);
        
    /* Determine file offsets. */
    entry_offset = 0;
    RomFSBuildFileContext *prev_file = NULL;
    for (const auto &it : this->files) {
        cur_file = it.second;
        this->file_partition_size = (this->file_partition_size + 0xFULL) & ~0xFULL;
        /* Check for extra padding in the original romfs source and preserve it, to help ourselves later. */
        if (prev_file && prev_file->source == cur_file->source && (prev_file->source == RomFSDataSource::BaseRomFS || prev_file->source == RomFSDataSource::FileRomFS)) {
            u64 expected = (this->file_partition_size - prev_file->offset + prev_file->orig_offset);
            if (expected != cur_file->orig_offset) {
                if (expected > cur_file->orig_offset) {
                    /* This case should NEVER happen. */
                    fatalSimple(0xF601);
                }
                this->file_partition_size += cur_file->orig_offset - expected;
            }
        }
        cur_file->offset = this->file_partition_size;
        this->file_partition_size += cur_file->size;
        cur_file->entry_offset = entry_offset;
        entry_offset += sizeof(RomFSFileEntry) + ((cur_file->path_len - cur_file->cur_path_ofs + 3) & ~3);
        prev_file = cur_file;
    }
    /* Assign deferred parent/sibling ownership. */
    for (auto it = this->files.rbegin(); it != this->files.rend(); it++) {
        cur_file = it->second;
        cur_file->sibling = cur_file->parent->file;
        cur_file->parent->file = cur_file;
    }
    
    /* Determine directory offsets. */
    entry_offset = 0;
    for (const auto &it : this->directories) {
        cur_dir = it.second;
        cur_dir->entry_offset = entry_offset;
        entry_offset += sizeof(RomFSDirectoryEntry) + ((cur_dir->path_len - cur_dir->cur_path_ofs + 3) & ~3);
    }
    /* Assign deferred parent/sibling ownership. */
    for (auto it = this->directories.rbegin(); it->second != this->root; it++) {
        cur_dir = it->second;
        cur_dir->sibling = cur_dir->parent->child;
        cur_dir->parent->child = cur_dir;
    }
    
    
    /* Populate file tables. */
    for (const auto &it : this->files) {
        cur_file = it.second;
        RomFSFileEntry *cur_entry = romfs_get_fentry(file_table, cur_file->entry_offset);

        cur_entry->parent = cur_file->parent->entry_offset;
        cur_entry->sibling = (cur_file->sibling == NULL) ? ROMFS_ENTRY_EMPTY : cur_file->sibling->entry_offset;
        cur_entry->offset = cur_file->offset;
        cur_entry->size = cur_file->size;
        
        u32 name_size = cur_file->path_len - cur_file->cur_path_ofs;
        u32 hash = romfs_calc_path_hash(cur_file->parent->entry_offset, (unsigned char *)cur_file->path + cur_file->cur_path_ofs, 0, name_size);
        cur_entry->hash = file_hash_table[hash % file_hash_table_entry_count];
        file_hash_table[hash % file_hash_table_entry_count] = cur_file->entry_offset;
           
        cur_entry->name_size = name_size;
        memset(cur_entry->name, 0, (cur_entry->name_size + 3) & ~3);
        memcpy(cur_entry->name, cur_file->path + cur_file->cur_path_ofs, name_size);
        
        switch (cur_file->source) {
            case RomFSDataSource::BaseRomFS:
            case RomFSDataSource::FileRomFS:
                /* Try to compact, if possible. */
                if (out_infos->back().type == cur_file->source) {
                    out_infos->back().size = cur_file->offset + ROMFS_FILEPARTITION_OFS + cur_file->size - out_infos->back().virtual_offset;
                } else {
                    out_infos->emplace_back(cur_file->offset + ROMFS_FILEPARTITION_OFS, cur_file->size, cur_file->orig_offset + ROMFS_FILEPARTITION_OFS, cur_file->source);
                }
                break;
            case RomFSDataSource::LooseFile:
                {
                    char *path = new char[cur_file->path_len + 1];
                    strcpy(path, cur_file->path);
                    out_infos->emplace_back(cur_file->offset + ROMFS_FILEPARTITION_OFS, cur_file->size, path, cur_file->source);
                }
                break;
            default:
                fatalSimple(0xF601);
        }
    }
        
    /* Populate dir tables. */
    for (const auto &it : this->directories) {
        cur_dir = it.second;
        RomFSDirectoryEntry *cur_entry = romfs_get_direntry(dir_table, cur_dir->entry_offset);
        cur_entry->parent = cur_dir == this->root ? 0 : cur_dir->parent->entry_offset;
        cur_entry->sibling = (cur_dir->sibling == NULL) ? ROMFS_ENTRY_EMPTY : cur_dir->sibling->entry_offset;
        cur_entry->child = (cur_dir->child == NULL) ? ROMFS_ENTRY_EMPTY : cur_dir->child->entry_offset;
        cur_entry->file = (cur_dir->file == NULL) ? ROMFS_ENTRY_EMPTY : cur_dir->file->entry_offset;
        
        u32 name_size = cur_dir->path_len - cur_dir->cur_path_ofs;
        u32 hash = romfs_calc_path_hash(cur_dir == this->root ? 0 : cur_dir->parent->entry_offset, (unsigned char *)cur_dir->path + cur_dir->cur_path_ofs, 0, name_size);
        cur_entry->hash = dir_hash_table[hash % dir_hash_table_entry_count];
        dir_hash_table[hash % dir_hash_table_entry_count] = cur_dir->entry_offset;
        
        cur_entry->name_size = name_size;
        memset(cur_entry->name, 0, (cur_entry->name_size + 3) & ~3);
        memcpy(cur_entry->name, cur_dir->path + cur_dir->cur_path_ofs, name_size);
        
    }
    
    /* Delete directories. */
    for (const auto &it : this->directories) {
        cur_dir = it.second;
        delete cur_dir->path;
        delete cur_dir;
    }
    this->root = NULL;
    this->directories.clear();
    
    /* Delete files. */
    for (const auto &it : this->files) {
        cur_file = it.second;
        delete cur_file->path;
        delete cur_file;
    }
    this->files.clear();
    
    /* Set header fields. */
    header->header_size = sizeof(*header);
    header->file_hash_table_size = this->file_hash_table_size;
    header->file_table_size = this->file_table_size;
    header->dir_hash_table_size = this->dir_hash_table_size;
    header->dir_table_size = this->dir_table_size;
    header->file_partition_ofs = ROMFS_FILEPARTITION_OFS;
    header->dir_hash_table_ofs = (header->file_partition_ofs + this->file_partition_size + 3ULL) & ~3ULL;
    header->dir_table_ofs = header->dir_hash_table_ofs + header->dir_hash_table_size;
    header->file_hash_table_ofs = header->dir_table_ofs + header->dir_table_size;
    header->file_table_ofs = header->file_hash_table_ofs + header->file_hash_table_size;
    
    const size_t metadata_size = this->dir_hash_table_size + this->dir_table_size + this->file_hash_table_size + this->file_table_size;
    
    /* Try to save metadata to the SD card, to save on memory space. */
    if (R_SUCCEEDED(Utils::SaveSdFileForAtmosphere(this->title_id, ROMFS_METADATA_FILE_PATH, metadata, metadata_size))) {
        out_infos->emplace_back(header->dir_hash_table_ofs, metadata_size, RomFSDataSource::MetaData);
        delete metadata;
    } else {
        out_infos->emplace_back(header->dir_hash_table_ofs, metadata_size, metadata, RomFSDataSource::Memory);
    }
    
}
