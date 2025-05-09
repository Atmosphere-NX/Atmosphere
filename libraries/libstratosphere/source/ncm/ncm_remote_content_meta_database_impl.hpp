/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>

namespace ams::ncm {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteContentMetaDatabaseImpl {
        private:
            ::NcmContentMetaDatabase m_srv;
        public:
            RemoteContentMetaDatabaseImpl(::NcmContentMetaDatabase &db) : m_srv(db) { /* ... */ }

            ~RemoteContentMetaDatabaseImpl() { ::ncmContentMetaDatabaseClose(std::addressof(m_srv)); }
        private:
            ALWAYS_INLINE ::NcmContentMetaKey *Convert(ContentMetaKey *k) {
                static_assert(sizeof(ContentMetaKey) == sizeof(::NcmContentMetaKey));
                return reinterpret_cast<::NcmContentMetaKey *>(k);
            }
            ALWAYS_INLINE const ::NcmContentMetaKey *Convert(const ContentMetaKey *k) {
                static_assert(sizeof(ContentMetaKey) == sizeof(::NcmContentMetaKey));
                return reinterpret_cast<const ::NcmContentMetaKey *>(k);
            }

            ALWAYS_INLINE const ::NcmContentMetaKey *Convert(const ContentMetaKey &k) {
                static_assert(sizeof(ContentMetaKey) == sizeof(::NcmContentMetaKey));
                return reinterpret_cast<const ::NcmContentMetaKey *>(std::addressof(k));
            }

            ALWAYS_INLINE ::NcmApplicationContentMetaKey *Convert(ApplicationContentMetaKey *k) {
                static_assert(sizeof(ApplicationContentMetaKey) == sizeof(::NcmApplicationContentMetaKey));
                return reinterpret_cast<::NcmApplicationContentMetaKey *>(k);
            }

            ALWAYS_INLINE ::NcmContentInfo *Convert(ContentInfo *c) {
                static_assert(sizeof(ContentInfo)   == sizeof(::NcmContentInfo));
                return reinterpret_cast<::NcmContentInfo *>(c);
            }

            ALWAYS_INLINE ::NcmContentId *Convert(ContentId *c) {
                static_assert(sizeof(ContentId)     == sizeof(::NcmContentId));
                return reinterpret_cast<::NcmContentId *>(c);
            }

            ALWAYS_INLINE ::NcmContentId *Convert(ContentId &c) {
                static_assert(sizeof(ContentId)     == sizeof(::NcmContentId));
                return reinterpret_cast<::NcmContentId *>(std::addressof(c));
            }

            ALWAYS_INLINE const ::NcmContentId *Convert(const ContentId *c) {
                static_assert(sizeof(ContentId)     == sizeof(::NcmContentId));
                return reinterpret_cast<const ::NcmContentId *>(c);
            }


            ALWAYS_INLINE const ::NcmContentId *Convert(const ContentId &c) {
                static_assert(sizeof(ContentId)     == sizeof(::NcmContentId));
                return reinterpret_cast<const ::NcmContentId *>(std::addressof(c));
            }
        public:
            Result Set(const ContentMetaKey &key, const sf::InBuffer &value) {
                R_RETURN(ncmContentMetaDatabaseSet(std::addressof(m_srv), Convert(key), value.GetPointer(), value.GetSize()));
            }

            Result Get(sf::Out<u64> out_size, const ContentMetaKey &key, const sf::OutBuffer &out_value) {
                R_RETURN(ncmContentMetaDatabaseGet(std::addressof(m_srv), Convert(key), out_size.GetPointer(), out_value.GetPointer(), out_value.GetSize()));
            }

            Result Remove(const ContentMetaKey &key) {
                R_RETURN(ncmContentMetaDatabaseRemove(std::addressof(m_srv), Convert(key)));
            }

            Result GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type) {
                R_RETURN(ncmContentMetaDatabaseGetContentIdByType(std::addressof(m_srv), Convert(out_content_id.GetPointer()), Convert(key), static_cast<::NcmContentType>(type)));
            }

