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
#include <stratosphere/sf.hpp>
#include <stratosphere/ncm/ncm_content_meta.hpp>

namespace ams::ncm {

    class IContentMetaDatabase : public sf::IServiceObject {
        protected:
            enum class CommandId {
                Set                              = 0,
                Get                              = 1,
                Remove                           = 2,
                GetContentIdByType               = 3,
                ListContentInfo                  = 4,
                List                             = 5,
                GetLatestContentMetaKey          = 6,
                ListApplication                  = 7,
                Has                              = 8,
                HasAll                           = 9,
                GetSize                          = 10,
                GetRequiredSystemVersion         = 11,
                GetPatchId                       = 12,
                DisableForcibly                  = 13,
                LookupOrphanContent              = 14,
                Commit                           = 15,
                HasContent                       = 16,
                ListContentMetaInfo              = 17,
                GetAttributes                    = 18,
                GetRequiredApplicationVersion    = 19,
                GetContentIdByTypeAndIdOffset    = 20,
                GetCount                         = 21,
                GetOwnerApplicationId            = 22,
            };
        public:
            /* Actual commands. */
            virtual Result Set(const ContentMetaKey &key, sf::InBuffer value) = 0;
            virtual Result Get(sf::Out<u64> out_size, const ContentMetaKey &key, sf::OutBuffer out_value) = 0;
            virtual Result Remove(const ContentMetaKey &key) = 0;
            virtual Result GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type) = 0;
            virtual Result ListContentInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, s32 offset) = 0;
            virtual Result List(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type) = 0;
            virtual Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, u64 id) = 0;
            virtual Result ListApplication(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType meta_type) = 0;
            virtual Result Has(sf::Out<bool> out, const ContentMetaKey &key) = 0;
            virtual Result HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) = 0;
            virtual Result GetSize(sf::Out<u64> out_size, const ContentMetaKey &key) = 0;
            virtual Result GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key) = 0;
            virtual Result GetPatchId(sf::Out<PatchId> out_patch_id, const ContentMetaKey &key) = 0;
            virtual Result DisableForcibly() = 0;
            virtual Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) = 0;
            virtual Result Commit() = 0;
            virtual Result HasContent(sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id) = 0;
            virtual Result ListContentMetaInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, s32 offset) = 0;
            virtual Result GetAttributes(sf::Out<u8> out_attributes, const ContentMetaKey &key) = 0;
            virtual Result GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key) = 0;
            virtual Result GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) = 0;
            virtual Result GetCount(sf::Out<u32> out_count) = 0;
            virtual Result GetOwnerApplicationId(sf::Out<ApplicationId> out_id, const ContentMetaKey &key) = 0;
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
                MAKE_SERVICE_COMMAND_META(GetRequiredApplicationVersion, hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(GetContentIdByTypeAndIdOffset, hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(GetCount,                      hos::Version_10_0_0),
                MAKE_SERVICE_COMMAND_META(GetOwnerApplicationId,         hos::Version_10_0_0),
            };
    };

}
