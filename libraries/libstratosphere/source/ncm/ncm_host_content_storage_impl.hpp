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
                R_SUCCEED();
            }

            static Result GetRightsId(ncm::RightsId *out_rights_id, const Path &path, fs::ContentAttributes attr) {
                if (hos::GetVersion() >= hos::Version_3_0_0) {
                    R_TRY(fs::GetRightsId(std::addressof(out_rights_id->id), std::addressof(out_rights_id->key_generation), path.str, attr));
                } else {
                    R_TRY(fs::GetRightsId(std::addressof(out_rights_id->id), path.str, attr));
                    out_rights_id->key_generation = 0;
                }
                R_SUCCEED();
            }
        public:
            HostContentStorageImpl(RegisteredHostContent *registered_content) : m_registered_content(registered_content), m_disabled(false) { /* ... */ }
        public:
            /* Actual commands. */
            Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out);
            Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size);
            Result DeletePlaceHolder(PlaceHolderId placeholder_id);
            Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id);
            Result WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data);
            Result Register(PlaceHolderId placeholder_id, ContentId content_id);
            Result Delete(ContentId content_id);
            Result Has(sf::Out<bool> out, ContentId content_id);
            Result GetPath(sf::Out<Path> out, ContentId content_id);
            Result GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id);
            Result CleanupAllPlaceHolder();
            Result ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf);
            Result GetContentCount(sf::Out<s32> out_count);
            Result ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 start_offset);
            Result GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id);
            Result DisableForcibly();
            Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id);
            Result SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size);
            Result ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset);
            Result GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id);
            Result GetRightsIdFromPlaceHolderIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id);
            Result GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, fs::ContentAttributes attr);
            Result GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id);
            Result GetRightsIdFromContentIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id);
            Result GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id, fs::ContentAttributes attr);
            Result WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data);
            Result GetFreeSpaceSize(sf::Out<s64> out_size);
            Result GetTotalSpaceSize(sf::Out<s64> out_size);
            Result FlushPlaceHolder();
            Result GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id);
            Result RepairInvalidFileAttribute();
            Result GetRightsIdFromPlaceHolderIdWithCacheDeprecated(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id);
            Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id, fs::ContentAttributes attr);
            Result RegisterPath(const ContentId &content_id, const Path &path);
            Result ClearRegisteredPath();
            Result GetProgramId(sf::Out<ncm::ProgramId> out, ContentId content_id, fs::ContentAttributes attr);

            /* 16.0.0 Alignment change hacks. */
            Result CreatePlaceHolder_AtmosphereAlignmentFix(ContentId content_id, PlaceHolderId placeholder_id, s64 size) { R_RETURN(this->CreatePlaceHolder(placeholder_id, content_id, size)); }
            Result Register_AtmosphereAlignmentFix(ContentId content_id, PlaceHolderId placeholder_id) { R_RETURN(this->Register(placeholder_id, content_id)); }
            Result RevertToPlaceHolder_AtmosphereAlignmentFix(ncm::ContentId old_content_id, ncm::ContentId new_content_id, ncm::PlaceHolderId placeholder_id) { R_RETURN(this->RevertToPlaceHolder(placeholder_id, old_content_id, new_content_id)); }
            Result GetRightsIdFromPlaceHolderIdWithCacheDeprecated(sf::Out<ncm::RightsId> out_rights_id, ContentId cache_content_id, PlaceHolderId placeholder_id) { R_RETURN(this->GetRightsIdFromPlaceHolderIdWithCache(out_rights_id, placeholder_id, cache_content_id, fs::ContentAttributes_None)); }
    };
    static_assert(ncm::IsIContentStorage<HostContentStorageImpl>);

}
