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

#include "impl/ncm_placeholder_accessor.hpp"
#include "ncm_icontentstorage.hpp"
#include "ncm_path_utils.hpp"

namespace sts::ncm {

    class ContentStorageInterface : public IContentStorage {
        protected:
            impl::PlaceHolderAccessor placeholder_accessor;
            ContentId cached_content_id;
            FILE* content_cache_file_handle;
        public:
            ~ContentStorageInterface();

            Result Initialize(const char* root_path, MakeContentPathFunc content_path_func, MakePlaceHolderPathFunc placeholder_path_func, bool delay_flush);
            void Finalize();
        private:
            void ClearContentCache();
            unsigned int GetContentDirectoryDepth();
            Result OpenCachedContentFile(ContentId content_id);

            inline void GetContentRootPath(char* out_content_root) {
                path::GetContentRootPath(out_content_root, this->root_path);
            }

            inline void GetContentPath(char* out_content_path, ContentId content_id) {
                char content_root_path[FS_MAX_PATH] = {0};

                this->GetContentRootPath(content_root_path);
                this->make_content_path_func(out_content_path, content_id, content_root_path);
            }
        public:
            virtual Result GeneratePlaceHolderId(Out<PlaceHolderId> out) override;
            virtual Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) override;
            virtual Result DeletePlaceHolder(PlaceHolderId placeholder_id) override;
            virtual Result HasPlaceHolder(Out<bool> out, PlaceHolderId placeholder_id) override;
            virtual Result WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, InBuffer<u8> data) override;
            virtual Result Register(PlaceHolderId placeholder_id, ContentId content_id) override;
            virtual Result Delete(ContentId content_id) override;
            virtual Result Has(Out<bool> out, ContentId content_id) override;
            virtual Result GetPath(OutPointerWithServerSize<lr::Path, 0x1> out, ContentId content_id) override;
            virtual Result GetPlaceHolderPath(OutPointerWithServerSize<lr::Path, 0x1> out, PlaceHolderId placeholder_id) override;
            virtual Result CleanupAllPlaceHolder() override;
            virtual Result ListPlaceHolder(Out<u32> out_count, OutBuffer<PlaceHolderId> out_buf) override;
            virtual Result GetContentCount(Out<u32> out_count) override;
            virtual Result ListContentId(Out<u32> out_count, OutBuffer<ContentId> out_buf, u32 start_offset) override;
            virtual Result GetSizeFromContentId(Out<u64> out_size, ContentId content_id) override;
            virtual Result DisableForcibly() override;
            virtual Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) override;
            virtual Result SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) override;
            virtual Result ReadContentIdFile(OutBuffer<u8> buf, ContentId content_id, u64 offset) override;
            virtual Result GetRightsIdFromPlaceHolderId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id) override;
            virtual Result GetRightsIdFromContentId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, ContentId content_id) override;
            virtual Result WriteContentForDebug(ContentId content_id, u64 offset, InBuffer<u8> data) override;
            virtual Result GetFreeSpaceSize(Out<u64> out_size) override;
            virtual Result GetTotalSpaceSize(Out<u64> out_size) override;
            virtual Result FlushPlaceHolder() override;
            virtual Result GetSizeFromPlaceHolderId(Out<u64> out, PlaceHolderId placeholder_id) override;
            virtual Result RepairInvalidFileAttribute() override;
            virtual Result GetRightsIdFromPlaceHolderIdWithCache(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id, ContentId cache_content_id) override;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GeneratePlaceHolderId),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, CreatePlaceHolder),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, DeletePlaceHolder),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, HasPlaceHolder),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, WritePlaceHolder),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, Register),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, Delete),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, Has),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetPath),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetPlaceHolderPath),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, CleanupAllPlaceHolder),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, ListPlaceHolder),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GeneratePlaceHolderId),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetContentCount),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, ListContentId),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetSizeFromContentId),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, DisableForcibly),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, RevertToPlaceHolder,                   FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, SetPlaceHolderSize,                    FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, ReadContentIdFile,                     FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetRightsIdFromPlaceHolderId,          FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetRightsIdFromContentId,              FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, WriteContentForDebug,                  FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetFreeSpaceSize,                      FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetTotalSpaceSize,                     FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, FlushPlaceHolder,                      FirmwareVersion_300),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetSizeFromPlaceHolderId,              FirmwareVersion_400),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, RepairInvalidFileAttribute,            FirmwareVersion_400),
                MAKE_SERVICE_COMMAND_META(ContentStorageInterface, GetRightsIdFromPlaceHolderIdWithCache, FirmwareVersion_800),
            };
    };

}