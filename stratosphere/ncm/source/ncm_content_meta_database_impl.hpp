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
#include "ncm_content_meta_database_impl_base.hpp"

namespace ams::ncm {

    class ContentMetaDatabaseImpl : public ContentMetaDatabaseImplBase {
        public:
            ContentMetaDatabaseImpl(ContentMetaKeyValueStore *kvs, const char *mount_name) : ContentMetaDatabaseImplBase(kvs, mount_name) { /* ... */ }
            ContentMetaDatabaseImpl(ContentMetaKeyValueStore *kvs) : ContentMetaDatabaseImplBase(kvs) { /* ... */ }
        private:
            Result GetContentIdByTypeImpl(ContentId *out, const ContentMetaKey& key, ContentType type, std::optional<u8> id_offset);
            Result GetLatestContentMetaKeyImpl(ContentMetaKey *out_key, ProgramId id);
        public:
            virtual Result Set(const ContentMetaKey &key, sf::InBuffer value) override;
            virtual Result Get(sf::Out<u64> out_size, const ContentMetaKey &key, sf::OutBuffer out_value) override;
            virtual Result Remove(const ContentMetaKey &key) override;
            virtual Result GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type) override;
            virtual Result ListContentInfo(sf::Out<u32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, u32 start_index) override;
            virtual Result List(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType type, ProgramId application_program_id, ProgramId program_id_min, ProgramId program_id_max, ContentInstallType install_type) override;
            virtual Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId id) override;
            virtual Result ListApplication(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType type) override;
            virtual Result Has(sf::Out<bool> out, const ContentMetaKey &key) override;
            virtual Result HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) override;
            virtual Result GetSize(sf::Out<u64> out_size, const ContentMetaKey &key) override;
            virtual Result GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key) override;
            virtual Result GetPatchId(sf::Out<ProgramId> out_patch_id, const ContentMetaKey &key) override;
            virtual Result DisableForcibly() override;
            virtual Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) override;
            virtual Result Commit() override;
            virtual Result HasContent(sf::Out<bool> out, const ContentMetaKey &key, ContentId content_id) override;
            virtual Result ListContentMetaInfo(sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, u32 start_index) override;
            virtual Result GetAttributes(sf::Out<ContentMetaAttribute> out_attributes, const ContentMetaKey &key) override;
            virtual Result GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key) override;
            virtual Result GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) override;
    };

}
