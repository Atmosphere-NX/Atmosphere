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

    class ContentStorageImplBase {
        NON_COPYABLE(ContentStorageImplBase);
        NON_MOVEABLE(ContentStorageImplBase);
        protected:
            PathString m_root_path;
            MakeContentPathFunction m_make_content_path_func;
            bool m_disabled;
        protected:
            ContentStorageImplBase() : m_root_path(), m_make_content_path_func(), m_disabled(false) { /* ... */ }
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
            /* Actual commands. */
            virtual Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) = 0;
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) = 0;
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id) = 0;
            virtual Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) = 0;
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data) = 0;
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id) = 0;
            virtual Result Delete(ContentId content_id) = 0;
            virtual Result Has(sf::Out<bool> out, ContentId content_id) = 0;
            virtual Result GetPath(sf::Out<Path> out, ContentId content_id) = 0;
            virtual Result GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) = 0;
            virtual Result CleanupAllPlaceHolder() = 0;
            virtual Result ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) = 0;
            virtual Result GetContentCount(sf::Out<s32> out_count) = 0;
            virtual Result ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 start_offset) = 0;
            virtual Result GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) = 0;
            virtual Result DisableForcibly() = 0;
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) = 0;
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) = 0;
            virtual Result ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset) = 0;
            virtual Result GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) = 0;
            virtual Result GetRightsIdFromPlaceHolderIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) = 0;
            virtual Result GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, fs::ContentAttributes attr) = 0;
            virtual Result GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) = 0;
            virtual Result GetRightsIdFromContentIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) = 0;
            virtual Result GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id, fs::ContentAttributes attr) = 0;
            virtual Result WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data) = 0;
            virtual Result GetFreeSpaceSize(sf::Out<s64> out_size) = 0;
            virtual Result GetTotalSpaceSize(sf::Out<s64> out_size) = 0;
            virtual Result FlushPlaceHolder() = 0;
            virtual Result GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id) = 0;
            virtual Result RepairInvalidFileAttribute() = 0;
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id, fs::ContentAttributes attr) = 0;
            virtual Result RegisterPath(const ContentId &content_id, const Path &path) = 0;
            virtual Result ClearRegisteredPath() = 0;

            /* 16.0.0 Alignment change hacks. */
            Result CreatePlaceHolder_AtmosphereAlignmentFix(ContentId content_id, PlaceHolderId placeholder_id, s64 size) { R_RETURN(this->CreatePlaceHolder(placeholder_id, content_id, size)); }
            Result Register_AtmosphereAlignmentFix(ContentId content_id, PlaceHolderId placeholder_id) { R_RETURN(this->Register(placeholder_id, content_id)); }
            Result RevertToPlaceHolder_AtmosphereAlignmentFix(ncm::ContentId old_content_id, ncm::ContentId new_content_id, ncm::PlaceHolderId placeholder_id) { R_RETURN(this->RevertToPlaceHolder(placeholder_id, old_content_id, new_content_id)); }
            Result GetRightsIdFromPlaceHolderIdWithCacheDeprecated(sf::Out<ncm::RightsId> out_rights_id, ContentId cache_content_id, PlaceHolderId placeholder_id) { R_RETURN(this->GetRightsIdFromPlaceHolderIdWithCache(out_rights_id, placeholder_id, cache_content_id, fs::ContentAttributes_None)); }
    };
    static_assert(ncm::IsIContentStorage<ContentStorageImplBase>);

}
