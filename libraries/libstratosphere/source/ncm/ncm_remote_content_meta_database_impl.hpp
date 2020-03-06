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
#include <stratosphere.hpp>

namespace ams::ncm {

    class RemoteContentMetaDatabaseImpl final : public IContentMetaDatabase {
        private:
            ::NcmContentMetaDatabase srv;
        public:
            RemoteContentMetaDatabaseImpl(::NcmContentMetaDatabase &db) : srv(db) { /* ... */ }

            ~RemoteContentMetaDatabaseImpl() { ::ncmContentMetaDatabaseClose(std::addressof(srv)); }
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
            virtual Result Set(const ContentMetaKey &key, sf::InBuffer value) override {
                return ncmContentMetaDatabaseSet(std::addressof(this->srv), Convert(key), value.GetPointer(), value.GetSize());
            }

            virtual Result Get(sf::Out<u64> out_size, const ContentMetaKey &key, sf::OutBuffer out_value) override {
                return ncmContentMetaDatabaseGet(std::addressof(this->srv), Convert(key), out_size.GetPointer(), out_value.GetPointer(), out_value.GetSize());
            }

            virtual Result Remove(const ContentMetaKey &key) override {
                return ncmContentMetaDatabaseRemove(std::addressof(this->srv), Convert(key));
            }

            virtual Result GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type) override {
                return ncmContentMetaDatabaseGetContentIdByType(std::addressof(this->srv), Convert(out_content_id.GetPointer()), Convert(key), static_cast<::NcmContentType>(type));
            }

            virtual Result ListContentInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, s32 offset) override {
                return ncmContentMetaDatabaseListContentInfo(std::addressof(this->srv), out_entries_written.GetPointer(), Convert(out_info.GetPointer()), out_info.GetSize(), Convert(key), offset);
            }

            virtual Result List(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type) override {
                return ncmContentMetaDatabaseList(std::addressof(this->srv), out_entries_total.GetPointer(), out_entries_written.GetPointer(), Convert(out_info.GetPointer()), out_info.GetSize(), static_cast<::NcmContentMetaType>(meta_type), application_id.value, min, max, static_cast<::NcmContentInstallType>(install_type));
            }

            virtual Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, u64 id) override {
                return ncmContentMetaDatabaseGetLatestContentMetaKey(std::addressof(this->srv), Convert(out_key.GetPointer()), static_cast<u64>(id));
            }

            virtual Result ListApplication(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType meta_type) override {
                return ncmContentMetaDatabaseListApplication(std::addressof(this->srv), out_entries_total.GetPointer(), out_entries_written.GetPointer(), Convert(out_keys.GetPointer()), out_keys.GetSize(), static_cast<::NcmContentMetaType>(meta_type));
            }

            virtual Result Has(sf::Out<bool> out, const ContentMetaKey &key) override {
                return ncmContentMetaDatabaseHas(std::addressof(this->srv), out.GetPointer(), Convert(key));
            }

            virtual Result HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) override {
                return ncmContentMetaDatabaseHasAll(std::addressof(this->srv), out.GetPointer(), Convert(keys.GetPointer()), keys.GetSize());
            }

            virtual Result GetSize(sf::Out<u64> out_size, const ContentMetaKey &key) override {
                return ncmContentMetaDatabaseGetSize(std::addressof(this->srv), out_size.GetPointer(), Convert(key));
            }

            virtual Result GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key) override {
                return ncmContentMetaDatabaseGetRequiredSystemVersion(std::addressof(this->srv), out_version.GetPointer(), Convert(key));
            }

            virtual Result GetPatchId(sf::Out<PatchId> out_patch_id, const ContentMetaKey &key) override {
                return ncmContentMetaDatabaseGetPatchId(std::addressof(this->srv), reinterpret_cast<u64 *>(out_patch_id.GetPointer()), Convert(key));
            }

            virtual Result DisableForcibly() override {
                return ncmContentMetaDatabaseDisableForcibly(std::addressof(this->srv));
            }

            virtual Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) override {
                return ncmContentMetaDatabaseLookupOrphanContent(std::addressof(this->srv), out_orphaned.GetPointer(), Convert(content_ids.GetPointer()), std::min(out_orphaned.GetSize(), content_ids.GetSize()));
            }

            virtual Result Commit() override {
                return ncmContentMetaDatabaseCommit(std::addressof(this->srv));
            }

            virtual Result HasContent(sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id) override {
                return ncmContentMetaDatabaseHasContent(std::addressof(this->srv), out.GetPointer(), Convert(key), Convert(content_id));
            }

            virtual Result ListContentMetaInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, s32 offset) override {
                return ncmContentMetaDatabaseListContentMetaInfo(std::addressof(this->srv), out_entries_written.GetPointer(), out_meta_info.GetPointer(), out_meta_info.GetSize(), Convert(key), offset);
            }

            virtual Result GetAttributes(sf::Out<u8> out_attributes, const ContentMetaKey &key) override {
                static_assert(sizeof(ContentMetaAttribute) == sizeof(u8));
                return ncmContentMetaDatabaseGetAttributes(std::addressof(this->srv), Convert(key), out_attributes.GetPointer());
            }

            virtual Result GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key) override {
                return ncmContentMetaDatabaseGetRequiredApplicationVersion(std::addressof(this->srv), out_version.GetPointer(), Convert(key));
            }

            virtual Result GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) override {
                return ncmContentMetaDatabaseGetContentIdByTypeAndIdOffset(std::addressof(this->srv), Convert(out_content_id.GetPointer()), Convert(key), static_cast<::NcmContentType>(type), id_offset);
            }

    };

}
