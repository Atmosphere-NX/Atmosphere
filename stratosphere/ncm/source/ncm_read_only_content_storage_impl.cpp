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
#include "ncm_read_only_content_storage_impl.hpp"

namespace ams::ncm {

    Result ReadOnlyContentStorageImpl::Initialize(const char *root_path, MakeContentPathFunc content_path_func) {
        R_TRY(this->EnsureEnabled());

        const size_t root_path_len = strnlen(root_path, FS_MAX_PATH-1);
        AMS_ABORT_UNLESS(root_path_len < FS_MAX_PATH-1);
        strncpy(this->root_path, root_path, FS_MAX_PATH-2);
        this->make_content_path_func = *content_path_func;
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, sf::InBuffer data) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::Delete(ContentId content_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::Has(sf::Out<bool> out, ContentId content_id) {
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

    Result ReadOnlyContentStorageImpl::GetPath(sf::Out<lr::Path> out, ContentId content_id) {
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

    Result ReadOnlyContentStorageImpl::GetPlaceHolderPath(sf::Out<lr::Path> out, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::CleanupAllPlaceHolder() {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::ListPlaceHolder(sf::Out<u32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::GetContentCount(sf::Out<u32> out_count) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::ListContentId(sf::Out<u32> out_count, const sf::OutArray<ContentId> &out_buf, u32 start_offset) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::GetSizeFromContentId(sf::Out<u64> out_size, ContentId content_id) {
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

    Result ReadOnlyContentStorageImpl::DisableForcibly() {
        this->disabled = true;
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::ReadContentIdFile(sf::OutBuffer buf, ContentId content_id, u64 offset) {
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
   
        FILE *f = nullptr;
        R_TRY(fs::OpenFile(&f, content_path, FsOpenMode_Read));
        
        ON_SCOPE_EXIT {
            fclose(f);
        };
   
        R_TRY(fs::ReadFile(f, offset, buf.GetPointer(), buf.GetSize()));
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) {
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromContentId(&rights_id, content_id));
        out_rights_id.SetValue(rights_id.id);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) {
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
        
        ncm::RightsId rights_id;
        R_TRY(GetRightsId(&rights_id, common_path));
        out_rights_id.SetValue(rights_id);

        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::WriteContentForDebug(ContentId content_id, u64 offset, sf::InBuffer data) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::GetFreeSpaceSize(sf::Out<u64> out_size) {
        out_size.SetValue(0);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GetTotalSpaceSize(sf::Out<u64> out_size) {
        out_size.SetValue(0);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::FlushPlaceHolder() {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::GetSizeFromPlaceHolderId(sf::Out<u64> out, PlaceHolderId placeholder_id) {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::RepairInvalidFileAttribute() {
        return ResultInvalidContentStorageOperation();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        return ResultInvalidContentStorageOperation();
    }

}