            Result ListContentInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, s32 offset) {
                R_RETURN(ncmContentMetaDatabaseListContentInfo(std::addressof(m_srv), out_entries_written.GetPointer(), Convert(out_info.GetPointer()), out_info.GetSize(), Convert(key), offset));
            }

            Result List(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type) {
                R_RETURN(ncmContentMetaDatabaseList(std::addressof(m_srv), out_entries_total.GetPointer(), out_entries_written.GetPointer(), Convert(out_info.GetPointer()), out_info.GetSize(), static_cast<::NcmContentMetaType>(meta_type), application_id.value, min, max, static_cast<::NcmContentInstallType>(install_type)));
            }

            Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, u64 id) {
                R_RETURN(ncmContentMetaDatabaseGetLatestContentMetaKey(std::addressof(m_srv), Convert(out_key.GetPointer()), static_cast<u64>(id)));
            }

            Result ListApplication(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType meta_type) {
                R_RETURN(ncmContentMetaDatabaseListApplication(std::addressof(m_srv), out_entries_total.GetPointer(), out_entries_written.GetPointer(), Convert(out_keys.GetPointer()), out_keys.GetSize(), static_cast<::NcmContentMetaType>(meta_type)));
            }

            Result Has(sf::Out<bool> out, const ContentMetaKey &key) {
                R_RETURN(ncmContentMetaDatabaseHas(std::addressof(m_srv), out.GetPointer(), Convert(key)));
            }

            Result HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) {
                R_RETURN(ncmContentMetaDatabaseHasAll(std::addressof(m_srv), out.GetPointer(), Convert(keys.GetPointer()), keys.GetSize()));
            }

            Result GetSize(sf::Out<u64> out_size, const ContentMetaKey &key) {
                R_RETURN(ncmContentMetaDatabaseGetSize(std::addressof(m_srv), out_size.GetPointer(), Convert(key)));
            }

            Result GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key) {
                R_RETURN(ncmContentMetaDatabaseGetRequiredSystemVersion(std::addressof(m_srv), out_version.GetPointer(), Convert(key)));
            }

            Result GetPatchContentMetaId(sf::Out<u64> out_patch_id, const ContentMetaKey &key) {
                R_RETURN(ncmContentMetaDatabaseGetPatchContentMetaId(std::addressof(m_srv), out_patch_id.GetPointer(), Convert(key)));
            }

            Result DisableForcibly() {
                R_RETURN(ncmContentMetaDatabaseDisableForcibly(std::addressof(m_srv)));
            }

            Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) {
                R_RETURN(ncmContentMetaDatabaseLookupOrphanContent(std::addressof(m_srv), out_orphaned.GetPointer(), Convert(content_ids.GetPointer()), std::min(out_orphaned.GetSize(), content_ids.GetSize())));
            }

            Result Commit() {
                R_RETURN(ncmContentMetaDatabaseCommit(std::addressof(m_srv)));
            }

            Result HasContent(sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id) {
                R_RETURN(ncmContentMetaDatabaseHasContent(std::addressof(m_srv), out.GetPointer(), Convert(key), Convert(content_id)));
            }

            Result ListContentMetaInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, s32 offset) {
                R_RETURN(ncmContentMetaDatabaseListContentMetaInfo(std::addressof(m_srv), out_entries_written.GetPointer(), out_meta_info.GetPointer(), out_meta_info.GetSize(), Convert(key), offset));
            }

            Result GetAttributes(sf::Out<u8> out_attributes, const ContentMetaKey &key) {
                static_assert(sizeof(ContentMetaAttribute) == sizeof(u8));
                R_RETURN(ncmContentMetaDatabaseGetAttributes(std::addressof(m_srv), Convert(key), out_attributes.GetPointer()));
            }

            Result GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key) {
                R_RETURN(ncmContentMetaDatabaseGetRequiredApplicationVersion(std::addressof(m_srv), out_version.GetPointer(), Convert(key)));
            }

            Result GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) {
                R_RETURN(ncmContentMetaDatabaseGetContentIdByTypeAndIdOffset(std::addressof(m_srv), Convert(out_content_id.GetPointer()), Convert(key), static_cast<::NcmContentType>(type), id_offset));
            }

            Result GetCount(sf::Out<u32> out_count) {
                /* TODO: libnx bindings */
                AMS_UNUSED(out_count);
                AMS_ABORT();
            }

            Result GetOwnerApplicationId(sf::Out<ApplicationId> out_id, const ContentMetaKey &key) {
                /* TODO: libnx bindings */
                AMS_UNUSED(out_id, key);
                AMS_ABORT();
            }

            Result GetContentAccessibilities(sf::Out<u8> out_accessibilities, const ContentMetaKey &key) {
                /* TODO: libnx bindings */
                AMS_UNUSED(out_accessibilities, key);
                AMS_ABORT();
            }

            Result GetContentInfoByType(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type) {
                /* TODO: libnx bindings */
                AMS_UNUSED(out_content_info, key, type);
                AMS_ABORT();
            }

            Result GetContentInfoByTypeAndIdOffset(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type, u8 id_offset) {
                /* TODO: libnx bindings */
                AMS_UNUSED(out_content_info, key, type, id_offset);
                AMS_ABORT();
            }

            Result GetPlatform(sf::Out<ncm::ContentMetaPlatform> out, const ContentMetaKey &key) {
                static_assert(sizeof(ncm::ContentMetaPlatform) == sizeof(u8));
                R_RETURN(ncmContentMetaDatabaseGetPlatform(std::addressof(m_srv), reinterpret_cast<u8 *>(out.GetPointer()), Convert(key)));
            }

            Result HasAttributes(sf::Out<u8> out, u8 attr_mask) {
                /* TODO: libnx bindings */
                AMS_UNUSED(out, attr_mask);
                AMS_ABORT();
            }
    };
    static_assert(ncm::IsIContentMetaDatabase<RemoteContentMetaDatabaseImpl>);
    #endif

}
