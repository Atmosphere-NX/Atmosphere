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
#include <stratosphere/ncm/ncm_content_id.hpp>
#include <stratosphere/ncm/ncm_placeholder_id.hpp>
#include <stratosphere/ncm/ncm_path.hpp>
#include <stratosphere/ncm/ncm_rights_id.hpp>

namespace ams::ncm {

    class IContentStorage : public sf::IServiceObject {
        NON_COPYABLE(IContentStorage);
        NON_MOVEABLE(IContentStorage);
        protected:
            enum class CommandId {
                GeneratePlaceHolderId                     = 0,
                CreatePlaceHolder                         = 1,
                DeletePlaceHolder                         = 2,
                HasPlaceHolder                            = 3,
                WritePlaceHolder                          = 4,
                Register                                  = 5,
                Delete                                    = 6,
                Has                                       = 7,
                GetPath                                   = 8,
                GetPlaceHolderPath                        = 9,
                CleanupAllPlaceHolder                     = 10,
                ListPlaceHolder                           = 11,
                GetContentCount                           = 12,
                ListContentId                             = 13,
                GetSizeFromContentId                      = 14,
                DisableForcibly                           = 15,
                RevertToPlaceHolder                       = 16,
                SetPlaceHolderSize                        = 17,
                ReadContentIdFile                         = 18,
                GetRightsIdFromPlaceHolderIdDeprecated    = 19,
                GetRightsIdFromPlaceHolderId              = 19,
                GetRightsIdFromContentIdDeprecated        = 20,
                GetRightsIdFromContentId                  = 20,
                WriteContentForDebug                      = 21,
                GetFreeSpaceSize                          = 22,
                GetTotalSpaceSize                         = 23,
                FlushPlaceHolder                          = 24,
                GetSizeFromPlaceHolderId                  = 25,
                RepairInvalidFileAttribute                = 26,
                GetRightsIdFromPlaceHolderIdWithCache     = 27,
            };
        public:
            IContentStorage() { /* ... */ }
        public:
            virtual Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) = 0;
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) = 0;
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id) = 0;
            virtual Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) = 0;
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, sf::InBuffer data) = 0;
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id) = 0;
            virtual Result Delete(ContentId content_id) = 0;
            virtual Result Has(sf::Out<bool> out, ContentId content_id) = 0;
            virtual Result GetPath(sf::Out<Path> out, ContentId content_id) = 0;
            virtual Result GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) = 0;
            virtual Result CleanupAllPlaceHolder() = 0;
            virtual Result ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) = 0;
            virtual Result GetContentCount(sf::Out<s32> out_count) = 0;
            virtual Result ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 start_offset) = 0;
            virtual Result GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) = 0;
            virtual Result DisableForcibly() = 0;
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) = 0;
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) = 0;
            virtual Result ReadContentIdFile(sf::OutBuffer buf, ContentId content_id, s64 offset) = 0;
            virtual Result GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) = 0;
            virtual Result GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) = 0;
            virtual Result GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) = 0;
            virtual Result GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) = 0;
            virtual Result WriteContentForDebug(ContentId content_id, s64 offset, sf::InBuffer data) = 0;
            virtual Result GetFreeSpaceSize(sf::Out<s64> out_size) = 0;
            virtual Result GetTotalSpaceSize(sf::Out<s64> out_size) = 0;
            virtual Result FlushPlaceHolder() = 0;
            virtual Result GetSizeFromPlaceHolderId(sf::Out<s64> out, PlaceHolderId placeholder_id) = 0;
            virtual Result RepairInvalidFileAttribute() = 0;
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GeneratePlaceHolderId),
                MAKE_SERVICE_COMMAND_META(CreatePlaceHolder),
                MAKE_SERVICE_COMMAND_META(DeletePlaceHolder),
                MAKE_SERVICE_COMMAND_META(HasPlaceHolder),
                MAKE_SERVICE_COMMAND_META(WritePlaceHolder),
                MAKE_SERVICE_COMMAND_META(Register),
                MAKE_SERVICE_COMMAND_META(Delete),
                MAKE_SERVICE_COMMAND_META(Has),
                MAKE_SERVICE_COMMAND_META(GetPath),
                MAKE_SERVICE_COMMAND_META(GetPlaceHolderPath),
                MAKE_SERVICE_COMMAND_META(CleanupAllPlaceHolder),
                MAKE_SERVICE_COMMAND_META(ListPlaceHolder),
                MAKE_SERVICE_COMMAND_META(GeneratePlaceHolderId),
                MAKE_SERVICE_COMMAND_META(GetContentCount),
                MAKE_SERVICE_COMMAND_META(ListContentId),
                MAKE_SERVICE_COMMAND_META(GetSizeFromContentId),
                MAKE_SERVICE_COMMAND_META(DisableForcibly),
                MAKE_SERVICE_COMMAND_META(RevertToPlaceHolder,                       hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(SetPlaceHolderSize,                        hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(ReadContentIdFile,                         hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(GetRightsIdFromPlaceHolderIdDeprecated,    hos::Version_2_0_0, hos::Version_2_3_0),
                MAKE_SERVICE_COMMAND_META(GetRightsIdFromPlaceHolderId,              hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(GetRightsIdFromContentIdDeprecated,        hos::Version_2_0_0, hos::Version_2_3_0),
                MAKE_SERVICE_COMMAND_META(GetRightsIdFromContentId,                  hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(WriteContentForDebug,                      hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(GetFreeSpaceSize,                          hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(GetTotalSpaceSize,                         hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(FlushPlaceHolder,                          hos::Version_3_0_0),
                MAKE_SERVICE_COMMAND_META(GetSizeFromPlaceHolderId,                  hos::Version_4_0_0),
                MAKE_SERVICE_COMMAND_META(RepairInvalidFileAttribute,                hos::Version_4_0_0),
                MAKE_SERVICE_COMMAND_META(GetRightsIdFromPlaceHolderIdWithCache,     hos::Version_8_0_0),
            };
    };

}
