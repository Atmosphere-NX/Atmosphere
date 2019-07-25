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

namespace sts::ncm {

    class ContentMetaDatabaseInterface : public IContentMetaDatabase {
        public:
            ContentMetaDatabaseInterface(sts::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs, const char* mount_name) : IContentMetaDatabase(kvs, mount_name) {
            }
            ContentMetaDatabaseInterface(sts::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs) : IContentMetaDatabase(kvs) {
            }
        private:
            Result GetContentIdByTypeImpl(ContentId* out, const ContentMetaKey& key, ContentType type, std::optional<u8> id_offset);
            Result GetLatestContentMetaKeyImpl(ContentMetaKey* out_key, TitleId tid);
        public:
            virtual Result Set(ContentMetaKey key, InBuffer<u8> value) override;
            virtual Result Get(Out<u64> out_size, ContentMetaKey key, OutBuffer<u8> out_value) override;
            virtual Result Remove(ContentMetaKey key) override;
            virtual Result GetContentIdByType(Out<ContentId> out_content_id, ContentMetaKey key, ContentType type) override;
            virtual Result ListContentInfo(Out<u32> out_entries_written, OutBuffer<ContentInfo> out_info, ContentMetaKey key, u32 start_index) override;
            virtual Result List(Out<u32> out_entries_total, Out<u32> out_entries_written, OutBuffer<ContentMetaKey> out_info, ContentMetaType type, TitleId application_title_id, TitleId title_id_min, TitleId title_id_max, ContentInstallType install_type) override;
            virtual Result GetLatestContentMetaKey(Out<ContentMetaKey> out_key, TitleId tid) override;
            virtual Result ListApplication(Out<u32> out_entries_total, Out<u32> out_entries_written, OutBuffer<ApplicationContentMetaKey> out_keys, ContentMetaType type) override;
            virtual Result Has(Out<bool> out, ContentMetaKey key) override;
            virtual Result HasAll(Out<bool> out, InBuffer<ContentMetaKey> keys) override;
            virtual Result GetSize(Out<u64> out_size, ContentMetaKey key) override;
            virtual Result GetRequiredSystemVersion(Out<u32> out_version, ContentMetaKey key) override;
            virtual Result GetPatchId(Out<TitleId> out_patch_id, ContentMetaKey key) override;
            virtual Result DisableForcibly() override;
            virtual Result LookupOrphanContent(OutBuffer<bool> out_orphaned, InBuffer<ContentId> content_ids) override;
            virtual Result Commit() override;
            virtual Result HasContent(Out<bool> out, ContentMetaKey key, ContentId content_id) override;
            virtual Result ListContentMetaInfo(Out<u32> out_entries_written, OutBuffer<ContentMetaInfo> out_meta_info, ContentMetaKey key, u32 start_index) override;
            virtual Result GetAttributes(Out<ContentMetaAttribute> out_attributes, ContentMetaKey key) override;
            virtual Result GetRequiredApplicationVersion(Out<u32> out_version, ContentMetaKey key) override;
            virtual Result GetContentIdByTypeAndIdOffset(Out<ContentId> out_content_id, ContentMetaKey key, ContentType type, u8 id_offset) override;
        
            /* APIs. */
            virtual Result GetLatestProgram(ContentId* out_content_id, TitleId title_id) override;
            virtual Result GetLatestData(ContentId* out_content_id, TitleId title_id) override;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, Set),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, Get),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, Remove),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, GetContentIdByType),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, ListContentInfo),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, List),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, GetLatestContentMetaKey),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, ListApplication),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, Has),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, HasAll),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, GetSize),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, GetRequiredSystemVersion),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, GetPatchId),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, DisableForcibly),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, LookupOrphanContent),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, Commit),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, HasContent),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, ListContentMetaInfo),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, GetAttributes),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, GetRequiredApplicationVersion, FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentMetaDatabaseInterface, GetContentIdByTypeAndIdOffset, FirmwareVersion_500),
            };
    };

    class OnMemoryContentMetaDatabaseInterface : public ContentMetaDatabaseInterface {
        public:
            OnMemoryContentMetaDatabaseInterface(sts::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs) : ContentMetaDatabaseInterface(kvs) {
            }
        public:
            virtual Result GetLatestContentMetaKey(Out<ContentMetaKey> out_key, TitleId tid) override;
            virtual Result LookupOrphanContent(OutBuffer<bool> out_orphaned, InBuffer<ContentId> content_ids) override;
            virtual Result Commit() override;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, Set),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, Get),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, Remove),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, GetContentIdByType),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, ListContentInfo),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, List),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, GetLatestContentMetaKey),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, ListApplication),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, Has),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, HasAll),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, GetSize),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, GetRequiredSystemVersion),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, GetPatchId),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, DisableForcibly),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, LookupOrphanContent),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, Commit),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, HasContent),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, ListContentMetaInfo),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, GetAttributes),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, GetRequiredApplicationVersion, FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(OnMemoryContentMetaDatabaseInterface, GetContentIdByTypeAndIdOffset, FirmwareVersion_500),
            };
    };

}