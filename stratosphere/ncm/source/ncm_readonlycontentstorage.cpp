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

#include "ncm_fs.hpp"
#include "ncm_path_utils.hpp"
#include "ncm_readonlycontentstorage.hpp"

namespace sts::ncm {

    Result ReadOnlyContentStorageInterface::Initialize(const char* root_path, MakeContentPathFunc content_path_func) {
        if (this->disabled) {
            return ResultNcmInvalidContentStorage;
        }

        const size_t root_path_len = strnlen(root_path, FS_MAX_PATH-1);

        if (root_path_len >= FS_MAX_PATH-1) {
            std::abort();
        }

        strncpy(this->root_path, root_path, FS_MAX_PATH-2);
        this->make_content_path_func = *content_path_func;
        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::GeneratePlaceHolderId(Out<PlaceHolderId> out) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::HasPlaceHolder(Out<bool> out, PlaceHolderId placeholder_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, InBuffer<u8> data) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::Delete(ContentId content_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::Has(Out<bool> out, ContentId content_id) {
        if (this->disabled) {
            return ResultNcmInvalidContentStorage;
        }

        char content_path[FS_MAX_PATH] = {0};
        this->make_content_path_func(content_path, content_id, this->root_path);

        bool has = false;
        R_TRY(HasFile(&has, content_path));
        
        if (!has) {
            path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
            R_TRY(HasFile(&has, content_path));
        }

        out.SetValue(has);
        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::GetPath(OutPointerWithServerSize<lr::Path, 0x1> out, ContentId content_id) {
        if (this->disabled) {
            return ResultNcmInvalidContentStorage;
        }

        char content_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        bool is_content_meta_file = false;

        path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        R_TRY(HasFile(&is_content_meta_file, content_path));
        
        if (!is_content_meta_file) {
            this->make_content_path_func(content_path, content_id, this->root_path);
        }
        
        R_TRY(ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, content_path));
        *out.pointer = common_path;

        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::GetPlaceHolderPath(OutPointerWithServerSize<lr::Path, 0x1> out, PlaceHolderId placeholder_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::CleanupAllPlaceHolder() {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::ListPlaceHolder(Out<u32> out_count, OutBuffer<PlaceHolderId> out_buf) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::GetContentCount(Out<u32> out_count) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::ListContentId(Out<u32> out_count, OutBuffer<ContentId> out_buf, u32 start_offset) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::GetSizeFromContentId(Out<u64> out_size, ContentId content_id) {
        if (this->disabled) {
            return ResultNcmInvalidContentStorage;
        }

        char content_path[FS_MAX_PATH] = {0};
        bool is_content_file = false;

        this->make_content_path_func(content_path, content_id, this->root_path);
        R_TRY(HasFile(&is_content_file, content_path));
        
        if (!is_content_file) {
            path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        }

        struct stat st;
        errno = 0;
        stat(content_path, &st);

        if (errno != 0) {
            return fsdevGetLastResult();
        }

        out_size.SetValue(st.st_size);
        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::DisableForcibly() {
        this->disabled = true;
        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::ReadContentIdFile(OutBuffer<u8> buf, ContentId content_id, u64 offset) {
        /* Offset is too large */
        if (offset >> 0x3f != 0) {
            return ResultNcmInvalidOffset;
        }

        if (this->disabled) {
            return ResultNcmInvalidContentStorage;
        }

        char content_path[FS_MAX_PATH] = {0};
        bool is_content_file = false;

        this->make_content_path_func(content_path, content_id, this->root_path);
        R_TRY(HasFile(&is_content_file, content_path));
        
        if (!is_content_file) {
            path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        }
   
        errno = 0;
        FILE* f = fopen(content_path, "rb");

        ON_SCOPE_EXIT {
            fclose(f);
        };

        if (f == NULL || errno != 0) {
            return fsdevGetLastResult();
        }

        errno = 0;    
        fseek(f, offset, SEEK_SET);
        fread(buf.buffer, buf.num_elements, 1, f);

        if (errno != 0) {
            return fsdevGetLastResult();
        }

        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromPlaceHolderId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromContentId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, ContentId content_id) {
        if (this->disabled) {
            return ResultNcmInvalidContentStorage;
        }

        FsRightsId rights_id = {0};
        u8 key_generation = 0;

        char content_path[FS_MAX_PATH] = {0};
        char common_path[FS_MAX_PATH] = {0};
        bool is_content_meta_file = false;

        path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        R_TRY(HasFile(&is_content_meta_file, content_path));
        
        if (!is_content_meta_file) {
            this->make_content_path_func(content_path, content_id, this->root_path);
        }

        R_TRY(ConvertToFsCommonPath(common_path, FS_MAX_PATH-1, content_path));
        R_TRY(fsGetRightsIdAndKeyGenerationByPath(common_path, &key_generation, &rights_id));

        out_rights_id.SetValue(rights_id);
        out_key_generation.SetValue(static_cast<u64>(key_generation));

        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::WriteContentForDebug(ContentId content_id, u64 offset, InBuffer<u8> data) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::GetFreeSpaceSize(Out<u64> out_size) {
        out_size.SetValue(0);
        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::GetTotalSpaceSize(Out<u64> out_size) {
        out_size.SetValue(0);
        return ResultSuccess;
    }

    Result ReadOnlyContentStorageInterface::FlushPlaceHolder() {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::GetSizeFromPlaceHolderId(Out<u64> out, PlaceHolderId placeholder_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::RepairInvalidFileAttribute() {
        return ResultNcmInvalidContentStorageOperation;
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromPlaceHolderIdWithCache(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        return ResultNcmInvalidContentStorageOperation;
    }

}