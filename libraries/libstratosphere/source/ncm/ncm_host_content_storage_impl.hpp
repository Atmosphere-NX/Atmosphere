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
#include <stratosphere.hpp>

namespace ams::ncm {

    class HostContentStorageImpl {
        protected:
            RegisteredHostContent *m_registered_content;
            bool m_disabled;
        protected:
            /* Helpers. */
            Result EnsureEnabled() const {
                R_UNLESS(!m_disabled, ncm::ResultInvalidContentStorage());
                return ResultSuccess();
            }

            static Result GetRightsId(ncm::RightsId *out_rights_id, const Path &path) {
                if (hos::GetVersion() >= hos::Version_3_0_0) {
                    R_TRY(fs::GetRightsId(std::addressof(out_rights_id->id), std::addressof(out_rights_id->key_generation), path.str));
                } else {
                    R_TRY(fs::GetRightsId(std::addressof(out_rights_id->id), path.str));
                    out_rights_id->key_generation = 0;
                }
                return ResultSuccess();
            }
        public:
            HostContentStorageImpl(RegisteredHostContent *registered_content) : m_registered_content(registered_content), m_disabled(false) { /* ... */ }
        public:
            /* Actual commands. */
            virtual Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out);
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size);
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id);
            virtual Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id);
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data);
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id);
            virtual Result Delete(ContentId content_id);
            virtual Result Has(sf::Out<bool> out, ContentId content_id);
            virtual Result GetPath(sf::Out<Path> out, ContentId content_id);
            virtual Result GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id);
            virtual Result CleanupAllPlaceHolder();
            virtual Result ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf);
            virtual Result GetContentCount(sf::Out<s32> out_count);
            virtual Result ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 start_offset);
            virtual Result GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id);
            virtual Result DisableForcibly();
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id);
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size);
            virtual Result ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset);
            virtual Result GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id);
            virtual Result GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id);
            virtual Result GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id);
            virtual Result GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id);
            virtual Result WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data);
            virtual Result GetFreeSpaceSize(sf::Out<s64> out_size);
            virtual Result GetTotalSpaceSize(sf::Out<s64> out_size);
            virtual Result FlushPlaceHolder();
            virtual Result GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id);
            virtual Result RepairInvalidFileAttribute();
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id);
            virtual Result RegisterPath(const ContentId &content_id, const Path &path);
            virtual Result ClearRegisteredPath();
    };
    static_assert(ncm::IsIContentStorage<HostContentStorageImpl>);

}
