/*
 * Copyright (c) Atmosphère-NX
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
#include "ncm_host_content_storage_impl.hpp"

namespace ams::ncm {

    Result HostContentStorageImpl::GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
        AMS_UNUSED(out);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) {
        AMS_UNUSED(placeholder_id, content_id, size);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        AMS_UNUSED(placeholder_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
        AMS_UNUSED(out, placeholder_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data) {
        AMS_UNUSED(placeholder_id, offset, data);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        AMS_UNUSED(placeholder_id, content_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::Delete(ContentId content_id) {
        AMS_UNUSED(content_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::Has(sf::Out<bool> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());

        /* Attempt to locate the content. */
        Path path;
        R_TRY_CATCH(m_registered_content->GetPath(std::addressof(path), content_id)) {
            /* The content is absent, this is fine. */
            R_CATCH(ncm::ResultContentNotFound) {
                out.SetValue(false);
                R_SUCCEED();
            }
        } R_END_TRY_CATCH;

        out.SetValue(true);
        R_SUCCEED();
    }

    Result HostContentStorageImpl::GetPath(sf::Out<Path> out, ContentId content_id) {
        R_TRY(this->EnsureEnabled());
        R_RETURN(m_registered_content->GetPath(out.GetPointer(), content_id));
    }

    Result HostContentStorageImpl::GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) {
        AMS_UNUSED(out, placeholder_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::CleanupAllPlaceHolder() {
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
        AMS_UNUSED(out_count, out_buf);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetContentCount(sf::Out<s32> out_count) {
        AMS_UNUSED(out_count);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 offset) {
        AMS_UNUSED(out_count, out_buf, offset);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) {
        AMS_UNUSED(out_size, content_id);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result HostContentStorageImpl::DisableForcibly() {
        m_disabled = true;
        R_SUCCEED();
    }

    Result HostContentStorageImpl::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        AMS_UNUSED(placeholder_id, old_content_id, new_content_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) {
        AMS_UNUSED(placeholder_id, size);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset) {
        AMS_UNUSED(buf, content_id, offset);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result HostContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        AMS_UNUSED(out_rights_id, placeholder_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        AMS_UNUSED(out_rights_id, placeholder_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, fs::ContentAttributes attr) {
        AMS_UNUSED(out_rights_id, placeholder_id, attr);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) {
        /* Obtain the regular rights id for the content id. */
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromContentIdDeprecated2(std::addressof(rights_id), content_id));

        /* Output the fs rights id. */
        out_rights_id.SetValue(rights_id.id);
        R_SUCCEED();
    }

    Result HostContentStorageImpl::GetRightsIdFromContentIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) {
        R_RETURN(this->GetRightsIdFromContentId(out_rights_id, content_id, fs::ContentAttributes_None));
    }

    Result HostContentStorageImpl::GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id, fs::ContentAttributes attr) {
        R_TRY(this->EnsureEnabled());

        /* Get the content path. */
        Path path;
        R_TRY(m_registered_content->GetPath(std::addressof(path), content_id));

        /* Acquire the rights id for the content. */
        RightsId rights_id;
        R_TRY_CATCH(GetRightsId(std::addressof(rights_id), path, attr)) {
            /* The content is absent, output a blank rights id. */
            R_CATCH(fs::ResultTargetNotFound) {
                out_rights_id.SetValue({});
                R_SUCCEED();
            }
        } R_END_TRY_CATCH;

        /* Output the rights id. */
        out_rights_id.SetValue(rights_id);
        R_SUCCEED();
    }

    Result HostContentStorageImpl::WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data) {
        AMS_UNUSED(content_id, offset, data);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetFreeSpaceSize(sf::Out<s64> out_size) {
        out_size.SetValue(0);
        R_SUCCEED();
    }

    Result HostContentStorageImpl::GetTotalSpaceSize(sf::Out<s64> out_size) {
        out_size.SetValue(0);
        R_SUCCEED();
    }

    Result HostContentStorageImpl::FlushPlaceHolder() {
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id) {
        AMS_UNUSED(out, placeholder_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::RepairInvalidFileAttribute() {
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetRightsIdFromPlaceHolderIdWithCacheDeprecated(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        AMS_UNUSED(out_rights_id, placeholder_id, cache_content_id);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id, fs::ContentAttributes attr) {
        AMS_UNUSED(out_rights_id, placeholder_id, cache_content_id, attr);
        R_THROW(ncm::ResultNotSupported());
    }

    Result HostContentStorageImpl::RegisterPath(const ContentId &content_id, const Path &path) {
        AMS_ABORT_UNLESS(spl::IsDevelopment());
        R_RETURN(m_registered_content->RegisterPath(content_id, path));
    }

    Result HostContentStorageImpl::ClearRegisteredPath() {
        AMS_ABORT_UNLESS(spl::IsDevelopment());
        m_registered_content->ClearPaths();
        R_SUCCEED();
    }

    Result HostContentStorageImpl::GetProgramId(sf::Out<ncm::ProgramId> out, ContentId content_id, fs::ContentAttributes attr) {
        R_TRY(this->EnsureEnabled());

        /* Get the content path. */
        Path path;
        R_TRY(m_registered_content->GetPath(std::addressof(path), content_id));

        /* Check for correct extension. */
        const auto path_len = std::strlen(path.str);
        const char *extension = path.str + path_len - 1;
        if (*extension == '/') {
            --extension;
        }
        R_UNLESS(path_len >= 4 && std::memcmp(extension - 4, ".ncd", 4) == 0, ncm::ResultInvalidContentMetaDirectory());

        /* Obtain the program id for the content. */
        ncm::ProgramId program_id;
        R_TRY(fs::GetProgramId(std::addressof(program_id), path.str, attr));

        out.SetValue(program_id);
        R_SUCCEED();
    }

}
