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

#include "lr_types.hpp"
#include "ncm_types.hpp"

namespace sts::ncm {
    
    class IContentStorage : public IServiceObject {
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
        public:
            virtual Result GeneratePlaceHolderId(Out<PlaceHolderId> out);
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size);
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id);
            virtual Result HasPlaceHolder(Out<bool> out, PlaceHolderId placeholder_id);
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, InBuffer<u8> data);
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id);
            virtual Result Delete(ContentId content_id);
            virtual Result Has(Out<bool> out, ContentId content_id);
            virtual Result GetPath(OutPointerWithServerSize<lr::Path, 0x1> out, ContentId content_id);
            virtual Result GetPlaceHolderPath(OutPointerWithServerSize<lr::Path, 0x1> out, PlaceHolderId placeholder_id);
            virtual Result CleanupAllPlaceHolder();
            virtual Result ListPlaceHolder(Out<u32> out_count, OutBuffer<PlaceHolderId> out_buf);
            virtual Result GetContentCount(Out<u32> out_count);
            virtual Result ListContentId(Out<u32> out_count, OutBuffer<ContentId> out_buf, u32 start_offset);
            virtual Result GetSizeFromContentId(Out<u64> out_size, ContentId content_id);
            virtual Result DisableForcibly();
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id);
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size);
            virtual Result ReadContentIdFile(OutBuffer<u8> buf, ContentId content_id, u64 offset);
            virtual Result GetRightsIdFromPlaceHolderId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id);
            virtual Result GetRightsIdFromContentId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, ContentId content_id);
            virtual Result WriteContentForDebug(ContentId content_id, u64 offset, InBuffer<u8> data);
            virtual Result GetFreeSpaceSize(Out<u64> out_size);
            virtual Result GetTotalSpaceSize(Out<u64> out_size);
            virtual Result FlushPlaceHolder();
            virtual Result GetSizeFromPlaceHolderId(Out<u64> out, PlaceHolderId placeholder_id);
            virtual Result RepairInvalidFileAttribute();
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id, ContentId cache_content_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(IContentStorage, GeneratePlaceHolderId),
                MAKE_SERVICE_COMMAND_META(IContentStorage, CreatePlaceHolder),
                MAKE_SERVICE_COMMAND_META(IContentStorage, DeletePlaceHolder),
                MAKE_SERVICE_COMMAND_META(IContentStorage, HasPlaceHolder),
                MAKE_SERVICE_COMMAND_META(IContentStorage, WritePlaceHolder),
                MAKE_SERVICE_COMMAND_META(IContentStorage, Register),
                MAKE_SERVICE_COMMAND_META(IContentStorage, Delete),
                MAKE_SERVICE_COMMAND_META(IContentStorage, Has),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetPath),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetPlaceHolderPath),
                MAKE_SERVICE_COMMAND_META(IContentStorage, CleanupAllPlaceHolder),
                MAKE_SERVICE_COMMAND_META(IContentStorage, ListPlaceHolder),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GeneratePlaceHolderId),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetContentCount),
                MAKE_SERVICE_COMMAND_META(IContentStorage, ListContentId),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetSizeFromContentId),
                MAKE_SERVICE_COMMAND_META(IContentStorage, DisableForcibly),
                MAKE_SERVICE_COMMAND_META(IContentStorage, RevertToPlaceHolder,                   FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentStorage, SetPlaceHolderSize,                    FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentStorage, ReadContentIdFile,                     FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetRightsIdFromPlaceHolderId,          FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetRightsIdFromContentId,              FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentStorage, WriteContentForDebug,                  FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetFreeSpaceSize,                      FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetTotalSpaceSize,                     FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(IContentStorage, FlushPlaceHolder,                      FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetSizeFromPlaceHolderId,              FirmwareVersion_400),
                MAKE_SERVICE_COMMAND_META(IContentStorage, RepairInvalidFileAttribute,            FirmwareVersion_400),
                MAKE_SERVICE_COMMAND_META(IContentStorage, GetRightsIdFromPlaceHolderIdWithCache, FirmwareVersion_800),
            };
    };

}