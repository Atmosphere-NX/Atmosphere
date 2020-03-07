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
            std::shared_ptr<IContentMetaDatabase> interface;
        public:
            ContentMetaDatabase() { /* ... */ }
            explicit ContentMetaDatabase(std::shared_ptr<IContentMetaDatabase> intf) : interface(std::move(intf)) { /* ... */ }

            ContentMetaDatabase(ContentMetaDatabase &&rhs) {
                this->interface = std::move(rhs.interface);
            }

            ContentMetaDatabase &operator=(ContentMetaDatabase &&rhs) {
                ContentMetaDatabase(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(ContentMetaDatabase &rhs) {
                std::swap(this->interface, rhs.interface);
            }
        public:
            Result Set(const ContentMetaKey &key, const void *buf, size_t size) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->Set(key, sf::InBuffer(buf, size));
            }

            Result Get(size_t *out_size, void *dst, size_t dst_size, const ContentMetaKey &key) {
                AMS_ASSERT(this->interface != nullptr);
                u64 size;
                R_TRY(this->interface->Get(std::addressof(size), key, sf::OutBuffer(dst, dst_size)));

                *out_size = size;
                return ResultSuccess();
            }

            #define AMS_NCM_DEFINE_GETTERS(Kind, IdType)                                                                                \
            Result Get##Kind(ContentId *out, IdType##Id id, u32 version) {                                                              \
                return this->interface->GetContentIdByType(out, ContentMetaKey::MakeUnknownType(id.value, version), ContentType::Kind); \
            }                                                                                                                           \
                                                                                                                                        \
            Result GetLatest##Kind(ContentId *out, IdType##Id id) {                                                                     \
                ContentMetaKey latest_key;                                                                                              \
                R_TRY(this->interface->GetLatestContentMetaKey(std::addressof(latest_key), id.value));                                  \
                return this->interface->GetContentIdByType(out, latest_key, ContentType::Kind);                                         \
            }

            AMS_NCM_DEFINE_GETTERS(Program,          Program)
            AMS_NCM_DEFINE_GETTERS(Data,             Data)
            AMS_NCM_DEFINE_GETTERS(Control,          Application)
            AMS_NCM_DEFINE_GETTERS(HtmlDocument,     Application)
            AMS_NCM_DEFINE_GETTERS(LegalInformation, Application)

            #undef AMS_NCM_DEFINE_GETTERS

            Result Remove(const ContentMetaKey &key) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->Remove(key);
            }

            Result Remove(SystemProgramId id, u32 version) {
                return this->Remove(ContentMetaKey::Make(id, version));
            }

            Result Remove(SystemDataId id, u32 version) {
                return this->Remove(ContentMetaKey::Make(id, version));
            }

            Result Remove(ApplicationId id, u32 version) {
                return this->Remove(ContentMetaKey::Make(id, version));
            }

            Result GetContentIdByType(ContentId *out_content_id, const ContentMetaKey &key, ContentType type) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->GetContentIdByType(out_content_id, key, type);
            }

            Result GetContentIdByTypeAndIdOffset(ContentId *out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->GetContentIdByTypeAndIdOffset(out_content_id, key, type, id_offset);
            }

            ListCount ListApplication(ApplicationContentMetaKey *dst, size_t dst_size) {
                ListCount lc = {};
                R_ABORT_UNLESS(this->interface->ListApplication(std::addressof(lc.total), std::addressof(lc.written), sf::OutArray<ApplicationContentMetaKey>(dst, dst_size), ContentMetaType::Unknown));
                return lc;
            }

            ListCount ListContentMeta(ContentMetaKey *dst, size_t dst_size, ContentMetaType type = ContentMetaType::Unknown, ApplicationId app_id = InvalidApplicationId, u64 min = std::numeric_limits<u64>::min(), u64 max = std::numeric_limits<u64>::max(), ContentInstallType install_type = ContentInstallType::Full) {
                ListCount lc = {};
                R_ABORT_UNLESS(this->interface->List(std::addressof(lc.total), std::addressof(lc.written), sf::OutArray<ContentMetaKey>(dst, dst_size), type, app_id, min, max, install_type));
                return lc;
            }

            Result GetLatest(ContentMetaKey *out_key, u64 id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->GetLatestContentMetaKey(out_key, id);
            }

            Result ListContentInfo(s32 *out_count, ContentInfo *dst, size_t dst_size, const ContentMetaKey &key, s32 offset) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->ListContentInfo(out_count, sf::OutArray<ContentInfo>(dst, dst_size), key, offset);
            }

            Result ListContentMetaInfo(s32 *out_count, ContentMetaInfo *dst, size_t dst_size, const ContentMetaKey &key, s32 offset) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->ListContentMetaInfo(out_count, sf::OutArray<ContentMetaInfo>(dst, dst_size), key, offset);
            }

            Result Has(bool *out, const ContentMetaKey &key) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->Has(out, key);
            }

            Result HasAll(bool *out, const ContentMetaKey *keys, size_t num_keys) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->HasAll(out, sf::InArray<ContentMetaKey>(keys, num_keys));
            }

            Result HasContent(bool *out, const ContentMetaKey &key, const ContentId &content_id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->HasContent(out, key, content_id);
            }

            Result GetSize(size_t *out_size, const ContentMetaKey &key) {
                AMS_ASSERT(this->interface != nullptr);
                u64 size;
                R_TRY(this->interface->GetSize(std::addressof(size), key));

                *out_size = size;
                return ResultSuccess();
            }

            Result GetRequiredSystemVersion(u32 *out_version, const ContentMetaKey &key) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->GetRequiredSystemVersion(out_version, key);
            }

            Result GetPatchId(PatchId *out_patch_id, const ContentMetaKey &key) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->GetPatchId(out_patch_id, key);
            }

            Result DisableForcibly() {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->DisableForcibly();
            }

            Result LookupOrphanContent(bool *out_orphaned, ContentId *content_list, size_t count) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->LookupOrphanContent(sf::OutArray<bool>(out_orphaned, count), sf::InArray<ContentId>(content_list, count));
            }

            Result Commit() {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->Commit();
            }

            Result GetAttributes(u8 *out_attributes, const ContentMetaKey &key) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->GetAttributes(out_attributes, key);
            }

            Result GetRequiredApplicationVersion(u32 *out_version, const ContentMetaKey &key) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->GetRequiredApplicationVersion(out_version, key);
            }
    };

}
