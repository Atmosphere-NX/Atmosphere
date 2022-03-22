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
#pragma once
#include <stratosphere/ncm/ncm_i_content_storage.hpp>

namespace ams::ncm {

    class ContentStorage {
        NON_COPYABLE(ContentStorage);
        private:
            sf::SharedPointer<IContentStorage> m_interface;
        public:
            ContentStorage() { /* ... */ }
            explicit ContentStorage(sf::SharedPointer<IContentStorage> intf) : m_interface(intf) { /* ... */ }

            ContentStorage(ContentStorage &&rhs) {
                m_interface = std::move(rhs.m_interface);
            }

            ContentStorage &operator=(ContentStorage &&rhs) {
                ContentStorage(std::move(rhs)).swap(*this);
                return *this;
            }

            void swap(ContentStorage &rhs) {
                std::swap(m_interface, rhs.m_interface);
            }
        public:
            PlaceHolderId GeneratePlaceHolderId() {
                AMS_ASSERT(m_interface != nullptr);

                PlaceHolderId id;
                R_ABORT_UNLESS(m_interface->GeneratePlaceHolderId(std::addressof(id)));
                return id;
            }

            Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->CreatePlaceHolder(placeholder_id, content_id, size);
            }

            Result DeletePlaceHolder(PlaceHolderId placeholder_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->DeletePlaceHolder(placeholder_id);
            }

            Result HasPlaceHolder(bool *out, PlaceHolderId placeholder_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->HasPlaceHolder(out, placeholder_id);
            }

            Result WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const void *buf, size_t size) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->WritePlaceHolder(placeholder_id, offset, sf::InBuffer(buf, size));
            }

            Result Register(PlaceHolderId placeholder_id, ContentId content_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->Register(placeholder_id, content_id);
            }

            Result Delete(ContentId content_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->Delete(content_id);
            }

            Result Has(bool *out, ContentId content_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->Has(out, content_id);
            }

            void GetPath(Path *out, ContentId content_id) {
                AMS_ASSERT(m_interface != nullptr);
                R_ABORT_UNLESS(m_interface->GetPath(out, content_id));
            }

            void GetPlaceHolderPath(Path *out, PlaceHolderId placeholder_id) {
                AMS_ASSERT(m_interface != nullptr);
                R_ABORT_UNLESS(m_interface->GetPlaceHolderPath(out, placeholder_id));
            }

            Result CleanupAllPlaceHolder() {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->CleanupAllPlaceHolder();
            }

            Result ListPlaceHolder(s32 *out_count, PlaceHolderId *out_list, size_t out_list_size) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ListPlaceHolder(out_count, sf::OutArray<PlaceHolderId>(out_list, out_list_size));
            }

            Result GetContentCount(s32 *out_count) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->GetContentCount(out_count);
            }

            Result ListContentId(s32 *out_count, ContentId *out_list, size_t out_list_size, s32 offset) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ListContentId(out_count, sf::OutArray<ContentId>(out_list, out_list_size), offset);
            }

            Result GetSize(s64 *out_size, ContentId content_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->GetSizeFromContentId(out_size, content_id);
            }

            Result GetSize(s64 *out_size, PlaceHolderId placeholder_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->GetSizeFromPlaceHolderId(out_size, placeholder_id);
            }

            Result DisableForcibly() {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->DisableForcibly();
            }

            Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->RevertToPlaceHolder(placeholder_id, old_content_id, new_content_id);
            }

            Result SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->SetPlaceHolderSize(placeholder_id, size);
            }

            Result ReadContentIdFile(void *dst, size_t size, ContentId content_id, s64 offset) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ReadContentIdFile(sf::OutBuffer(dst, size), content_id, offset);
            }

            Result GetRightsId(ncm::RightsId *out_rights_id, PlaceHolderId placeholder_id) {
                AMS_ASSERT(m_interface != nullptr);

                const auto vers = hos::GetVersion();
                if (vers >= hos::Version_3_0_0) {
                    return m_interface->GetRightsIdFromPlaceHolderId(out_rights_id, placeholder_id);
                } else {
                    AMS_ABORT_UNLESS(vers >= hos::Version_2_0_0);
                    *out_rights_id = {};
                    return m_interface->GetRightsIdFromPlaceHolderIdDeprecated(std::addressof(out_rights_id->id), placeholder_id);
                }
            }

            Result GetRightsId(ncm::RightsId *out_rights_id, ContentId content_id) {
                AMS_ASSERT(m_interface != nullptr);

                const auto vers = hos::GetVersion();
                if (vers >= hos::Version_3_0_0) {
                    return m_interface->GetRightsIdFromContentId(out_rights_id, content_id);
                } else {
                    AMS_ABORT_UNLESS(vers >= hos::Version_2_0_0);
                    *out_rights_id = {};
                    return m_interface->GetRightsIdFromContentIdDeprecated(std::addressof(out_rights_id->id), content_id);
                }
            }

            Result WriteContentForDebug(ContentId content_id, s64 offset, const void *buf, size_t size) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->WriteContentForDebug(content_id, offset, sf::InBuffer(buf, size));
            }

            Result GetFreeSpaceSize(s64 *out_size) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->GetFreeSpaceSize(out_size);
            }

            Result GetTotalSpaceSize(s64 *out_size) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->GetTotalSpaceSize(out_size);
            }

            Result FlushPlaceHolder() {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->FlushPlaceHolder();
            }

            Result RepairInvalidFileAttribute() {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->RepairInvalidFileAttribute();
            }

            Result GetRightsIdFromPlaceHolderIdWithCache(ncm::RightsId *out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->GetRightsIdFromPlaceHolderIdWithCache(out_rights_id, placeholder_id, cache_content_id);
            }
    };

}
