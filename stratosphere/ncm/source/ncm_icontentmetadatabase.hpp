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
#include <stratosphere/kvdb/kvdb_memory_key_value_store.hpp>

namespace sts::ncm {

    class IContentMetaDatabase : public IServiceObject {
        protected:
            enum class CommandId {
                Set = 0,
                Get = 1,
                Remove = 2,
                GetContentIdByType = 3,
                ListContentInfo = 4,
                List = 5,
                GetLatestContentMetaKey = 6,
                ListApplication = 7,
                Has = 8,
                HasAll = 9,
                GetSize = 10,
                GetRequiredSystemVersion = 11,
                GetPatchId = 12,
                DisableForcibly = 13,
                LookupOrphanContent = 14,
                Commit = 15,
                HasContent = 16,
                ListContentMetaInfo = 17,
                GetAttributes = 18,
                GetRequiredApplicationVersion = 19,
                GetContentIdByTypeAndIdOffset = 20,
            };
        protected:
            sts::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs;
            char mount_name[16];
            bool disabled;
        protected:
            Result EnsureEnabled();
        public:
            IContentMetaDatabase(sts::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs) : 
                kvs(kvs), disabled(false)
            {
            }

            IContentMetaDatabase(sts::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs, const char* mount_name) : 
                IContentMetaDatabase(kvs)
            {
                strcpy(this->mount_name, mount_name);
            }
        public:
            /* Actual commands. */
            virtual Result Set(ContentMetaKey key, InBuffer<u8> value);
            virtual Result Get(Out<u64> out_size, ContentMetaKey key, OutBuffer<u8> out_value);
            virtual Result Remove(ContentMetaKey key);
            virtual Result GetContentIdByType(Out<ContentId> out_content_id, ContentMetaKey key, ContentType type);
            virtual Result ListContentInfo(Out<u32> out_entries_written, OutBuffer<ContentInfo> out_info, ContentMetaKey key, u32 start_index);
            virtual Result List(Out<u32> out_entries_total, Out<u32> out_entries_written, OutBuffer<ContentMetaKey> out_info, ContentMetaType meta_type, TitleId application_title_id, TitleId title_id_min, TitleId title_id_max, ContentInstallType install_type);
            virtual Result GetLatestContentMetaKey(Out<ContentMetaKey> out_key, TitleId tid);
            virtual Result ListApplication(Out<u32> out_entries_total, Out<u32> out_entries_written, OutBuffer<ApplicationContentMetaKey> out_keys, ContentMetaType meta_type);
            virtual Result Has(Out<bool> out, ContentMetaKey key);
            virtual Result HasAll(Out<bool> out, InBuffer<ContentMetaKey> keys);
            virtual Result GetSize(Out<u64> out_size, ContentMetaKey key);
            virtual Result GetRequiredSystemVersion(Out<u32> out_version, ContentMetaKey key);
            virtual Result GetPatchId(Out<TitleId> out_patch_id, ContentMetaKey key);
            virtual Result DisableForcibly();
            virtual Result LookupOrphanContent(OutBuffer<bool> out_orphaned, InBuffer<ContentId> content_ids);
            virtual Result Commit();
            virtual Result HasContent(Out<bool> out, ContentMetaKey key, ContentId content_id);
            virtual Result ListContentMetaInfo(Out<u32> out_entries_written, OutBuffer<ContentMetaInfo> out_meta_info, ContentMetaKey key, u32 start_index);
            virtual Result GetAttributes(Out<ContentMetaAttribute> out_attributes, ContentMetaKey key);
            virtual Result GetRequiredApplicationVersion(Out<u32> out_version, ContentMetaKey key);
            virtual Result GetContentIdByTypeAndIdOffset(Out<ContentId> out_content_id, ContentMetaKey key, ContentType type, u8 id_offset);

            /* APIs. */
            virtual Result GetLatestProgram(ContentId* out_content_id, TitleId title_id);
            virtual Result GetLatestData(ContentId* out_content_id, TitleId title_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, Set),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, Get),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, Remove),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, GetContentIdByType),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, ListContentInfo),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, List),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, GetLatestContentMetaKey),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, ListApplication),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, Has),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, HasAll),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, GetSize),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, GetRequiredSystemVersion),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, GetPatchId),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, DisableForcibly),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, LookupOrphanContent),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, Commit),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, HasContent),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, ListContentMetaInfo),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, GetAttributes),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, GetRequiredApplicationVersion, FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentMetaDatabase, GetContentIdByTypeAndIdOffset, FirmwareVersion_500),
            };
    };

}
