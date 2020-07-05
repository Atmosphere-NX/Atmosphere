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

    #define AMS_NCM_I_CONTENT_META_DATABASE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                               \
        AMS_SF_METHOD_INFO(C, H,  0, Result, Set,                           (const ContentMetaKey &key, sf::InBuffer value))                                                                                                                                                                                                   \
        AMS_SF_METHOD_INFO(C, H,  1, Result, Get,                           (sf::Out<u64> out_size, const ContentMetaKey &key, sf::OutBuffer out_value))                                                                                                                                                                       \
        AMS_SF_METHOD_INFO(C, H,  2, Result, Remove,                        (const ContentMetaKey &key))                                                                                                                                                                                                                       \
        AMS_SF_METHOD_INFO(C, H,  3, Result, GetContentIdByType,            (sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type))                                                                                                                                                                  \
        AMS_SF_METHOD_INFO(C, H,  4, Result, ListContentInfo,               (sf::Out<s32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, s32 offset))                                                                                                                              \
        AMS_SF_METHOD_INFO(C, H,  5, Result, List,                          (sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type))                      \
        AMS_SF_METHOD_INFO(C, H,  6, Result, GetLatestContentMetaKey,       (sf::Out<ContentMetaKey> out_key, u64 id))                                                                                                                                                                                                         \
        AMS_SF_METHOD_INFO(C, H,  7, Result, ListApplication,               (sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType meta_type))                                                                                            \
        AMS_SF_METHOD_INFO(C, H,  8, Result, Has,                           (sf::Out<bool> out, const ContentMetaKey &key))                                                                                                                                                                                                    \
        AMS_SF_METHOD_INFO(C, H,  9, Result, HasAll,                        (sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys))                                                                                                                                                                                      \
        AMS_SF_METHOD_INFO(C, H, 10, Result, GetSize,                       (sf::Out<u64> out_size, const ContentMetaKey &key))                                                                                                                                                                                                \
        AMS_SF_METHOD_INFO(C, H, 11, Result, GetRequiredSystemVersion,      (sf::Out<u32> out_version, const ContentMetaKey &key))                                                                                                                                                                                             \
        AMS_SF_METHOD_INFO(C, H, 12, Result, GetPatchId,                    (sf::Out<PatchId> out_patch_id, const ContentMetaKey &key))                                                                                                                                                                                        \
        AMS_SF_METHOD_INFO(C, H, 13, Result, DisableForcibly,               ())                                                                                                                                                                                                                                                \
        AMS_SF_METHOD_INFO(C, H, 14, Result, LookupOrphanContent,           (const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids))                                                                                                                                                               \
        AMS_SF_METHOD_INFO(C, H, 15, Result, Commit,                        ())                                                                                                                                                                                                                                                \
        AMS_SF_METHOD_INFO(C, H, 16, Result, HasContent,                    (sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id))                                                                                                                                                                       \
        AMS_SF_METHOD_INFO(C, H, 17, Result, ListContentMetaInfo,           (sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, s32 offset))                                                                                                                     \
        AMS_SF_METHOD_INFO(C, H, 18, Result, GetAttributes,                 (sf::Out<u8> out_attributes, const ContentMetaKey &key))                                                                                                                                                                                           \
        AMS_SF_METHOD_INFO(C, H, 19, Result, GetRequiredApplicationVersion, (sf::Out<u32> out_version, const ContentMetaKey &key),                                                                                                                                                                        hos::Version_2_0_0)  \
        AMS_SF_METHOD_INFO(C, H, 20, Result, GetContentIdByTypeAndIdOffset, (sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset),                                                                                                                               hos::Version_5_0_0)  \
        AMS_SF_METHOD_INFO(C, H, 21, Result, GetCount,                      (sf::Out<u32> out_count),                                                                                                                                                                                                     hos::Version_10_0_0) \
        AMS_SF_METHOD_INFO(C, H, 22, Result, GetOwnerApplicationId,         (sf::Out<ApplicationId> out_id, const ContentMetaKey &key),                                                                                                                                                                   hos::Version_10_0_0)

    AMS_SF_DEFINE_INTERFACE(IContentMetaDatabase, AMS_NCM_I_CONTENT_META_DATABASE_INTERFACE_INFO)

}
