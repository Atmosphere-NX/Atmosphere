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

    class ContentMetaDatabaseImplBase {
        NON_COPYABLE(ContentMetaDatabaseImplBase);
        NON_MOVEABLE(ContentMetaDatabaseImplBase);
        protected:
            using ContentMetaKeyValueStore = ams::kvdb::MemoryKeyValueStore<ContentMetaKey>;
        protected:
            ContentMetaKeyValueStore *m_kvs;
            char m_mount_name[fs::MountNameLengthMax + 1];
            bool m_disabled;
        protected:
            ContentMetaDatabaseImplBase(ContentMetaKeyValueStore *kvs) : m_kvs(kvs), m_mount_name(), m_disabled(false) { /* ... */ }

            ContentMetaDatabaseImplBase(ContentMetaKeyValueStore *kvs, const char *mount_name) : ContentMetaDatabaseImplBase(kvs) {
                std::strcpy(m_mount_name, mount_name);
            }
        protected:
            /* Helpers. */
            Result EnsureEnabled() const {
                R_UNLESS(!m_disabled, ncm::ResultInvalidContentMetaDatabase());
                R_SUCCEED();
            }

            Result GetContentMetaSize(size_t *out, const ContentMetaKey &key) const {
                R_TRY_CATCH(m_kvs->GetValueSize(out, key)) {
                    R_CONVERT(kvdb::ResultKeyNotFound, ncm::ResultContentMetaNotFound())
                } R_END_TRY_CATCH;

                R_SUCCEED();
            }

            Result GetContentMetaPointer(const void **out_value_ptr, size_t *out_size, const ContentMetaKey &key) const {
                R_TRY(this->GetContentMetaSize(out_size, key));
                R_RETURN(m_kvs->GetValuePointer(reinterpret_cast<const ContentMetaHeader **>(out_value_ptr), key));
            }
        public:
            /* Actual commands. */
            virtual Result Set(const ContentMetaKey &key, const sf::InBuffer &value) = 0;
            virtual Result Get(sf::Out<u64> out_size, const ContentMetaKey &key, const sf::OutBuffer &out_value) = 0;
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
            virtual Result GetPatchContentMetaId(sf::Out<u64> out_patch_id, const ContentMetaKey &key) = 0;
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
            virtual Result GetContentAccessibilities(sf::Out<u8> out_accessibilities, const ContentMetaKey &key) = 0;
            virtual Result GetContentInfoByType(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type) = 0;
            virtual Result GetContentInfoByTypeAndIdOffset(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type, u8 id_offset) = 0;
            virtual Result GetPlatform(sf::Out<ncm::ContentMetaPlatform> out, const ContentMetaKey &key) = 0;
            virtual Result HasAttributes(sf::Out<u8> out, u8 attr_mask) = 0;
    };
    static_assert(ncm::IsIContentMetaDatabase<ContentMetaDatabaseImplBase>);

}
