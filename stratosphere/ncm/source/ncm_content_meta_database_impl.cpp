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

#include "ncm_content_meta_database_impl.hpp"
#include "ncm_utils.hpp"

namespace ams::ncm {

    Result ContentMetaDatabaseImpl::GetContentIdByTypeImpl(ContentId *out, const ContentMetaKey& key, ContentType type, std::optional<u8> id_offset) {
        R_TRY(this->EnsureEnabled());

        /* Find the meta key. */
        ContentMetaKeyValueStore::Entry *entry = nullptr;
        R_TRY(this->FindContentMetaKeyValue(std::addressof(entry), key));
        const auto stored_key = entry->GetKey();

        /* Create a reader for this content meta. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, stored_key));

        ContentMetaReader reader(meta, meta_size);

        /* Find the content info. */
        const ContentInfo *content_info = nullptr;
        if (id_offset) {
            content_info = reader.GetContentInfo(type, *id_offset);
        } else {
            content_info = reader.GetContentInfo(type);
        }
        R_UNLESS(content_info != nullptr, ncm::ResultContentNotFound());

        /* Save output. */
        *out = content_info->content_id;
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::GetLatestContentMetaKeyImpl(ContentMetaKey *out_key, ProgramId id) {
        R_TRY(this->EnsureEnabled());

        std::optional<ContentMetaKey> found_key = std::nullopt;
        for (auto entry = this->kvs->lower_bound(ContentMetaKey::Make(id, 0, ContentMetaType::Unknown)); entry != this->kvs->end(); entry++) {
            if (entry->GetKey().id != id) {
                break;
            }

            if (entry->GetKey().install_type == ContentInstallType::Full) {
                found_key = entry->GetKey();
            }
        }
        R_UNLESS(found_key, ncm::ResultContentMetaNotFound());

        *out_key = *found_key;
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::Set(const ContentMetaKey &key, sf::InBuffer value) {
        R_TRY(this->EnsureEnabled());
        return this->kvs->Set(key, value.GetPointer(), value.GetSize());
    }

    Result ContentMetaDatabaseImpl::Get(sf::Out<u64> out_size, const ContentMetaKey &key, sf::OutBuffer out_value) {
        R_TRY(this->EnsureEnabled());
        return this->kvs->Get(out_size.GetPointer(), out_value.GetPointer(), out_value.GetSize(), key);
    }

    Result ContentMetaDatabaseImpl::Remove(const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());
        return this->kvs->Remove(key);
    }

    Result ContentMetaDatabaseImpl::GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type) {
        return this->GetContentIdByTypeImpl(out_content_id.GetPointer(), key, type, std::nullopt);
    }

