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

#include "ncm_icontentstorage.hpp"

namespace sts::ncm {

    Result IContentStorage::EnsureEnabled() {
        if (this->disabled) {
            return ResultNcmInvalidContentStorage;
        }

        return ResultSuccess;
    }

    Result IContentStorage::GeneratePlaceHolderId(Out<PlaceHolderId> out) {
        std::abort();
    }

    Result IContentStorage::CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, u64 size) {
        std::abort();
    }

    Result IContentStorage::DeletePlaceHolder(PlaceHolderId placeholder_id) {
        std::abort();
    }

    Result IContentStorage::HasPlaceHolder(Out<bool> out, PlaceHolderId placeholder_id) {
        std::abort();
    }

    Result IContentStorage::WritePlaceHolder(PlaceHolderId placeholder_id, u64 offset, InBuffer<u8> data) {
        std::abort();
    }

    Result IContentStorage::Register(PlaceHolderId placeholder_id, ContentId content_id) {
        std::abort();
    }

    Result IContentStorage::Delete(ContentId content_id) {
        std::abort();
    }

    Result IContentStorage::Has(Out<bool> out, ContentId content_id) {
        std::abort();
    }

    Result IContentStorage::GetPath(OutPointerWithServerSize<lr::Path, 0x1> out, ContentId content_id) {
        std::abort();
    }

    Result IContentStorage::GetPlaceHolderPath(OutPointerWithServerSize<lr::Path, 0x1> out, PlaceHolderId placeholder_id) {
        std::abort();
    }

    Result IContentStorage::CleanupAllPlaceHolder() {
        std::abort();
    }

    Result IContentStorage::ListPlaceHolder(Out<u32> out_count, OutBuffer<PlaceHolderId> out_buf) {
        std::abort();
    }

    Result IContentStorage::GetContentCount(Out<u32> out_count) {
        std::abort();
    }

    Result IContentStorage::ListContentId(Out<u32> out_count, OutBuffer<ContentId> out_buf, u32 start_offset) {    
        std::abort();
    }

    Result IContentStorage::GetSizeFromContentId(Out<u64> out_size, ContentId content_id) {
        std::abort();
    }

    Result IContentStorage::DisableForcibly() {
        std::abort();
    }

    Result IContentStorage::RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
        std::abort();
    }

    Result IContentStorage::SetPlaceHolderSize(PlaceHolderId placeholder_id, u64 size) {
        std::abort();
    }

    Result IContentStorage::ReadContentIdFile(OutBuffer<u8> buf, ContentId content_id, u64 offset) {
        std::abort();
    }

    Result IContentStorage::GetRightsIdFromPlaceHolderId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id) {
        std::abort();
    }

    Result IContentStorage::GetRightsIdFromContentId(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, ContentId content_id) {
        std::abort();
    }

    Result IContentStorage::WriteContentForDebug(ContentId content_id, u64 offset, InBuffer<u8> data) {
        std::abort();
    }

    Result IContentStorage::GetFreeSpaceSize(Out<u64> out_size) {
        std::abort();
    }

    Result IContentStorage::GetTotalSpaceSize(Out<u64> out_size) {
        std::abort();
    }

    Result IContentStorage::FlushPlaceHolder() {
        std::abort();
    }

    Result IContentStorage::GetSizeFromPlaceHolderId(Out<u64> out_size, PlaceHolderId placeholder_id) {
        std::abort();
    }

    Result IContentStorage::RepairInvalidFileAttribute() {
        std::abort();
    }

    Result IContentStorage::GetRightsIdFromPlaceHolderIdWithCache(Out<FsRightsId> out_rights_id, Out<u64> out_key_generation, PlaceHolderId placeholder_id, ContentId cache_content_id) {
        std::abort();
    }

}
