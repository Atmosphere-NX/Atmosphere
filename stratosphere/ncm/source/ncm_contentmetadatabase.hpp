/*
 * Copyright (c) 2019 Adubbz
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

#include "ncm_icontentmetadatabase.hpp"

namespace ams::ncm {

    class ContentMetaDatabaseInterface : public IContentMetaDatabase {
        public:
            ContentMetaDatabaseInterface(ams::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs, const char* mount_name) : IContentMetaDatabase(kvs, mount_name) {
            }
            ContentMetaDatabaseInterface(ams::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs) : IContentMetaDatabase(kvs) {
            }
        private:
            Result GetContentIdByTypeImpl(ContentId* out, const ContentMetaKey& key, ContentType type, std::optional<u8> id_offset);
            Result GetLatestContentMetaKeyImpl(ContentMetaKey* out_key, ProgramId tid);
        public:
            virtual Result Set(ContentMetaKey key, sf::InBuffer value) override;
            virtual Result Get(sf::Out<u64> out_size, ContentMetaKey key, sf::OutBuffer out_value) override;
            virtual Result Remove(ContentMetaKey key) override;
            virtual Result GetContentIdByType(sf::Out<ContentId> out_content_id, ContentMetaKey key, ContentType type) override;
            virtual Result ListContentInfo(sf::Out<u32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, ContentMetaKey key, u32 start_index) override;
            virtual Result List(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType type, ProgramId application_title_id, ProgramId title_id_min, ProgramId title_id_max, ContentInstallType install_type) override;
            virtual Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId tid) override;
            virtual Result ListApplication(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType type) override;
            virtual Result Has(sf::Out<bool> out, ContentMetaKey key) override;
            virtual Result HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) override;
            virtual Result GetSize(sf::Out<u64> out_size, ContentMetaKey key) override;
            virtual Result GetRequiredSystemVersion(sf::Out<u32> out_version, ContentMetaKey key) override;
            virtual Result GetPatchId(sf::Out<ProgramId> out_patch_id, ContentMetaKey key) override;
            virtual Result DisableForcibly() override;
            virtual Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) override;
            virtual Result Commit() override;
            virtual Result HasContent(sf::Out<bool> out, ContentMetaKey key, ContentId content_id) override;
            virtual Result ListContentMetaInfo(sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, ContentMetaKey key, u32 start_index) override;
            virtual Result GetAttributes(sf::Out<ContentMetaAttribute> out_attributes, ContentMetaKey key) override;
            virtual Result GetRequiredApplicationVersion(sf::Out<u32> out_version, ContentMetaKey key) override;
            virtual Result GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, ContentMetaKey key, ContentType type, u8 id_offset) override;
        
            /* APIs. */
            virtual Result GetLatestProgram(ContentId* out_content_id, ProgramId title_id) override;
            virtual Result GetLatestData(ContentId* out_content_id, ProgramId title_id) override;
    };

    class OnMemoryContentMetaDatabaseInterface : public ContentMetaDatabaseInterface {
        public:
            OnMemoryContentMetaDatabaseInterface(ams::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs) : ContentMetaDatabaseInterface(kvs) {
            }
        public:
            virtual Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId tid) override;
            virtual Result LookupOrphanContent(const sf::OutArray<bool> & out_orphaned, const sf::InArray<ContentId> &content_ids) override;
            virtual Result Commit() override;
    };

}
