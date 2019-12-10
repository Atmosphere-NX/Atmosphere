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

namespace ams::ncm {

    class IContentMetaDatabase : public sf::IServiceObject {
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
            ams::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs;
            char mount_name[16];
            bool disabled;
        protected:
            Result EnsureEnabled() {
                if (this->disabled) {
                    return ResultInvalidContentMetaDatabase();
                }
                return ResultSuccess();
            }
        public:
            IContentMetaDatabase(ams::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs) : 
                kvs(kvs), disabled(false)
            {
            }

            IContentMetaDatabase(ams::kvdb::MemoryKeyValueStore<ContentMetaKey>* kvs, const char* mount_name) : 
                IContentMetaDatabase(kvs)
            {
                strcpy(this->mount_name, mount_name);
            }
        public:
            /* Actual commands. */
            virtual Result Set(ContentMetaKey key, sf::InBuffer value);
            virtual Result Get(sf::Out<u64> out_size, ContentMetaKey key, sf::OutBuffer out_value);
            virtual Result Remove(ContentMetaKey key);
            virtual Result GetContentIdByType(sf::Out<ContentId> out_content_id, ContentMetaKey key, ContentType type);
            virtual Result ListContentInfo(sf::Out<u32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, ContentMetaKey key, u32 start_index);
            virtual Result List(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ProgramId application_title_id, ProgramId title_id_min, ProgramId title_id_max, ContentInstallType install_type);
            virtual Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId tid);
            virtual Result ListApplication(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType meta_type);
            virtual Result Has(sf::Out<bool> out, ContentMetaKey key);
            virtual Result HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys);
            virtual Result GetSize(sf::Out<u64> out_size, ContentMetaKey key);
            virtual Result GetRequiredSystemVersion(sf::Out<u32> out_version, ContentMetaKey key);
            virtual Result GetPatchId(sf::Out<ProgramId> out_patch_id, ContentMetaKey key);
            virtual Result DisableForcibly();
            virtual Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids);
            virtual Result Commit();
            virtual Result HasContent(sf::Out<bool> out, ContentMetaKey key, ContentId content_id);
            virtual Result ListContentMetaInfo(sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, ContentMetaKey key, u32 start_index);
            virtual Result GetAttributes(sf::Out<ContentMetaAttribute> out_attributes, ContentMetaKey key);
            virtual Result GetRequiredApplicationVersion(sf::Out<u32> out_version, ContentMetaKey key);
            virtual Result GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, ContentMetaKey key, ContentType type, u8 id_offset);

            /* APIs. */
            virtual Result GetLatestProgram(ContentId* out_content_id, ProgramId title_id);
            virtual Result GetLatestData(ContentId* out_content_id, ProgramId title_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(Set),
                MAKE_SERVICE_COMMAND_META(Get),
                MAKE_SERVICE_COMMAND_META(Remove),
                MAKE_SERVICE_COMMAND_META(GetContentIdByType),
                MAKE_SERVICE_COMMAND_META(ListContentInfo),
                MAKE_SERVICE_COMMAND_META(List),
                MAKE_SERVICE_COMMAND_META(GetLatestContentMetaKey),
                MAKE_SERVICE_COMMAND_META(ListApplication),
                MAKE_SERVICE_COMMAND_META(Has),
                MAKE_SERVICE_COMMAND_META(HasAll),
                MAKE_SERVICE_COMMAND_META(GetSize),
                MAKE_SERVICE_COMMAND_META(GetRequiredSystemVersion),
                MAKE_SERVICE_COMMAND_META(GetPatchId),
                MAKE_SERVICE_COMMAND_META(DisableForcibly),
                MAKE_SERVICE_COMMAND_META(LookupOrphanContent),
                MAKE_SERVICE_COMMAND_META(Commit),
                MAKE_SERVICE_COMMAND_META(HasContent),
                MAKE_SERVICE_COMMAND_META(ListContentMetaInfo),
                MAKE_SERVICE_COMMAND_META(GetAttributes),
                MAKE_SERVICE_COMMAND_META(GetRequiredApplicationVersion, hos::Version_200),
                MAKE_SERVICE_COMMAND_META(GetContentIdByTypeAndIdOffset, hos::Version_500),
            };
    };

}
