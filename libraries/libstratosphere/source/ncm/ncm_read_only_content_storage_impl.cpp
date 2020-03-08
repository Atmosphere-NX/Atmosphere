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
#include <stratosphere.hpp>
#include "ncm_read_only_content_storage_impl.hpp"
#include "ncm_fs_utils.hpp"

namespace ams::ncm {

    namespace {

        void MakeContentPath(PathString *out, ContentId id, MakeContentPathFunction func, const char *root_path) {
            return func(out, id, root_path);
        }

        void MakeGameCardContentMetaPath(PathString *out, ContentId id, MakeContentPathFunction func, const char *root_path) {
            /* Determine the content path. */
            PathString path;
            func(std::addressof(path), id, root_path);

            /* Substitute the .nca extension with .cmnt.nca. */
            *out = path.GetSubstring(0, path.GetLength() - 4);
            out->Append(".cnmt.nca");
        }

        Result OpenContentIdFileImpl(fs::FileHandle *out, ContentId id, MakeContentPathFunction func, const char *root_path) {
            PathString path;
            MakeContentPath(std::addressof(path), id, func, root_path);

            /* Open the content file. */
            /* If absent, make the path for game card content meta and open again. */
            R_TRY_CATCH(fs::OpenFile(out, path, fs::OpenMode_Read)) {
                R_CATCH(fs::ResultPathNotFound) {
                    MakeGameCardContentMetaPath(std::addressof(path), id, func, root_path);
                    R_TRY(fs::OpenFile(out, path, fs::OpenMode_Read));
                }
            } R_END_TRY_CATCH;

            return ResultSuccess();
        }

    }

    Result ReadOnlyContentStorageImpl::Initialize(const char *path, MakeContentPathFunction content_path_func) {
        R_TRY(this->EnsureEnabled());
        this->root_path.Set(path);
        this->make_content_path_func = content_path_func;
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, sf::InBuffer data) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::Delete(ContentId content_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::Has(sf::Out<bool> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        /* Make the content path. */
        PathString content_path;
        MakeContentPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);

        /* Check if the file exists. */
        bool has;
        R_TRY(fs::HasFile(std::addressof(has), content_path));

        /* If the file is absent, make the path for game card content meta and check presence again. */
        if (!has) {
            MakeGameCardContentMetaPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);
            R_TRY(fs::HasFile(std::addressof(has), content_path));
        }

        out.SetValue(has);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GetPath(sf::Out<Path> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        /* Make the path for game card content meta. */
        PathString content_path;
        MakeGameCardContentMetaPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);

        /* Check if the file exists. */
        bool has_file;
        R_TRY(fs::HasFile(std::addressof(has_file), content_path));

        /* If the file is absent, make the path for regular content. */
        if (!has_file) {
            MakeContentPath(std::addressof(content_path), content_id, this->make_content_path_func, this->root_path);
        }

        /* Substitute mount name with the common mount name. */
        Path common_path;
        R_TRY(fs::ConvertToFsCommonPath(common_path.str, sizeof(common_path.str), content_path));

        out.SetValue(common_path);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::CleanupAllPlaceHolder() {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::GetContentCount(sf::Out<s32> out_count) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 offset) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        /* Open the file for the content id. */
        fs::FileHandle file;
        R_TRY(OpenContentIdFileImpl(std::addressof(file), content_id, this->make_content_path_func, this->root_path));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Determine the file size. */
        s64 file_size;
        R_TRY(fs::GetFileSize(std::addressof(file_size), file));

        out_size.SetValue(file_size);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::DisableForcibly() {
        this->disabled = true;
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::ReadContentIdFile(sf::OutBuffer buf, ContentId content_id, s64 offset) {
        /* Ensure offset is valid. */
        R_UNLESS(offset >= 0, ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        /* Open the file for the content id. */
        fs::FileHandle file;
        R_TRY(OpenContentIdFileImpl(std::addressof(file), content_id, this->make_content_path_func, this->root_path));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Read from the given offset up to the given size. */
        R_TRY(fs::ReadFile(file, offset, buf.GetPointer(), buf.GetSize()));

        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) {
        /* Obtain the regular rights id for the content id. */
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromContentId(&rights_id, content_id));

        /* Output the fs rights id. */
        out_rights_id.SetValue(rights_id.id);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        /* Get the content path. */
        Path path;
        R_TRY(this->GetPath(std::addressof(path), content_id));

        /* Get the rights id. */
        ncm::RightsId rights_id;
        R_TRY(GetRightsId(&rights_id, path));
        out_rights_id.SetValue(rights_id);

        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::WriteContentForDebug(ContentId content_id, s64 offset, sf::InBuffer data) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::GetFreeSpaceSize(sf::Out<s64> out_size) {
        out_size.SetValue(0);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::GetTotalSpaceSize(sf::Out<s64> out_size) {
        out_size.SetValue(0);
        return ResultSuccess();
    }

    Result ReadOnlyContentStorageImpl::FlushPlaceHolder() {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::RepairInvalidFileAttribute() {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

    Result ReadOnlyContentStorageImpl::GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        return ncm::ResultWriteToReadOnlyContentStorage();
    }

}
