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

#include "debug.hpp"

namespace sts::ncm {

    Result ReadOnlyContentStorageInterface::Initialize(const char* root_path, MakeContentPathFunc content_path_func) {
        R_DEBUG_START
        R_TRY(this->EnsureEnabled());

        const size_t root_path_len = strnlen(root_path, FS_MAX_PATH-1);

        if (root_path_len >= FS_MAX_PATH-1) {
            std::abort();
        }

        strncpy(this->root_path, root_path, FS_MAX_PATH-2);
        this->make_content_path_func = *content_path_func;
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GeneratePlaceHolderId(Out<PlaceHolderId> out) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::HasPlaceHolder(Out<bool> out, PlaceHolderId placeholder_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, InBuffer<u8> data) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::Delete(ContentId content_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::Has(Out<bool> out, ContentId content_id) {
        R_DEBUG_START
        R_TRY(this->EnsureEnabled());

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
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetPath(OutPointerWithServerSize<lr::Path, 0x1> out, ContentId content_id) {
        R_DEBUG_START
        R_TRY(this->EnsureEnabled());

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
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetPlaceHolderPath(OutPointerWithServerSize<lr::Path, 0x1> out, PlaceHolderId placeholder_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::CleanupAllPlaceHolder() {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::ListPlaceHolder(Out<u32> out_count, OutBuffer<PlaceHolderId> out_buf) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetContentCount(Out<u32> out_count) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::ListContentId(Out<u32> out_count, OutBuffer<ContentId> out_buf, u32 start_offset) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetSizeFromContentId(Out<u64> out_size, ContentId content_id) {
        R_DEBUG_START
        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        bool is_content_file = false;

        this->make_content_path_func(content_path, content_id, this->root_path);
        R_TRY(HasFile(&is_content_file, content_path));
        
        if (!is_content_file) {
            path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        }

        struct stat st;
        if (stat(content_path, &st) == -1) {
            return fsdevGetLastResult();
        }

        out_size.SetValue(st.st_size);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::DisableForcibly() {
        R_DEBUG_START
        this->disabled = true;
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::ReadContentIdFile(OutBuffer<u8> buf, ContentId content_id, u64 offset) {
        R_DEBUG_START
        /* Offset is too large */
        if (offset >> 0x3f != 0) {
            return ResultNcmInvalidOffset;
        }

        R_TRY(this->EnsureEnabled());

        char content_path[FS_MAX_PATH] = {0};
        bool is_content_file = false;

        this->make_content_path_func(content_path, content_id, this->root_path);
        R_TRY(HasFile(&is_content_file, content_path));
        
        if (!is_content_file) {
            path::GetContentMetaPath(content_path, content_id, this->make_content_path_func, this->root_path);
        }
   
        FILE* f = nullptr;
        R_TRY(OpenFile(&f, content_path, FS_OPEN_READ));
        
        ON_SCOPE_EXIT {
            fclose(f);
        };
   
        R_TRY(ReadFile(f, offset, buf.buffer, buf.num_elements));

        return ResultSuccess;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromPlaceHolderId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromContentId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, ContentId content_id) {
        R_DEBUG_START
        R_TRY(this->EnsureEnabled());

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
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::WriteContentForDebug(ContentId content_id, u64 offset, InBuffer<u8> data) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetFreeSpaceSize(Out<u64> out_size) {
        R_DEBUG_START
        out_size.SetValue(0);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetTotalSpaceSize(Out<u64> out_size) {
        R_DEBUG_START
        out_size.SetValue(0);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::FlushPlaceHolder() {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetSizeFromPlaceHolderId(Out<u64> out, PlaceHolderId placeholder_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::RepairInvalidFileAttribute() {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

    Result ReadOnlyContentStorageInterface::GetRightsIdFromPlaceHolderIdWithCache(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        R_DEBUG_START
        return ResultNcmInvalidContentStorageOperation;
        R_DEBUG_END
    }

}