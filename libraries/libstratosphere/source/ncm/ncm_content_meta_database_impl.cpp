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
#include <stratosphere.hpp>
#include "ncm_content_meta_database_impl.hpp"

namespace ams::ncm {

    Result ContentMetaDatabaseImpl::GetContentInfoImpl(ContentInfo *out, const ContentMetaKey &key, ContentType type, util::optional<u8> id_offset) const {
        R_TRY(this->EnsureEnabled());

        /* Find the meta key. */
        const auto it = m_kvs->lower_bound(key);
        R_UNLESS(it != m_kvs->end(),        ncm::ResultContentMetaNotFound());
        R_UNLESS(it->GetKey().id == key.id, ncm::ResultContentMetaNotFound());

        const auto found_key = it->GetKey();

        /* Create a reader for this content meta. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, found_key));

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
        *out = *content_info;
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::Set(const ContentMetaKey &key, const sf::InBuffer &value) {
        R_TRY(this->EnsureEnabled());
        R_RETURN(m_kvs->Set(key, value.GetPointer(), value.GetSize()));
    }

    Result ContentMetaDatabaseImpl::Get(sf::Out<u64> out_size, const ContentMetaKey &key, const sf::OutBuffer &out_value) {
        R_TRY(this->EnsureEnabled());

        /* Get the entry from our key-value store. */
        size_t size;
        R_TRY_CATCH(m_kvs->Get(std::addressof(size), out_value.GetPointer(), out_value.GetSize(), key)) {
            R_CONVERT(kvdb::ResultKeyNotFound, ncm::ResultContentMetaNotFound())
        } R_END_TRY_CATCH;

        out_size.SetValue(size);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::Remove(const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        R_TRY_CATCH(m_kvs->Remove(key)) {
            R_CONVERT(kvdb::ResultKeyNotFound, ncm::ResultContentMetaNotFound())
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type) {
        R_RETURN(this->GetContentIdImpl(out_content_id.GetPointer(), key, type, util::nullopt));
    }

    Result ContentMetaDatabaseImpl::ListContentInfo(sf::Out<s32> out_count, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, s32 offset) {
        R_UNLESS(offset >= 0, ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        /* Obtain the content meta for the given key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Read content infos from the given offset up to the given count. */
        size_t count;
        for (count = 0; count < out_info.GetSize() && count + offset < reader.GetContentCount(); count++) {
            out_info[count] = *reader.GetContentInfo(offset + count);
        }

        out_count.SetValue(count);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::List(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type) {
        R_TRY(this->EnsureEnabled());

        size_t entries_total = 0;
        size_t entries_written = 0;

        /* Iterate over all entries. */
        for (auto &entry : *m_kvs) {
            const ContentMetaKey key = entry.GetKey();

            /* Check if this entry matches the given filters. */
            if (!((meta_type == ContentMetaType::Unknown || key.type == meta_type) && (min <= key.id && key.id <= max) && (install_type == ContentInstallType::Unknown || key.install_type == install_type))) {
                continue;
            }

            /* If application id is present, check if it matches the filter. */
            if (application_id != InvalidApplicationId) {
                /* Obtain the content meta for the key. */
                const void *meta;
                size_t meta_size;
                R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

                /* Create a reader. */
                ContentMetaReader reader(meta, meta_size);

                /* Ensure application id matches, if present. */
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
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, u64 id) {
        R_TRY(this->EnsureEnabled());

        util::optional<ContentMetaKey> found_key = util::nullopt;

        /* Find the last key with the desired program id. */
        for (auto entry = m_kvs->lower_bound(ContentMetaKey::MakeUnknownType(id, 0)); entry != m_kvs->end(); entry++) {
            /* No further entries will match the program id, discontinue. */
            if (entry->GetKey().id != id) {
                break;
            }

            /* We are only interested in keys with the Full content install type. */
            if (entry->GetKey().install_type == ContentInstallType::Full) {
                found_key = entry->GetKey();
            }
        }

        /* Check if the key is absent. */
        R_UNLESS(found_key, ncm::ResultContentMetaNotFound());

        out_key.SetValue(*found_key);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::ListApplication(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType type) {
        R_TRY(this->EnsureEnabled());

        size_t entries_total = 0;
        size_t entries_written = 0;

        /* Iterate over all entries. */
        for (auto &entry : *m_kvs) {
            const ContentMetaKey key = entry.GetKey();

            /* Check if this entry matches the given filters. */
            if (!(type == ContentMetaType::Unknown || key.type == type)) {
                continue;
            }

            /* Check if the entry has an application id. */
            ContentMetaReader reader(entry.GetValuePointer(), entry.GetValueSize());

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
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::Has(sf::Out<bool> out, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        *out = false;

        /* Check if key is present. */
        size_t size;
        R_TRY_CATCH(m_kvs->GetValueSize(&size, key)) {
            R_CONVERT(kvdb::ResultKeyNotFound, ResultSuccess());
        } R_END_TRY_CATCH;

        *out = true;
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) {
        R_TRY(this->EnsureEnabled());
        *out = false;

        /* Check if keys are present. */
        for (size_t i = 0; i < keys.GetSize(); i++) {
            /* Check if we have the current key. */
            bool has;
            R_TRY(this->Has(std::addressof(has), keys[i]));

            /* If we don't, then we can early return because we don't have all. */
            R_SUCCEED_IF(!has);
        }

        *out = true;
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetSize(sf::Out<u64> out_size, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        /* Determine the content meta size for the key. */
        size_t size;
        R_TRY(this->GetContentMetaSize(&size, key));

        out_size.SetValue(size);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        /* Only applications and patches have a required system version. */
        R_UNLESS(key.type == ContentMetaType::Application || key.type == ContentMetaType::Patch, ncm::ResultInvalidContentMetaKey());

        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Obtain the required system version. */
        switch (key.type) {
            case ContentMetaType::Application:
                out_version.SetValue(reader.GetExtendedHeader<ApplicationMetaExtendedHeader>()->required_system_version);
                break;
            case ContentMetaType::Patch:
                out_version.SetValue(reader.GetExtendedHeader<PatchMetaExtendedHeader>()->required_system_version);
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetPatchContentMetaId(sf::Out<u64> out_patch_id, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        /* Only applications can have patches. */
        R_UNLESS(key.type == ContentMetaType::Application || key.type == ContentMetaType::AddOnContent, ncm::ResultInvalidContentMetaKey());

        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Obtain the patch id. */
        switch (key.type) {
            case ContentMetaType::Application:
                out_patch_id.SetValue(reader.GetExtendedHeader<ApplicationMetaExtendedHeader>()->patch_id.value);
                break;
            case ContentMetaType::AddOnContent:
                R_UNLESS(reader.GetExtendedHeaderSize() == sizeof(AddOnContentMetaExtendedHeader), ncm::ResultInvalidAddOnContentMetaExtendedHeader());
                out_patch_id.SetValue(reader.GetExtendedHeader<AddOnContentMetaExtendedHeader>()->data_patch_id.value);
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::DisableForcibly() {
        m_disabled = true;
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) {
        R_TRY(this->EnsureEnabled());
        R_UNLESS(out_orphaned.GetSize() >= content_ids.GetSize(), ncm::ResultBufferInsufficient());

        /* Default to orphaned for all content ids. */
        for (size_t i = 0; i < out_orphaned.GetSize(); i++) {
            out_orphaned[i] = true;
        }

        auto IsOrphanedContent = [](const sf::InArray<ContentId> &list, const ncm::ContentId &id) ALWAYS_INLINE_LAMBDA -> util::optional<size_t> {
            /* Check if any input content ids match our found content id. */
            for (size_t i = 0; i < list.GetSize(); i++) {
                if (list[i] == id) {
                    return util::make_optional(i);
                }
            }

            return util::nullopt;
        };

        /* Iterate over all entries. */
        for (auto &entry : *m_kvs) {
            ContentMetaReader reader(entry.GetValuePointer(), entry.GetValueSize());

            /* Check if any of this entry's content infos matches one of the content ids for lookup. */
            /* If they do, then the content id isn't orphaned. */
            for (size_t i = 0; i < reader.GetContentCount(); i++) {
                if (auto found = IsOrphanedContent(content_ids, reader.GetContentInfo(i)->GetId()); found) {
                    out_orphaned[*found] = false;
                }
            }
        }

        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::Commit() {
        R_TRY(this->EnsureEnabled());

        /* Save and commit. */
        R_TRY(m_kvs->Save());
        R_RETURN(fs::CommitSaveData(m_mount_name));
    }

    Result ContentMetaDatabaseImpl::HasContent(sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id) {
        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Optimistically suppose that we will find the content. */
        out.SetValue(true);

        /* Check if any content infos contain a matching id. */
        for (size_t i = 0; i < reader.GetContentCount(); i++) {
            R_SUCCEED_IF(content_id == reader.GetContentInfo(i)->GetId());
        }

        /* We didn't find a content info. */
        out.SetValue(false);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::ListContentMetaInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, s32 offset) {
        R_UNLESS(offset >= 0, ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Read content meta infos from the given offset up to the given count. */
        size_t count;
        for (count = 0; count < out_meta_info.GetSize() && count + offset < reader.GetContentMetaCount(); count++) {
            out_meta_info[count] = *reader.GetContentMetaInfo(count + offset);
        }

        /* Set the ouput value. */
        out_entries_written.SetValue(count);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetAttributes(sf::Out<u8> out_attributes, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Set the ouput value. */
        out_attributes.SetValue(reader.GetHeader()->attributes);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Get the required version. */
        u32 required_version;
        switch (key.type) {
            case ContentMetaType::AddOnContent:
                required_version = reader.GetExtendedHeader<AddOnContentMetaExtendedHeader>()->required_application_version;
                break;
            case ContentMetaType::Application:
                /* As of 9.0.0, applications can be dependent on a specific base application version. */
                AMS_ABORT_UNLESS(hos::GetVersion() >= hos::Version_9_0_0);
                required_version = reader.GetExtendedHeader<ApplicationMetaExtendedHeader>()->required_application_version;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Set the ouput value. */
        out_version.SetValue(required_version);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) {
        R_RETURN(this->GetContentIdImpl(out_content_id.GetPointer(), key, type, util::make_optional(id_offset)));
    }

    Result ContentMetaDatabaseImpl::GetCount(sf::Out<u32> out_count) {
        R_TRY(this->EnsureEnabled());
        out_count.SetValue(m_kvs->GetCount());
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetOwnerApplicationId(sf::Out<ApplicationId> out_id, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        /* Ensure this type of key has an owner. */
        R_UNLESS(key.type == ContentMetaType::Application || key.type == ContentMetaType::Patch || key.type == ContentMetaType::AddOnContent, ncm::ResultInvalidContentMetaKey());

        /* Applications are their own owner. */
        if (key.type == ContentMetaType::Application) {
            out_id.SetValue({key.id});
            R_SUCCEED();
        }

        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Get the owner application id. */
        ApplicationId owner_application_id;
        switch (key.type) {
            case ContentMetaType::Patch:
                owner_application_id = reader.GetExtendedHeader<PatchMetaExtendedHeader>()->application_id;
                break;
            case ContentMetaType::AddOnContent:
                owner_application_id = reader.GetExtendedHeader<AddOnContentMetaExtendedHeader>()->application_id;
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Set the output value. */
        out_id.SetValue(owner_application_id);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetContentAccessibilities(sf::Out<u8> out_accessibilities, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        /* Ensure this type of key is for an add-on content. */
        R_UNLESS(key.type == ContentMetaType::AddOnContent, ncm::ResultInvalidContentMetaKey());

        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Set the ouput value. */
        out_accessibilities.SetValue(reader.GetExtendedHeader<AddOnContentMetaExtendedHeader>()->content_accessibilities);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::GetContentInfoByType(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type) {
        R_RETURN(this->GetContentInfoImpl(out_content_info.GetPointer(), key, type, util::nullopt));
    }

    Result ContentMetaDatabaseImpl::GetContentInfoByTypeAndIdOffset(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type, u8 id_offset) {
        R_RETURN(this->GetContentInfoImpl(out_content_info.GetPointer(), key, type, util::make_optional(id_offset)));
    }

    Result ContentMetaDatabaseImpl::GetPlatform(sf::Out<ncm::ContentMetaPlatform> out, const ContentMetaKey &key) {
        R_TRY(this->EnsureEnabled());

        /* Obtain the content meta for the key. */
        const void *meta;
        size_t meta_size;
        R_TRY(this->GetContentMetaPointer(&meta, &meta_size, key));

        /* Create a reader. */
        ContentMetaReader reader(meta, meta_size);

        /* Set the ouput value. */
        out.SetValue(reader.GetHeader()->platform);
        R_SUCCEED();
    }

    Result ContentMetaDatabaseImpl::HasAttributes(sf::Out<u8> out, u8 attribute_mask) {
        R_TRY(this->EnsureEnabled());

        /* Create a variable to hold the combined attributes. */
        u8 combined_attributes = 0;

        /* Iterate over all entries. */
        for (auto &entry : *m_kvs) {
            /* Obtain the content meta for the key. */
            const void *meta;
            size_t meta_size;
            R_TRY(this->GetContentMetaPointer(&meta, &meta_size, entry.GetKey()));

            /* Create a reader. */
            ContentMetaReader reader(meta, meta_size);

            /* Accumulate the set attributes from the current entry. */
            combined_attributes |= (reader.GetHeader()->attributes) & attribute_mask;

            /* If all the attributes we're looking for have been found, we're done. */
            if ((combined_attributes & attribute_mask) == attribute_mask) {
                break;
            }
        }

        /* Set the output. */
        *out = combined_attributes;
        R_SUCCEED();
    }

}
