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

namespace ams::ncm {

    Result IntegratedContentStorageImpl::GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
        AMS_UNUSED(out);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) {
        AMS_UNUSED(placeholder_id, content_id, size);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        AMS_UNUSED(placeholder_id);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
        AMS_UNUSED(placeholder_id);

        /* Integrated storages cannot have placeholders. */
        *out = false;
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data) {
        AMS_UNUSED(placeholder_id, offset, data);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        AMS_UNUSED(placeholder_id, content_id);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::Delete(ContentId content_id) {
        AMS_UNUSED(content_id);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::Has(sf::Out<bool> out, ContentId content_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* If we don't locate the content, set the output to false. */
        *out = false;
        ON_RESULT_INCLUDED(ncm::ResultContentNotFound) { *out = false; };

        /* Check each interface in turn. */
        R_TRY(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            R_TRY(data.interface->Has(out, content_id));

            /* If it doesn't, continue on. */
            R_UNLESS(*out, ncm::ResultContentNotFound());

            /* If it does, we're done looking. */
            R_SUCCEED();
        }));

        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetPath(sf::Out<Path> out, ContentId content_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentNotFound());

        /* Check each interface in turn. */
        R_TRY(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            bool has;
            R_TRY(data.interface->Has(std::addressof(has), content_id));

            /* If it doesn't, continue on. */
            R_UNLESS(has, ncm::ResultContentNotFound());


            /* If it does, get the path. */
            R_RETURN(data.interface->GetPath(out, content_id));
        }));

        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) {
        AMS_UNUSED(out, placeholder_id);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::CleanupAllPlaceHolder() {
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
        AMS_UNUSED(out_buf);
        *out_count = 0;
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetContentCount(sf::Out<s32> out_count) {
        *out_count = 0;
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 offset) {
        AMS_UNUSED(out_buf, offset);
        *out_count = 0;
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentNotFound());

        /* Check each interface in turn. */
        R_TRY(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            bool has;
            R_TRY(data.interface->Has(std::addressof(has), content_id));

            /* If it doesn't, continue on. */
            R_UNLESS(has, ncm::ResultContentNotFound());


            /* If it does, get the size. */
            R_RETURN(data.interface->GetSizeFromContentId(out_size, content_id));
        }));

        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::DisableForcibly() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        m_disabled = true;
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        AMS_UNUSED(placeholder_id, old_content_id, new_content_id);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) {
        AMS_UNUSED(placeholder_id, size);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentNotFound());

        /* Check each interface in turn. */
        R_TRY(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            bool has;
            R_TRY(data.interface->Has(std::addressof(has), content_id));

            /* If it doesn't, continue on. */
            R_UNLESS(has, ncm::ResultContentNotFound());


            /* If it does, read the file. */
            R_RETURN(data.interface->ReadContentIdFile(buf, content_id, offset));
        }));

        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        /* Obtain the regular rights id for the placeholder id. */
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromPlaceHolderIdDeprecated2(std::addressof(rights_id), placeholder_id));

        /* Output the fs rights id. */
        out_rights_id.SetValue(rights_id.id);
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetRightsIdFromPlaceHolderIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
        R_RETURN(this->GetRightsIdFromPlaceHolderId(out_rights_id, placeholder_id, fs::ContentAttributes_None));
    }

    Result IntegratedContentStorageImpl::GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, fs::ContentAttributes attr) {
        AMS_UNUSED(out_rights_id, placeholder_id, attr);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) {
        /* Obtain the regular rights id for the content id. */
        ncm::RightsId rights_id;
        R_TRY(this->GetRightsIdFromContentIdDeprecated2(std::addressof(rights_id), content_id));

        /* Output the fs rights id. */
        out_rights_id.SetValue(rights_id.id);
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetRightsIdFromContentIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) {
        R_RETURN(this->GetRightsIdFromContentId(out_rights_id, content_id, fs::ContentAttributes_None));
    }

    Result IntegratedContentStorageImpl::GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id, fs::ContentAttributes attr) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentNotFound());

        /* Check each interface in turn. */
        R_TRY(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            bool has;
            R_TRY(data.interface->Has(std::addressof(has), content_id));

            /* If it doesn't, continue on. */
            R_UNLESS(has, ncm::ResultContentNotFound());


            /* If it does, read the file. */
            R_RETURN(data.interface->GetRightsIdFromContentId(out_rights_id, content_id, attr));
        }));

        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data) {
        AMS_UNUSED(content_id, offset, data);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::GetFreeSpaceSize(sf::Out<s64> out_size) {
        out_size.SetValue(0);
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetTotalSpaceSize(sf::Out<s64> out_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Determine the total size. */
        s64 total_size = 0;
        R_TRY(m_list.ForAll([&](const auto &data) {
            /* Get the current size. */
            s64 cur_size;
            R_TRY(data.interface->GetTotalSpaceSize(std::addressof(cur_size)));

            /* Add to the total. */
            total_size += cur_size;
            R_SUCCEED();
        }));

        *out_size = total_size;
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::FlushPlaceHolder() {
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id) {
        AMS_UNUSED(out, placeholder_id);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::RepairInvalidFileAttribute() {
        R_SUCCEED();
    }

    Result IntegratedContentStorageImpl::GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id, fs::ContentAttributes attr) {
        AMS_UNUSED(out_rights_id, placeholder_id, cache_content_id, attr);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::RegisterPath(const ContentId &content_id, const Path &path) {
        AMS_UNUSED(content_id, path);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::ClearRegisteredPath() {
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentStorageImpl::GetProgramId(sf::Out<ncm::ProgramId> out, ContentId content_id, fs::ContentAttributes attr) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentNotFound());

        /* Check each interface in turn. */
        R_TRY(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            bool has;
            R_TRY(data.interface->Has(std::addressof(has), content_id));

            /* If it doesn't, continue on. */
            R_UNLESS(has, ncm::ResultContentNotFound());


            /* If it does, read the file. */
            R_RETURN(data.interface->GetProgramId(out, content_id, attr));
        }));

        R_SUCCEED();
    }

}