    Result ContentMetaDatabaseImpl::ListContentInfo(sf::Out<u32> out_count, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, u32 offset) {
        R_UNLESS(offset <= std::numeric_limits<s32>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        ContentMetaReader reader(meta, meta_size);

        size_t count;
        for (count = 0; count < out_info.GetSize() && count + offset < reader.GetContentCount(); count++) {
            out_info[count] = *reader.GetContentInfo(offset + count);
        }

        out_count.SetValue(count);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::List(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType type, ProgramId application_id, ProgramId program_id_min, ProgramId program_id_max, ContentInstallType install_type) {
        R_TRY(this->EnsureEnabled());

        size_t entries_total = 0;
        size_t entries_written = 0;

        for (auto entry = this->kvs->begin(); entry != this->kvs->end(); entry++) {
            ContentMetaKey key = entry->GetKey();

            /* Check if this entry matches the given filters. */
            if (!((type == ContentMetaType::Unknown || key.type == type) && (program_id_min <= key.id && key.id <= program_id_max) && (install_type == ContentInstallType::Unknown || key.install_type == install_type))) {
                continue;
            }

            /* If application id is present, check if it matches the filter. */
            if (application_id != InvalidProgramId) {
                const void *meta;
                size_t meta_size;
                R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

                ContentMetaReader reader(meta, meta_size);

                if (const auto entry_application_id = reader.GetApplicationId(key); entry_application_id && application_id != *entry_application_id) {
                    continue;
                }
            }

            /* Write the entry to the output buffer. */
            if (entries_written < out_info.GetSize()) {
                out_info[entries_written++] = key;
            }
            entries_total++;
        }

        out_entries_total.SetValue(entries_total);
        out_entries_written.SetValue(entries_written);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId program_id) {
        R_TRY(this->EnsureEnabled());
        return this->GetLatestContentMetaKeyImpl(out_key.GetPointer(), program_id);
    }

    Result ContentMetaDatabaseImpl::ListApplication(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType type) {
        R_TRY(this->EnsureEnabled());

        size_t entries_total = 0;
        size_t entries_written = 0;

        for (auto entry = this->kvs->begin(); entry != this->kvs->end(); entry++) {
            ContentMetaKey key = entry->GetKey();

            /* Check if this entry matches the given filters. */
            if (!((type == ContentMetaType::Unknown || key.type == type))) {
                continue;
            }

            /* Check if the entry has an application id. */
            ContentMetaReader reader(entry->GetValuePointer(), entry->GetValueSize());

            if (const auto entry_application_id = reader.GetApplicationId(key); entry_application_id) {
                /* Write the entry to the output buffer. */
                if (entries_written < out_keys.GetSize()) {
                    out_keys[entries_written++] = { key, *entry_application_id };
                }
                entries_total++;
            }
        }

        out_entries_total.SetValue(entries_total);
        out_entries_written.SetValue(entries_written);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::Has(sf::Out<bool> out, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        *out = false;

        size_t size;
        R_TRY_CATCH(this->kvs->GetValueSize(&size, key)) {
            R_CONVERT(kvdb::ResultKeyNotFound, ResultSuccess());
        } R_END_TRY_CATCH;

        *out = true;
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) {
        R_TRY(this->EnsureEnabled());
        *out = false;

        for (size_t i = 0; i < keys.GetSize(); i++) {
            bool has;
            R_TRY(this->Has(std::addressof(has), keys[i]));
            R_UNLESS(has, ResultSuccess());
        }

        *out = true;
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::GetSize(sf::Out<u64> out_size, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        size_t size;
        R_TRY(this->GetContentMetaSize(&size, key));

        out_size.SetValue(size);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());
        R_UNLESS(key.type == ContentMetaType::Application || key.type == ContentMetaType::Patch, ncm::ResultInvalidContentMetaKey());

        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        ContentMetaReader reader(meta, meta_size);

        if (key.type == ContentMetaType::Application) {
            out_version.SetValue(reader.GetExtendedHeader<ApplicationMetaExtendedHeader>()->required_system_version);
        } else {
            out_version.SetValue(reader.GetExtendedHeader<PatchMetaExtendedHeader>()->required_system_version);
        }

        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::GetPatchId(sf::Out<ProgramId> out_patch_id, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());
        R_UNLESS(key.type == ContentMetaType::Application, ncm::ResultInvalidContentMetaKey());

        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        ContentMetaReader reader(meta, meta_size);

        out_patch_id.SetValue(reader.GetExtendedHeader<ApplicationMetaExtendedHeader>()->patch_id);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::DisableForcibly() {
        this->disabled = true;
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) {
        R_TRY(this->EnsureEnabled());
        R_UNLESS(out_orphaned.GetSize() >= content_ids.GetSize(), ncm::ResultBufferInsufficient());

        /* Default to orphaned for all content ids. */
        for (size_t i = 0; i < out_orphaned.GetSize(); i++) {
            out_orphaned[i] = true;
        }

        R_UNLESS(this->kvs->GetCount() != 0, ResultSuccess());

        auto IsOrphanedContent = [](const sf::InArray<ContentId> &list, const ncm::ContentId &id) ALWAYS_INLINE_LAMBDA {
            for (size_t i = 0; i < list.GetSize(); i++) {
                if (list[i] == id) {
                    return std::make_optional(i);
                }
            }
            return std::optional<size_t>(std::nullopt);
        };

        for (auto entry = this->kvs->begin(); entry != this->kvs->end(); entry++) {
            ContentMetaReader reader(entry->GetValuePointer(), entry->GetValueSize());

            /* Check if any of this entry's content infos matches one of the content ids for lookup. */
            /* If they do, then the content id isn't orphaned. */
            for (size_t i = 0; i < reader.GetContentCount(); i++) {
                if (auto found = IsOrphanedContent(content_ids, reader.GetContentInfo(i)->GetId()); found) {
                    out_orphaned[*found] = false;
                }
            }
        }

        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::Commit() {
        R_TRY(this->EnsureEnabled());
        R_TRY(this->kvs->Save());
        return fsdevCommitDevice(this->mount_name);
    }

    Result ContentMetaDatabaseImpl::HasContent(sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id) {
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        ContentMetaReader reader(meta, meta_size);
        for (size_t i = 0; i < reader.GetContentCount(); i++) {
            if (content_id == reader.GetContentInfo(i)->GetId()) {
                out.SetValue(true);
                return ResultSuccess();
            }
        }

        out.SetValue(false);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::ListContentMetaInfo(sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, u32 offset) {
        R_UNLESS(offset <= std::numeric_limits<s32>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        ContentMetaReader reader(meta, meta_size);

        size_t count;
        for (count = 0; count < out_meta_info.GetSize() && count + offset <= reader.GetContentMetaCount(); count++) {
            out_meta_info[count] = *reader.GetContentMetaInfo(count + offset);
        }

        out_entries_written.SetValue(count);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::GetAttributes(sf::Out<ContentMetaAttribute> out_attributes, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        ContentMetaReader reader(meta, meta_size);

        out_attributes.SetValue(reader.GetHeader()->attributes);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        ContentMetaReader reader(meta, meta_size);

        /* Get the required version. */
        u32 required_version;
        if (key.type == ContentMetaType::Application && hos::GetVersion() >= hos::Version_900) {
            /* As of 9.0.0, applications can be dependent on a specific base application version. */
            required_version = reader.GetExtendedHeader<ApplicationMetaExtendedHeader>()->required_application_version;
        } else if (key.type == ContentMetaType::AddOnContent) {
            required_version = reader.GetExtendedHeader<AddOnContentMetaExtendedHeader>()->required_application_version;
        } else {
            return ncm::ResultInvalidContentMetaKey();
        }

        out_version.SetValue(required_version);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseImpl::GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) {
        return this->GetContentIdByTypeImpl(out_content_id.GetPointer(), key, type, std::make_optional(id_offset));
    }

}
