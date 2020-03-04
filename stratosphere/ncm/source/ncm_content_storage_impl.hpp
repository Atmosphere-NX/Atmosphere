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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "impl/ncm_placeholder_accessor.hpp"
#include "impl/ncm_rights_cache.hpp"
#include "ncm_content_storage_impl_base.hpp"
#include "ncm_path_utils.hpp"

namespace ams::ncm {

    class ContentStorageImpl : public ContentStorageImplBase {
        protected:
            impl::PlaceHolderAccessor placeholder_accessor;
            ContentId cached_content_id;
            FILE *content_cache_file_handle;
            impl::RightsIdCache *rights_id_cache;
        public:
            ~ContentStorageImpl();

            Result Initialize(const char *root_path, MakeContentPathFunc content_path_func, MakePlaceHolderPathFunc placeholder_path_func, bool delay_flush, impl::RightsIdCache *rights_id_cache);
            void Finalize();
        private:
            void ClearContentCache();
            unsigned int GetContentDirectoryDepth();
            Result OpenCachedContentFile(ContentId content_id);

            inline void GetContentRootPath(PathString *content_root) {
                path::GetContentRootPath(content_root, this->root_path);
            }

            inline void GetContentPath(PathString *content_path, ContentId content_id) {
                PathString root_path;
                this->GetContentRootPath(std::addressof(root_path));
                this->make_content_path_func(content_path, content_id, root_path);
            }
        public:
            virtual Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) override;
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) override;
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id) override;
            virtual Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) override;
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, sf::InBuffer data) override;
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id) override;
            virtual Result Delete(ContentId content_id) override;
            virtual Result Has(sf::Out<bool> out, ContentId content_id) override;
            virtual Result GetPath(sf::Out<Path> out, ContentId content_id) override;
            virtual Result GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) override;
            virtual Result CleanupAllPlaceHolder() override;
            virtual Result ListPlaceHolder(sf::Out<u32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) override;
            virtual Result GetContentCount(sf::Out<u32> out_count) override;
            virtual Result ListContentId(sf::Out<u32> out_count, const sf::OutArray<ContentId> &out_buf, u32 start_offset) override;
            virtual Result GetSizeFromContentId(sf::Out<u64> out_size, ContentId content_id) override;
            virtual Result DisableForcibly() override;
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) override;
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) override;
            virtual Result ReadContentIdFile(sf::OutBuffer buf, ContentId content_id, u64 offset) override;
            virtual Result GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) override;
            virtual Result GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) override;
            virtual Result GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) override;
            virtual Result GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) override;
            virtual Result WriteContentForDebug(ContentId content_id, u64 offset, sf::InBuffer data) override;
            virtual Result GetFreeSpaceSize(sf::Out<u64> out_size) override;
            virtual Result GetTotalSpaceSize(sf::Out<u64> out_size) override;
            virtual Result FlushPlaceHolder() override;
            virtual Result GetSizeFromPlaceHolderId(sf::Out<u64> out, PlaceHolderId placeholder_id) override;
            virtual Result RepairInvalidFileAttribute() override;
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) override;
    };

}
