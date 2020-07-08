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

    #define AMS_NCM_I_CONTENT_STORAGE_INTERFACE_INFO(C, H)                                                                                                                                                                     \
        AMS_SF_METHOD_INFO(C, H,  0, Result, GeneratePlaceHolderId,                  (sf::Out<PlaceHolderId> out))                                                                                                             \
        AMS_SF_METHOD_INFO(C, H,  1, Result, CreatePlaceHolder,                      (PlaceHolderId placeholder_id, ContentId content_id, s64 size))                                                                           \
        AMS_SF_METHOD_INFO(C, H,  2, Result, DeletePlaceHolder,                      (PlaceHolderId placeholder_id))                                                                                                           \
        AMS_SF_METHOD_INFO(C, H,  3, Result, HasPlaceHolder,                         (sf::Out<bool> out, PlaceHolderId placeholder_id))                                                                                        \
        AMS_SF_METHOD_INFO(C, H,  4, Result, WritePlaceHolder,                       (PlaceHolderId placeholder_id, s64 offset, sf::InBuffer data))                                                                            \
        AMS_SF_METHOD_INFO(C, H,  5, Result, Register,                               (PlaceHolderId placeholder_id, ContentId content_id))                                                                                     \
        AMS_SF_METHOD_INFO(C, H,  6, Result, Delete,                                 (ContentId content_id))                                                                                                                   \
        AMS_SF_METHOD_INFO(C, H,  7, Result, Has,                                    (sf::Out<bool> out, ContentId content_id))                                                                                                \
        AMS_SF_METHOD_INFO(C, H,  8, Result, GetPath,                                (sf::Out<Path> out, ContentId content_id))                                                                                                \
        AMS_SF_METHOD_INFO(C, H,  9, Result, GetPlaceHolderPath,                     (sf::Out<Path> out, PlaceHolderId placeholder_id))                                                                                        \
        AMS_SF_METHOD_INFO(C, H, 10, Result, CleanupAllPlaceHolder,                  ())                                                                                                                                       \
        AMS_SF_METHOD_INFO(C, H, 11, Result, ListPlaceHolder,                        (sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf))                                                                     \
        AMS_SF_METHOD_INFO(C, H, 12, Result, GetContentCount,                        (sf::Out<s32> out_count))                                                                                                                 \
        AMS_SF_METHOD_INFO(C, H, 13, Result, ListContentId,                          (sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 start_offset))                                                       \
        AMS_SF_METHOD_INFO(C, H, 14, Result, GetSizeFromContentId,                   (sf::Out<s64> out_size, ContentId content_id))                                                                                            \
        AMS_SF_METHOD_INFO(C, H, 15, Result, DisableForcibly,                        ())                                                                                                                                       \
        AMS_SF_METHOD_INFO(C, H, 16, Result, RevertToPlaceHolder,                    (PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id),               hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 17, Result, SetPlaceHolderSize,                     (PlaceHolderId placeholder_id, s64 size),                                                         hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 18, Result, ReadContentIdFile,                      (sf::OutBuffer buf, ContentId content_id, s64 offset),                                            hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 19, Result, GetRightsIdFromPlaceHolderIdDeprecated, (sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id),                         hos::Version_2_0_0, hos::Version_2_3_0) \
        AMS_SF_METHOD_INFO(C, H, 19, Result, GetRightsIdFromPlaceHolderId,           (sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id),                             hos::Version_3_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 20, Result, GetRightsIdFromContentIdDeprecated,     (sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id),                                 hos::Version_2_0_0, hos::Version_2_3_0) \
        AMS_SF_METHOD_INFO(C, H, 20, Result, GetRightsIdFromContentId,               (sf::Out<ncm::RightsId> out_rights_id, ContentId content_id),                                     hos::Version_3_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 21, Result, WriteContentForDebug,                   (ContentId content_id, s64 offset, sf::InBuffer data),                                            hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 22, Result, GetFreeSpaceSize,                       (sf::Out<s64> out_size),                                                                          hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 23, Result, GetTotalSpaceSize,                      (sf::Out<s64> out_size),                                                                          hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 24, Result, FlushPlaceHolder,                       (),                                                                                               hos::Version_3_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 25, Result, GetSizeFromPlaceHolderId,               (sf::Out<s64> out, PlaceHolderId placeholder_id),                                                 hos::Version_4_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 26, Result, RepairInvalidFileAttribute,             (),                                                                                               hos::Version_4_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 27, Result, GetRightsIdFromPlaceHolderIdWithCache,  (sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id), hos::Version_8_0_0)

    AMS_SF_DEFINE_INTERFACE(IContentStorage, AMS_NCM_I_CONTENT_STORAGE_INTERFACE_INFO)

}
