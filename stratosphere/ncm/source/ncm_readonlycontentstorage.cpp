/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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

#include "ncm_fs.hpp"
#include "ncm_path_utils.hpp"
#include "ncm_readonlycontentstorage.hpp"

namespace ams::ncm {

    Result ReadOnlyContentStorageInterface::Initialize(const char* root_path, MakeContentPathFunc content_path_func) {
        R_TRY(this->EnsureEnabled());

        const size_t root_path_len = strnlen(root_path, FS_MAX_PATH-1);
        AMS_ABORT_UNLESS(root_path_len < FS_MAX_PATH-1);
        strncpy(this->root_path, root_path, FS_MAX_PATH-2);
        this->make_content_path_func = *content_path_func;
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, sf::InBuffer data) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::Delete(ContentId content_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::Has(sf::Out<bool> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        this->make_content_path_func(content_path, content_id, this->root_path);

        bool has = false;
        R_TRY(fs::HasFile(&has, content_path));
        
        if (!has) {
            path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
            R_TRY(fs::HasFile(&has, content_path));
        }

        out.SetValue(has);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::GetPath(sf::Out<lr::Path> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        bool is_content_meta_file = false;

        path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        R_TRY(fs::HasFile(&is_content_meta_file, content_path));
        
        if (!is_content_meta_file) {
            this->make_content_path_func(content_path, content_id, this->root_path);
        }
        
        R_TRY(fs::ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, content_path));
        out.SetValue(lr::Path::Encode(common_path));

        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::GetPlaceHolderPath(sf::Out<lr::Path> out, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::CleanupAllPlaceHolder() {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::ListPlaceHolder(sf::Out<u32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::GetContentCount(sf::Out<u32> out_count) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::ListContentId(sf::Out<u32> out_count, const sf::OutArray<ContentId> &out_buf, u32 start_offset) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::GetSizeFromContentId(sf::Out<u64> out_size, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        bool is_content_file = false;

        this->make_content_path_func(content_path, content_id, this->root_path);
        R_TRY(fs::HasFile(&is_content_file, content_path));
        
        if (!is_content_file) {
            path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        }

        struct stat st;
        R_UNLESS(stat(content_path, &st) != -1, fsdevGetLastResult());
        out_size.SetValue(st.st_size);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::DisableForcibly() {
        this->disabled = true;
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::ReadContentIdFile(sf::OutBuffer buf, ContentId content_id, u64 offset) {
        /* Offset is too large */
        R_UNLESS(offset <= std::numeric_limits<s64>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        bool is_content_file = false;

        this->make_content_path_func(content_path, content_id, this->root_path);
        R_TRY(fs::HasFile(&is_content_file, content_path));
        
        if (!is_content_file) {
            path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        }
   
        FILE* f = nullptr;
        R_TRY(fs::OpenFile(&f, content_path, FsOpenMode_Read));
        
        ON_SCOPE_EXIT {
            fclose(f);
        };
   
        R_TRY(fs::ReadFile(f, offset, buf.GetPointer(), buf.GetSize()));
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromPlaceHolderId(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromContentId(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        FsRightsId rights_id = {0};
        u8 key_generation = 0;

        char content_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        bool is_content_meta_file = false;

        path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        R_TRY(fs::HasFile(&is_content_meta_file, content_path));
        
        if (!is_content_meta_file) {
            this->make_content_path_func(content_path, content_id, this->root_path);
        }

        R_TRY(fs::ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, content_path));
        R_TRY(fsGetRightsIdAndKeyGenerationByPath(common_path, &key_generation, &rights_id));

        out_rights_id.SetValue(rights_id);
        out_key_generation.SetValue(static_cast<u64>(key_generation));

        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::WriteContentForDebug(ContentId content_id, u64 offset, sf::InBuffer data) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::GetFreeSpaceSize(sf::Out<u64> out_size) {
        out_size.SetValue(0);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::GetTotalSpaceSize(sf::Out<u64> out_size) {
        out_size.SetValue(0);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageInterface::FlushPlaceHolder() {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::GetSizeFromPlaceHolderId(sf::Out<u64> out, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::RepairInvalidFileAttribute() {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromPlaceHolderIdWithCache(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        return ResultInvalidContentStorageOperation();
    }

}
