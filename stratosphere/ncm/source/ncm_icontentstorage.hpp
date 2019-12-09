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

namespace ams::ncm {
    
    class IContentStorage : public sf::IServiceObject {
        protected:
            enum class CommandId {
                GeneratePlaceHolderId = 0,
                CreatePlaceHolder = 1,
                DeletePlaceHolder = 2,
                HasPlaceHolder = 3,
                WritePlaceHolder = 4,
                Register = 5,
                Delete = 6,
                Has = 7,
                GetPath = 8,
                GetPlaceHolderPath = 9,
                CleanupAllPlaceHolder = 10,
                ListPlaceHolder = 11,
                GetContentCount = 12,
                ListContentId = 13,
                GetSizeFromContentId = 14,
                DisableForcibly = 15,
                RevertToPlaceHolder = 16,
                SetPlaceHolderSize = 17,
                ReadContentIdFile = 18,
                GetRightsIdFromPlaceHolderId = 19,
                GetRightsIdFromContentId = 20,
                WriteContentForDebug = 21,
                GetFreeSpaceSize = 22,
                GetTotalSpaceSize = 23,
                FlushPlaceHolder = 24,
                GetSizeFromPlaceHolderId = 25,
                RepairInvalidFileAttribute = 26,
                GetRightsIdFromPlaceHolderIdWithCache = 27,
            };
        protected:
            char root_path[FS_MAX_PATH-1];
            MakeContentPathFunc make_content_path_func;
            bool disabled;
        protected:
            Result EnsureEnabled() {
                if (this->disabled) {
                    return ResultInvalidContentStorage();
                }
                return ResultSuccess();
            }
        public:
            virtual Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out);
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size);
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id);
            virtual Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id);
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, sf::InBuffer data);
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id);
            virtual Result Delete(ContentId content_id);
            virtual Result Has(sf::Out<bool> out, ContentId content_id);
            virtual Result GetPath(sf::Out<lr::Path> out, ContentId content_id);
            virtual Result GetPlaceHolderPath(sf::Out<lr::Path> out, PlaceHolderId placeholder_id);
            virtual Result CleanupAllPlaceHolder();
            virtual Result ListPlaceHolder(sf::Out<u32> out_count, const sf::OutArray<PlaceHolderId> &out_buf);
            virtual Result GetContentCount(sf::Out<u32> out_count);
            virtual Result ListContentId(sf::Out<u32> out_count, const sf::OutArray<ContentId> &out_buf, u32 start_offset);
            virtual Result GetSizeFromContentId(sf::Out<u64> out_size, ContentId content_id);
            virtual Result DisableForcibly();
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id);
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size);
            virtual Result ReadContentIdFile(sf::OutBuffer buf, ContentId content_id, u64 offset);
            virtual Result GetRightsIdFromPlaceHolderId(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, PlaceHolderId placeholder_id);
            virtual Result GetRightsIdFromContentId(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, ContentId content_id);
            virtual Result WriteContentForDebug(ContentId content_id, u64 offset, sf::InBuffer data);
            virtual Result GetFreeSpaceSize(sf::Out<u64> out_size);
            virtual Result GetTotalSpaceSize(sf::Out<u64> out_size);
            virtual Result FlushPlaceHolder();
            virtual Result GetSizeFromPlaceHolderId(sf::Out<u64> out, PlaceHolderId placeholder_id);
            virtual Result RepairInvalidFileAttribute();
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<FsRightsId> out_rights_id, sf::Out<u64> out_key_generation, PlaceHolderId placeholder_id, ContentId cache_content_id);
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
                MAKE_SERVICE_COMMAND_META(RevertToPlaceHolder,                   hos::Version_200),
                MAKE_SERVICE_COMMAND_META(SetPlaceHolderSize,                    hos::Version_200),
                MAKE_SERVICE_COMMAND_META(ReadContentIdFile,                     hos::Version_200),
                MAKE_SERVICE_COMMAND_META(GetRightsIdFromPlaceHolderId,          hos::Version_200),
                MAKE_SERVICE_COMMAND_META(GetRightsIdFromContentId,              hos::Version_200),
                MAKE_SERVICE_COMMAND_META(WriteContentForDebug,                  hos::Version_200),
                MAKE_SERVICE_COMMAND_META(GetFreeSpaceSize,                      hos::Version_200),
                MAKE_SERVICE_COMMAND_META(GetTotalSpaceSize,                     hos::Version_200),
                MAKE_SERVICE_COMMAND_META(FlushPlaceHolder,                      hos::Version_300),
                MAKE_SERVICE_COMMAND_META(GetSizeFromPlaceHolderId,              hos::Version_400),
                MAKE_SERVICE_COMMAND_META(RepairInvalidFileAttribute,            hos::Version_400),
                MAKE_SERVICE_COMMAND_META(GetRightsIdFromPlaceHolderIdWithCache, hos::Version_800),
            };
    };

}
