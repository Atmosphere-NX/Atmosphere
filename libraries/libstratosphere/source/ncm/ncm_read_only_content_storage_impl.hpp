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
#include "ncm_content_storage_impl_base.hpp"

namespace ams::ncm {

    class ReadOnlyContentStorageImpl : public ContentStorageImplBase {
        public:
            Result Initialize(const char *root_path, MakeContentPathFunction content_path_func);
        public:
            /* Actual commands. */
            virtual Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) override;
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) override;
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id) override;
            virtual Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) override;
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data) override;
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id) override;
            virtual Result Delete(ContentId content_id) override;
            virtual Result Has(sf::Out<bool> out, ContentId content_id) override;
            virtual Result GetPath(sf::Out<Path> out, ContentId content_id) override;
            virtual Result GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) override;
            virtual Result CleanupAllPlaceHolder() override;
            virtual Result ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) override;
            virtual Result GetContentCount(sf::Out<s32> out_count) override;
            virtual Result ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 start_offset) override;
            virtual Result GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) override;
            virtual Result DisableForcibly() override;
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) override;
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) override;
            virtual Result ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset) override;
            virtual Result GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) override;
            virtual Result GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) override;
            virtual Result GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) override;
            virtual Result GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) override;
            virtual Result WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data) override;
            virtual Result GetFreeSpaceSize(sf::Out<s64> out_size) override;
            virtual Result GetTotalSpaceSize(sf::Out<s64> out_size) override;
            virtual Result FlushPlaceHolder() override;
            virtual Result GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id) override;
            virtual Result RepairInvalidFileAttribute() override;
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) override;
            virtual Result RegisterPath(const ContentId &content_id, const Path &path) override;
            virtual Result ClearRegisteredPath() override;
    };

}
