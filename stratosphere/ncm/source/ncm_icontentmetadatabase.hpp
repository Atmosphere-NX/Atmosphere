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

#include "ncm_types.hpp"

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
            virtual Result Set(ContentMetaKey key, InBuffer<u8> value) = 0;
            virtual Result Get(Out<u64> out_size, ContentMetaKey key, OutBuffer<u8> out_value) = 0;
            virtual Result Remove(ContentMetaKey key) = 0;
            virtual Result GetContentIdByType(Out<ContentId> out_content_id, ContentMetaKey key, ContentType type) = 0;
            virtual Result ListContentInfo(Out<u32> out_entries_written, OutBuffer<ContentInfo> out_info, ContentMetaKey key, u32 start_index) = 0;
            virtual Result List(Out<u32> out_entries_total, Out<u32> out_entries_written, OutBuffer<ContentMetaKey> out_info, ContentMetaType meta_type, TitleId application_title_id, TitleId title_id_min, TitleId title_id_max, ContentInstallType install_type) = 0;
            virtual Result GetLatestContentMetaKey(Out<ContentMetaKey> out_key, TitleId tid) = 0;
            virtual Result ListApplication(Out<u32> out_entries_total, Out<u32> out_entries_written, OutBuffer<ApplicationContentMetaKey> out_keys, ContentMetaType meta_type) = 0;
            virtual Result Has(Out<bool> out, ContentMetaKey key) = 0;
            virtual Result HasAll(Out<bool> out, InBuffer<ContentMetaKey> keys) = 0;
            virtual Result GetSize(Out<u64> out_size, ContentMetaKey key) = 0;
            virtual Result GetRequiredSystemVersion(Out<u32> out_version, ContentMetaKey key) = 0;
            virtual Result GetPatchId(Out<TitleId> out_patch_id, ContentMetaKey key) = 0;
            virtual Result DisableForcibly() = 0;
            virtual Result LookupOrphanContent(OutBuffer<bool> out_orphaned, InBuffer<ContentId> content_ids) = 0;
            virtual Result Commit() = 0;
            virtual Result HasContent(Out<bool> out, ContentMetaKey key, ContentId content_id) = 0;
            virtual Result ListContentMetaInfo(Out<u32> out_entries_written, OutBuffer<ContentMetaInfo> out_meta_info, ContentMetaKey key, u32 start_index) = 0;
            virtual Result GetAttributes(Out<ContentMetaAttribute> out_attributes, ContentMetaKey key) = 0;
            virtual Result GetRequiredApplicationVersion(Out<u32> out_version, ContentMetaKey key) = 0;
            virtual Result GetContentIdByTypeAndIdOffset(Out<ContentId> out_content_id, ContentMetaKey key, ContentType type, u8 id_offset) = 0;

            /* APIs. */
            virtual Result GetLatestProgram(ContentId* out_content_id, TitleId title_id) = 0;
            virtual Result GetLatestData(ContentId* out_content_id, TitleId title_id) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {};
    };

}