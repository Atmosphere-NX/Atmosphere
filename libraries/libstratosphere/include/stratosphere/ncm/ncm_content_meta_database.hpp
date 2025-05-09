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
#include <stratosphere/ncm/ncm_i_content_meta_database.hpp>

namespace ams::ncm {

    class ContentMetaDatabase {
        NON_COPYABLE(ContentMetaDatabase);
        public:
            struct ListCount {
                s32 written;
                s32 total;
            };
        private:
            sf::SharedPointer<IContentMetaDatabase> m_interface;
        public:
            ContentMetaDatabase() : m_interface(nullptr) { /* ... */ }
            explicit ContentMetaDatabase(sf::SharedPointer<IContentMetaDatabase> intf) : m_interface(intf) { /* ... */ }

            ContentMetaDatabase(ContentMetaDatabase &&rhs) {
                m_interface = std::move(rhs.m_interface);
            }

            ContentMetaDatabase &operator=(ContentMetaDatabase &&rhs) {
                ContentMetaDatabase(std::move(rhs)).swap(*this);
                return *this;
            }

            void swap(ContentMetaDatabase &rhs) {
                std::swap(m_interface, rhs.m_interface);
            }
        public:
            Result Set(const ContentMetaKey &key, const void *buf, size_t size) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->Set(key, sf::InBuffer(buf, size)));
            }

            Result Get(size_t *out_size, void *dst, size_t dst_size, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                u64 size;
                R_TRY(m_interface->Get(std::addressof(size), key, sf::OutBuffer(dst, dst_size)));

                *out_size = size;
                R_SUCCEED();
            }

            #define AMS_NCM_DEFINE_GETTERS(Kind, IdType)                                                                                 \
            Result Get##Kind(ContentId *out, IdType##Id id, u32 version) {                                                               \
                R_RETURN(m_interface->GetContentIdByType(out, ContentMetaKey::MakeUnknownType(id.value, version), ContentType::Kind));   \
            }                                                                                                                            \
                                                                                                                                         \
            Result Get##Kind(ContentInfo *out, IdType##Id id, u32 version) {                                                             \
                R_RETURN(m_interface->GetContentInfoByType(out, ContentMetaKey::MakeUnknownType(id.value, version), ContentType::Kind)); \
            }                                                                                                                            \
                                                                                                                                         \
            Result GetLatest##Kind(ContentId *out, IdType##Id id) {                                                                      \
                ContentMetaKey latest_key;                                                                                               \
                R_TRY(m_interface->GetLatestContentMetaKey(std::addressof(latest_key), id.value));                                       \
                R_RETURN(m_interface->GetContentIdByType(out, latest_key, ContentType::Kind));                                           \
            }                                                                                                                            \
                                                                                                                                         \
            Result GetLatest##Kind(ContentInfo *out, IdType##Id id) {                                                                    \
                ContentMetaKey latest_key;                                                                                               \
                R_TRY(m_interface->GetLatestContentMetaKey(std::addressof(latest_key), id.value));                                       \
                R_RETURN(m_interface->GetContentInfoByType(out, latest_key, ContentType::Kind));                                         \
            }

            AMS_NCM_DEFINE_GETTERS(Program,          Program)
            AMS_NCM_DEFINE_GETTERS(Data,             Data)
            AMS_NCM_DEFINE_GETTERS(Control,          Application)
            AMS_NCM_DEFINE_GETTERS(HtmlDocument,     Application)
            AMS_NCM_DEFINE_GETTERS(LegalInformation, Application)

            #undef AMS_NCM_DEFINE_GETTERS

            Result Remove(const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->Remove(key));
            }

            Result Remove(SystemProgramId id, u32 version) {
                R_RETURN(this->Remove(ContentMetaKey::Make(id, version)));
            }

            Result Remove(SystemDataId id, u32 version) {
                R_RETURN(this->Remove(ContentMetaKey::Make(id, version)));
            }

            Result Remove(ApplicationId id, u32 version) {
                R_RETURN(this->Remove(ContentMetaKey::Make(id, version)));
            }

            Result GetContentIdByType(ContentId *out_content_id, const ContentMetaKey &key, ContentType type) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetContentIdByType(out_content_id, key, type));
            }

            Result GetContentIdByTypeAndIdOffset(ContentId *out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetContentIdByTypeAndIdOffset(out_content_id, key, type, id_offset));
            }

            ListCount ListApplication(ApplicationContentMetaKey *dst, size_t dst_size) {
                ListCount lc = {};
                R_ABORT_UNLESS(m_interface->ListApplication(std::addressof(lc.total), std::addressof(lc.written), sf::OutArray<ApplicationContentMetaKey>(dst, dst_size), ContentMetaType::Unknown));
                return lc;
            }

            ListCount ListContentMeta(ContentMetaKey *dst, size_t dst_size, ContentMetaType type = ContentMetaType::Unknown, ApplicationId app_id = InvalidApplicationId, u64 min = std::numeric_limits<u64>::min(), u64 max = std::numeric_limits<u64>::max(), ContentInstallType install_type = ContentInstallType::Full) {
                ListCount lc = {};
                R_ABORT_UNLESS(m_interface->List(std::addressof(lc.total), std::addressof(lc.written), sf::OutArray<ContentMetaKey>(dst, dst_size), type, app_id, min, max, install_type));
                return lc;
            }

            Result GetLatest(ContentMetaKey *out_key, u64 id) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetLatestContentMetaKey(out_key, id));
            }

            Result ListContentInfo(s32 *out_count, ContentInfo *dst, size_t dst_size, const ContentMetaKey &key, s32 offset) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->ListContentInfo(out_count, sf::OutArray<ContentInfo>(dst, dst_size), key, offset));
            }

            Result ListContentMetaInfo(s32 *out_count, ContentMetaInfo *dst, size_t dst_size, const ContentMetaKey &key, s32 offset) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->ListContentMetaInfo(out_count, sf::OutArray<ContentMetaInfo>(dst, dst_size), key, offset));
            }

            Result Has(bool *out, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->Has(out, key));
            }

            Result HasAll(bool *out, const ContentMetaKey *keys, size_t num_keys) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->HasAll(out, sf::InArray<ContentMetaKey>(keys, num_keys)));
            }

            Result HasContent(bool *out, const ContentMetaKey &key, const ContentId &content_id) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->HasContent(out, key, content_id));
            }

            Result GetSize(size_t *out_size, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                u64 size;
                R_TRY(m_interface->GetSize(std::addressof(size), key));

                *out_size = size;
                R_SUCCEED();
            }

            Result GetRequiredSystemVersion(u32 *out_version, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetRequiredSystemVersion(out_version, key));
            }

            Result GetPatchId(PatchId *out_patch_id, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                static_assert(sizeof(*out_patch_id) == sizeof(u64));
                R_RETURN(m_interface->GetPatchContentMetaId(reinterpret_cast<u64 *>(out_patch_id), key));
            }

            Result GetDataPatchId(DataPatchId *out_patch_id, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                static_assert(sizeof(*out_patch_id) == sizeof(u64));
                R_RETURN(m_interface->GetPatchContentMetaId(reinterpret_cast<u64 *>(out_patch_id), key));
            }

            Result DisableForcibly() {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->DisableForcibly());
            }

            Result LookupOrphanContent(bool *out_orphaned, ContentId *content_list, size_t count) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->LookupOrphanContent(sf::OutArray<bool>(out_orphaned, count), sf::InArray<ContentId>(content_list, count)));
            }

            Result Commit() {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->Commit());
            }

            Result GetAttributes(u8 *out_attributes, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetAttributes(out_attributes, key));
            }

            Result GetRequiredApplicationVersion(u32 *out_version, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetRequiredApplicationVersion(out_version, key));
            }

            Result GetContentAccessibilities(u8 *out_accessibilities, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetContentAccessibilities(out_accessibilities, key));
            }

            Result GetContentInfoByType(ContentInfo *out_content_info, const ContentMetaKey &key, ContentType type) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetContentInfoByType(out_content_info, key, type));
            }

            Result GetContentInfoByTypeAndIdOffset(ContentInfo *out_content_info, const ContentMetaKey &key, ContentType type, u8 id_offset) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetContentInfoByTypeAndIdOffset(out_content_info, key, type, id_offset));
            }

            Result GetPlatform(ContentMetaPlatform *out, const ContentMetaKey &key) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->GetPlatform(out, key));
            }

            Result HasAttributes(u8 *out, u8 attr_mask) {
                AMS_ASSERT(m_interface != nullptr);
                R_RETURN(m_interface->HasAttributes(out, attr_mask));
            }
    };

}
