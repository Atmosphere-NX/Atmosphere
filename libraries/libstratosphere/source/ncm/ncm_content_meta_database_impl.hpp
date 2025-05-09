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
#include "ncm_content_meta_database_impl_base.hpp"

namespace ams::ncm {

    class ContentMetaDatabaseImpl : public ContentMetaDatabaseImplBase {
        public:
            ContentMetaDatabaseImpl(ContentMetaKeyValueStore *kvs, const char *mount_name) : ContentMetaDatabaseImplBase(kvs, mount_name) { /* ... */ }
            ContentMetaDatabaseImpl(ContentMetaKeyValueStore *kvs) : ContentMetaDatabaseImplBase(kvs) { /* ... */ }
        private:
            /* Helpers. */
            Result GetContentInfoImpl(ContentInfo *out, const ContentMetaKey &key, ContentType type, util::optional<u8> id_offset) const;

            Result GetContentIdImpl(ContentId *out, const ContentMetaKey &key, ContentType type, util::optional<u8> id_offset) const {
                /* Get the content info. */
                ContentInfo content_info;
                R_TRY(this->GetContentInfoImpl(std::addressof(content_info), key, type, id_offset));

                /* Set the output id. */
                *out = content_info.GetId();
                R_SUCCEED();
            }
        public:
            /* Actual commands. */
            virtual Result Set(const ContentMetaKey &key, const sf::InBuffer &value) override;
            virtual Result Get(sf::Out<u64> out_size, const ContentMetaKey &key, const sf::OutBuffer &out_value) override;
            virtual Result Remove(const ContentMetaKey &key) override;
            virtual Result GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type) override;
            virtual Result ListContentInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, s32 offset) override;
            virtual Result List(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type) override;
            virtual Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, u64 id) override;
            virtual Result ListApplication(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType meta_type) override;
            virtual Result Has(sf::Out<bool> out, const ContentMetaKey &key) override;
            virtual Result HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) override;
            virtual Result GetSize(sf::Out<u64> out_size, const ContentMetaKey &key) override;
            virtual Result GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key) override;
            virtual Result GetPatchContentMetaId(sf::Out<u64> out_patch_id, const ContentMetaKey &key) override;
            virtual Result DisableForcibly() override;
            virtual Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) override;
            virtual Result Commit() override;
            virtual Result HasContent(sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id) override;
            virtual Result ListContentMetaInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, s32 offset) override;
            virtual Result GetAttributes(sf::Out<u8> out_attributes, const ContentMetaKey &key) override;
            virtual Result GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key) override;
            virtual Result GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) override;
            virtual Result GetCount(sf::Out<u32> out_count) override;
            virtual Result GetOwnerApplicationId(sf::Out<ApplicationId> out_id, const ContentMetaKey &key) override;
            virtual Result GetContentAccessibilities(sf::Out<u8> out_accessibilities, const ContentMetaKey &key) override;
            virtual Result GetContentInfoByType(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type) override;
            virtual Result GetContentInfoByTypeAndIdOffset(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type, u8 id_offset) override;
            virtual Result GetPlatform(sf::Out<ncm::ContentMetaPlatform> out, const ContentMetaKey &key) override;
            virtual Result HasAttributes(sf::Out<u8> out, u8 attr_mask) override;
    };

}
