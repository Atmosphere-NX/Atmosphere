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
#include <stratosphere/ncm/ncm_i_content_meta_database.hpp>
#include <stratosphere/ncm/ncm_integrated_list.hpp>

namespace ams::ncm {

    class IntegratedContentMetaDatabaseImpl {
        NON_COPYABLE(IntegratedContentMetaDatabaseImpl);
        NON_MOVEABLE(IntegratedContentMetaDatabaseImpl);
        private:
            using ListType = ncm::IntegratedList<ncm::IContentMetaDatabase, 2>;
            using DataType = ListType::ListData;
        private:
            os::SdkRecursiveMutex m_mutex;
            ListType m_list;
            bool m_disabled;
        public:
            IntegratedContentMetaDatabaseImpl() : m_mutex(), m_list(), m_disabled(false) { /* ... */ }

            void Add(sf::SharedPointer<ncm::IContentMetaDatabase> p, u8 id) {
                DataType data = {std::move(p), id};
                m_list.Add(data);
            }
        private:
            /* Helpers. */
            Result EnsureEnabled() const {
                R_UNLESS(!m_disabled, ncm::ResultInvalidContentMetaDatabase());
                R_SUCCEED();
            }
        public:
            /* Actual commands. */
            Result Set(const ContentMetaKey &key, const sf::InBuffer &value);
            Result Get(sf::Out<u64> out_size, const ContentMetaKey &key, const sf::OutBuffer &out_value);
            Result Remove(const ContentMetaKey &key);
            Result GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type);
            Result ListContentInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, s32 offset);
            Result List(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type);
            Result GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, u64 id);
            Result ListApplication(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType meta_type);
            Result Has(sf::Out<bool> out, const ContentMetaKey &key);
            Result HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys);
            Result GetSize(sf::Out<u64> out_size, const ContentMetaKey &key);
            Result GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key);
            Result GetPatchContentMetaId(sf::Out<u64> out_patch_id, const ContentMetaKey &key);
            Result DisableForcibly();
            Result LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids);
            Result Commit();
            Result HasContent(sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id);
            Result ListContentMetaInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, s32 offset);
            Result GetAttributes(sf::Out<u8> out_attributes, const ContentMetaKey &key);
            Result GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key);
            Result GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset);
            Result GetCount(sf::Out<u32> out_count);
            Result GetOwnerApplicationId(sf::Out<ApplicationId> out_id, const ContentMetaKey &key);
            Result GetContentAccessibilities(sf::Out<u8> out_accessibilities, const ContentMetaKey &key);
            Result GetContentInfoByType(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type);
            Result GetContentInfoByTypeAndIdOffset(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type, u8 id_offset);
            Result GetPlatform(sf::Out<ncm::ContentMetaPlatform> out, const ContentMetaKey &key);
            Result HasAttributes(sf::Out<u8> out, u8 attr_mask);
    };
    static_assert(ncm::IsIContentMetaDatabase<IntegratedContentMetaDatabaseImpl>);

}
