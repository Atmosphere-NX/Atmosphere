/*
 * Copyright (c) 2019 Adubbz
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

#include "ncm_contentstorage.hpp"
#include "ncm_fs.hpp"
#include "ncm_make_path.hpp"
#include "ncm_utils.hpp"

namespace ams::ncm {

    ContentStorageInterface::~ContentStorageInterface() {
        this->Finalize();
    }

    Result ContentStorageInterface::Initialize(const char* root_path, MakeContentPathFunc content_path_func, MakePlaceHolderPathFunc placeholder_path_func, bool delay_flush, impl::RightsIdCache* rights_id_cache) {
        R_TRY(this->EnsureEnabled());
        R_TRY(fs::CheckContentStorageDirectoriesExist(root_path));
        const size_t root_path_len = strnlen(root_path, FS_MAX_PATH-1);

        if (root_path_len >= FS_MAX_PATH-1) {
            std::abort();
        }

        strncpy(this->root_path, root_path, FS_MAX_PATH-2);
        this->make_content_path_func = *content_path_func;
        this->placeholder_accessor.Initialize(this->root_path, *placeholder_path_func, delay_flush);
        this->rights_id_cache = rights_id_cache;
        return ResultSuccess();
    }

    void ContentStorageInterface::Finalize() {
        this->ClearContentCache();
        this->placeholder_accessor.InvalidateAll();
    }

    void ContentStorageInterface::ClearContentCache() {
        if (this->cached_content_id != InvalidContentId) {
            fclose(this->content_cache_file_handle);
            this->cached_content_id = InvalidContentId;
        }
    }

    unsigned int ContentStorageInterface::GetContentDirectoryDepth() {
        if (this->make_content_path_func == static_cast<MakeContentPathFunc>(path::MakeContentPathFlat)) {
            return 1;
        } else if (this->make_content_path_func == static_cast<MakeContentPathFunc>(path::MakeContentPathHashByteLayered)) {
            return 2;
        } else if (this->make_content_path_func == static_cast<MakeContentPathFunc>(path::MakeContentPath10BitLayered)) {
            return 2;
        } else if (this->make_content_path_func == static_cast<MakeContentPathFunc>(path::MakeContentPathDualLayered)) {
            return 3;
        }

        std::abort();
    }

    Result ContentStorageInterface::OpenCachedContentFile(ContentId content_id) {
        R_UNLESS(this->cached_content_id != content_id, ResultSuccess());

        this->ClearContentCache();
        char content_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);

        R_TRY_CATCH(fs::OpenFile(&this->content_cache_file_handle, content_path, FsOpenMode_Read)) {
            R_CONVERT(ams::fs::ResultPathNotFound, ncm::ResultContentNotFound())
        } R_END_TRY_CATCH;

        this->cached_content_id = content_id;
        return ResultSuccess();
    }

    Result ContentStorageInterface::GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
        R_TRY(this->EnsureEnabled());

        ams::os::GenerateRandomBytes(out.GetPointer(), sizeof(PlaceHolderId));
        char placeholder_str[FS_MAX_PATH] = {0};
        GetStringFromPlaceHolderId(placeholder_str, *out.GetPointer());
        return ResultSuccess();
    }

    Result ContentStorageInterface::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) {
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);

        R_TRY(fs::EnsureParentDirectoryRecursively(content_path));
        R_TRY(this->placeholder_accessor.Create(placeholder_id, size));

        return ResultSuccess();
    }

    Result ContentStorageInterface::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());
        return this->placeholder_accessor.Delete(placeholder_id);
    }

    Result ContentStorageInterface::HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        char placeholder_path[FS_MAX_PATH] = {0};
        this->placeholder_accessor.MakePath(placeholder_path, placeholder_id);
        
        bool has = false;
        R_TRY(fs::HasFile(&has, placeholder_path));
        out.SetValue(has);

        return ResultSuccess();
    }

    Result ContentStorageInterface::WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, sf::InBuffer data) {
        /* Offset is too large */
        R_UNLESS(offset<= std::numeric_limits<s64>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());
        R_TRY(this->placeholder_accessor.Write(placeholder_id, offset, data.GetPointer(), data.GetSize()));
        return ResultSuccess();
    }

    Result ContentStorageInterface::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        this->ClearContentCache();
        R_TRY(this->EnsureEnabled());

        char placeholder_path[FS_MAX_PATH] = {0};
        char content_path[FS_MAX_PATH] = {0};

        this->placeholder_accessor.GetPath(placeholder_path, placeholder_id);
        this->GetContentPath(content_path, content_id);

        if (rename(placeholder_path, content_path) != 0) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CONVERT(ams::fs::ResultPathNotFound, ncm::ResultPlaceHolderNotFound())
                R_CONVERT(ams::fs::ResultPathAlreadyExists, ncm::ResultContentAlreadyExists())
            } R_END_TRY_CATCH;
        }

        return ResultSuccess();
    }

    Result ContentStorageInterface::Delete(ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        this->ClearContentCache();
        char content_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);

        if (std::remove(content_path) != 0) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CONVERT(ams::fs::ResultPathNotFound, ncm::ResultContentNotFound())
            } R_END_TRY_CATCH;
        }

        return ResultSuccess();
    }

    Result ContentStorageInterface::Has(sf::Out<bool> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);

        bool has = false;
        R_TRY(fs::HasFile(&has, content_path));
        out.SetValue(has);

        return ResultSuccess();
    }

    Result ContentStorageInterface::GetPath(sf::Out<lr::Path> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);
        R_TRY(fs::ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, content_path));
        out.SetValue(lr::Path::Encode(common_path));
        return ResultSuccess();
    }

    Result ContentStorageInterface::GetPlaceHolderPath(sf::Out<lr::Path> out, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        char placeholder_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        this->placeholder_accessor.GetPath(placeholder_path, placeholder_id);
        R_TRY(fs::ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, placeholder_path));
        out.SetValue(lr::Path::Encode(common_path));
        return ResultSuccess();
    }

    Result ContentStorageInterface::CleanupAllPlaceHolder() {
        R_TRY(this->EnsureEnabled());

        char placeholder_root_path[FS_MAX_PATH] = {0};
        this->placeholder_accessor.InvalidateAll();
        this->placeholder_accessor.MakeRootPath(placeholder_root_path);

        /* Nintendo uses CleanDirectoryRecursively which is 3.0.0+. 
           We'll just delete the directory and recreate it to support all firmwares. */
        R_TRY(fsdevDeleteDirectoryRecursively(placeholder_root_path));
        R_UNLESS(mkdir(placeholder_root_path, S_IRWXU) != -1, fsdevGetLastResult());
        return ResultSuccess();
    }

    Result ContentStorageInterface::ListPlaceHolder(sf::Out<u32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
        R_TRY(this->EnsureEnabled());

        char placeholder_root_path[FS_MAX_PATH] = {0};
        this->placeholder_accessor.MakeRootPath(placeholder_root_path);
        const unsigned int dir_depth = this->placeholder_accessor.GetDirectoryDepth();
        size_t entry_count = 0;

        R_TRY(fs::TraverseDirectory(placeholder_root_path, dir_depth, [&](bool* should_continue, bool* should_retry_dir_read, const char* current_path, struct dirent* dir_entry) -> Result {
            *should_continue = true;
            *should_retry_dir_read = false;
            
            if (dir_entry->d_type == DT_REG) {
                R_UNLESS(entry_count <= out_buf.GetSize(), ncm::ResultBufferInsufficient());
                
                PlaceHolderId cur_entry_placeholder_id = {0};
                R_TRY(GetPlaceHolderIdFromDirEntry(&cur_entry_placeholder_id, dir_entry));
                out_buf[entry_count++] = cur_entry_placeholder_id;
            }
            
            return ResultSuccess();
        }));

        out_count.SetValue(static_cast<u32>(entry_count));
        return ResultSuccess();
    }

    Result ContentStorageInterface::GetContentCount(sf::Out<u32> out_count) {
        R_TRY(this->EnsureEnabled());

        char content_root_path[FS_MAX_PATH] = {0};
        this->GetContentRootPath(content_root_path);
        const unsigned int dir_depth = this->GetContentDirectoryDepth();
        u32 content_count = 0;

        R_TRY(fs::TraverseDirectory(content_root_path, dir_depth, [&](bool* should_continue, bool* should_retry_dir_read, const char* current_path, struct dirent* dir_entry) -> Result {
            *should_continue = true;
            *should_retry_dir_read = false;

            if (dir_entry->d_type == DT_REG) {
                content_count++;
            }

            return ResultSuccess();
        }));

        out_count.SetValue(content_count);
        return ResultSuccess();
    }

    Result ContentStorageInterface::ListContentId(sf::Out<u32> out_count, const sf::OutArray<ContentId> &out_buf, u32 start_offset) {
        R_UNLESS(start_offset <= std::numeric_limits<s32>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        char content_root_path[FS_MAX_PATH] = {0};
        this->GetContentRootPath(content_root_path);
        const unsigned int dir_depth = this->GetContentDirectoryDepth();
        size_t entry_count = 0;

        R_TRY(fs::TraverseDirectory(content_root_path, dir_depth, [&](bool* should_continue,  bool* should_retry_dir_read, const char* current_path, struct dirent* dir_entry) {
            *should_retry_dir_read = false;
            *should_continue = true;

            if (dir_entry->d_type == DT_REG) {
                /* Skip entries until we reach the start offset. */
                if (start_offset > 0) {
                    start_offset--;
                    return ResultSuccess();
                }

                /* We don't necessarily expect to be able to completely fill the output buffer. */
                if (entry_count > out_buf.GetSize()) {
                    *should_continue = false;
                    return ResultSuccess();
                }

                size_t name_len = strlen(dir_entry->d_name);
                std::optional<ContentId> content_id = GetContentIdFromString(dir_entry->d_name, name_len);

                /* Skip to the next entry if the id was invalid. */
                if (!content_id) {
                    return ResultSuccess();
                }

                out_buf[entry_count++] = *content_id;
            }

            return ResultSuccess();
        }));

        for (size_t i = 0; i < entry_count; i++) {
            char content_name[sizeof(ContentId)*2+1] = {0};
            GetStringFromContentId(content_name, out_buf[i]);
        }

        out_count.SetValue(static_cast<u32>(entry_count));
        return ResultSuccess();
    }

    Result ContentStorageInterface::GetSizeFromContentId(sf::Out<u64> out_size, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);
        struct stat st;

        R_UNLESS(stat(content_path, &st) != -1, fsdevGetLastResult());
        out_size.SetValue(st.st_size);
        return ResultSuccess();
    }

    Result ContentStorageInterface::DisableForcibly() {
        this->disabled = true;
        this->ClearContentCache();
        this->placeholder_accessor.InvalidateAll();
        return ResultSuccess();
    }

    Result ContentStorageInterface::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        R_TRY(this->EnsureEnabled());
        
        char old_content_path[FS_MAX_PATH] = {0};
        char new_content_path[FS_MAX_PATH] = {0};
        char placeholder_path[FS_MAX_PATH] = {0};

        this->ClearContentCache();

        /* Ensure the new content path is ready. */
        this->GetContentPath(new_content_path, new_content_id);
        R_TRY(fs::EnsureParentDirectoryRecursively(new_content_path));

        R_TRY(this->placeholder_accessor.EnsureRecursively(placeholder_id));
        this->placeholder_accessor.GetPath(placeholder_path, placeholder_id);
        if (rename(old_content_path, placeholder_path) != 0) {
            R_TRY_CATCH(fsdevGetLastResult()) {
                R_CONVERT(ams::fs::ResultPathNotFound, ncm::ResultPlaceHolderNotFound())
                R_CONVERT(ams::fs::ResultPathAlreadyExists, ncm::ResultPlaceHolderAlreadyExists())
            } R_END_TRY_CATCH;
        }

        return ResultSuccess();
    }

    Result ContentStorageInterface::SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) {
        R_TRY(this->EnsureEnabled());
        R_TRY(this->placeholder_accessor.SetSize(placeholder_id, size));
        return ResultSuccess();
    }

    Result ContentStorageInterface::ReadContentIdFile(sf::OutBuffer buf, ContentId content_id, u64 offset) {
        /* Offset is too large */
        R_UNLESS(offset<= std::numeric_limits<s64>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());
        char content_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);
        R_TRY(this->OpenCachedContentFile(content_id));
        R_TRY(fs::ReadFile(this->content_cache_file_handle, offset, buf.GetPointer(), buf.GetSize()));

        return ResultSuccess();
    }

    Result ContentStorageInterface::GetRightsIdFromPlaceHolderId(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        FsRightsId rights_id = {0};
        u8 key_generation = 0;

        char placeholder_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        this->placeholder_accessor.GetPath(placeholder_path, placeholder_id);
        R_TRY(fs::ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, placeholder_path));
        R_TRY(fsGetRightsIdAndKeyGenerationByPath(common_path, &key_generation, &rights_id));

        out_rights_id.SetValue(rights_id);
        out_key_generation.SetValue(static_cast<u64>(key_generation));

        return ResultSuccess();
    }

    Result ContentStorageInterface::GetRightsIdFromContentId(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, ContentId content_id) {
        R_TRY(this->EnsureEnabled());
        
        {
            std::scoped_lock lk(this->rights_id_cache->mutex);

            /* Attempt to locate the content id in the cache. */
            for (size_t i = 0; i < impl::RightsIdCache::MaxEntries; i++) {
                impl::RightsIdCache::Entry* entry = &this->rights_id_cache->entries[i];

                if (entry->last_accessed != 1 && content_id == entry->uuid) {
                    entry->last_accessed = this->rights_id_cache->counter;
                    this->rights_id_cache->counter++;
                    out_rights_id.SetValue(entry->rights_id);
                    out_key_generation.SetValue(entry->key_generation);
                    return ResultSuccess();
                }
            }
        }

        FsRightsId rights_id = {0};
        u8 key_generation = 0;
        char content_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);
        R_TRY(fs::ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, content_path));
        R_TRY(fsGetRightsIdAndKeyGenerationByPath(common_path, &key_generation, &rights_id));

        {
            std::scoped_lock lk(this->rights_id_cache->mutex);
            impl::RightsIdCache::Entry* eviction_candidate = &this->rights_id_cache->entries[0];

            /* Find a suitable existing entry to store our new one at. */
            for (size_t i = 1; i < impl::RightsIdCache::MaxEntries; i++) {
                impl::RightsIdCache::Entry* entry = &this->rights_id_cache->entries[i];

                /* Change eviction candidates if the uuid already matches ours, or if the uuid doesn't already match and the last_accessed count is lower */
                if (content_id == entry->uuid || (content_id != eviction_candidate->uuid && entry->last_accessed < eviction_candidate->last_accessed)) {
                    eviction_candidate = entry;
                }
            }

            /* Update the cache. */
            eviction_candidate->uuid = content_id.uuid;
            eviction_candidate->rights_id = rights_id;
            eviction_candidate->key_generation = key_generation;
            eviction_candidate->last_accessed = this->rights_id_cache->counter;
            this->rights_id_cache->counter++;

            /* Set output. */
            out_rights_id.SetValue(rights_id);
            out_key_generation.SetValue(key_generation);
        }

        return ResultSuccess();
    }

    Result ContentStorageInterface::WriteContentForDebug(ContentId content_id, u64 offset, sf::InBuffer data) {
        /* Offset is too large */
        R_UNLESS(offset<= std::numeric_limits<s64>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        bool is_development = false;

        if (R_FAILED(splIsDevelopment(&is_development)) || !is_development) {
            std::abort();
        }

        this->ClearContentCache();

        char content_path[FS_MAX_PATH] = {0};
        this->GetContentPath(content_path, content_id);

        FILE* f = nullptr;
        R_TRY(fs::OpenFile(&f, content_path, FsOpenMode_Write));
        
        ON_SCOPE_EXIT {
            fclose(f);
        };

        R_TRY(fs::WriteFile(f, offset, data.GetPointer(), data.GetSize(), FsWriteOption_Flush));

        return ResultSuccess();
    }

    Result ContentStorageInterface::GetFreeSpaceSize(sf::Out<u64> out_size) {
        struct statvfs st = {0};
        R_UNLESS(statvfs(this->root_path, &st) != -1, fsdevGetLastResult());
        out_size.SetValue(st.f_bfree);
        return ResultSuccess();
    }

    Result ContentStorageInterface::GetTotalSpaceSize(sf::Out<u64> out_size) {
        struct statvfs st = {0};
        R_UNLESS(statvfs(this->root_path, &st) != -1, fsdevGetLastResult());
        out_size.SetValue(st.f_blocks);
        return ResultSuccess();
    }

    Result ContentStorageInterface::FlushPlaceHolder() {
        this->placeholder_accessor.InvalidateAll();
        return ResultSuccess();
    }

    Result ContentStorageInterface::GetSizeFromPlaceHolderId(sf::Out<u64> out_size, PlaceHolderId placeholder_id) {
        R_TRY(this->EnsureEnabled());

        bool found_in_cache = false;
        size_t size = 0;

        R_TRY(this->placeholder_accessor.GetSize(&found_in_cache, &size, placeholder_id));

        if (found_in_cache) {
            out_size.SetValue(size);
            return ResultSuccess();
        }

        char placeholder_path[FS_MAX_PATH] = {0};
        struct stat st;

        this->placeholder_accessor.GetPath(placeholder_path, placeholder_id);
        R_UNLESS(stat(placeholder_path, &st) != -1, fsdevGetLastResult());

        out_size.SetValue(st.st_size);
        return ResultSuccess();
    }

    Result ContentStorageInterface::RepairInvalidFileAttribute() {
        char content_root_path[FS_MAX_PATH] = {0};
        this->GetContentRootPath(content_root_path);
        unsigned int dir_depth = this->GetContentDirectoryDepth();
        auto fix_file_attributes = [&](bool* should_continue, bool* should_retry_dir_read, const char* current_path, struct dirent* dir_entry) {
            *should_retry_dir_read = false;
            *should_continue = true;

            if (dir_entry->d_type == DT_DIR) {
                if (path::IsNcaPath(current_path)) {
                    if (R_SUCCEEDED(fsdevSetConcatenationFileAttribute(current_path))) {
                        *should_retry_dir_read = true;
                    }
                }
            }

            return ResultSuccess();
        };

        R_TRY(fs::TraverseDirectory(content_root_path, dir_depth, fix_file_attributes));

        char placeholder_root_path[FS_MAX_PATH] = {0};
        this->placeholder_accessor.InvalidateAll();
        this->placeholder_accessor.MakeRootPath(placeholder_root_path);
        dir_depth = this->placeholder_accessor.GetDirectoryDepth();

        R_TRY(fs::TraverseDirectory(placeholder_root_path, dir_depth, fix_file_attributes));

        return ResultSuccess();
    }

    Result ContentStorageInterface::GetRightsIdFromPlaceHolderIdWithCache(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        R_TRY(this->EnsureEnabled());
        
        {
            std::scoped_lock lk(this->rights_id_cache->mutex);

            /* Attempt to locate the content id in the cache. */
            for (size_t i = 0; i < impl::RightsIdCache::MaxEntries; i++) {
                impl::RightsIdCache::Entry* entry = &this->rights_id_cache->entries[i];

                if (entry->last_accessed != 1 && cache_content_id == entry->uuid) {
                    entry->last_accessed = this->rights_id_cache->counter;
                    this->rights_id_cache->counter++;
                    out_rights_id.SetValue(entry->rights_id);
                    out_key_generation.SetValue(entry->key_generation);
                    return ResultSuccess();
                }
            }
        }

        FsRightsId rights_id = {0};
        u8 key_generation = 0;
        char placeholder_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        this->placeholder_accessor.GetPath(placeholder_path, placeholder_id);
        R_TRY(fs::ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, placeholder_path));
        R_TRY(fsGetRightsIdAndKeyGenerationByPath(common_path, &key_generation, &rights_id));

        {
            std::scoped_lock lk(this->rights_id_cache->mutex);
            impl::RightsIdCache::Entry* eviction_candidate = &this->rights_id_cache->entries[0];

            /* Find a suitable existing entry to store our new one at. */
            for (size_t i = 1; i < impl::RightsIdCache::MaxEntries; i++) {
                impl::RightsIdCache::Entry* entry = &this->rights_id_cache->entries[i];

                /* Change eviction candidates if the uuid already matches ours, or if the uuid doesn't already match and the last_accessed count is lower */
                if (cache_content_id == entry->uuid || (cache_content_id != eviction_candidate->uuid && entry->last_accessed < eviction_candidate->last_accessed)) {
                    eviction_candidate = entry;
                }
            }

            /* Update the cache. */
            eviction_candidate->uuid = cache_content_id.uuid;
            eviction_candidate->rights_id = rights_id;
            eviction_candidate->key_generation = key_generation;
            eviction_candidate->last_accessed = this->rights_id_cache->counter;
            this->rights_id_cache->counter++;

            /* Set output. */
            out_rights_id.SetValue(rights_id);
            out_key_generation.SetValue(key_generation);
        }

        return ResultSuccess();
    }

}
