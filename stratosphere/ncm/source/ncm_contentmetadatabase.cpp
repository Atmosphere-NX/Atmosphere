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

#include "ncm_contentmetadatabase.hpp"
#include "ncm_utils.hpp"

namespace ams::ncm {

    namespace {

        struct ContentMetaHeader {
            u16 extended_header_size;
            u16 content_count;
            u16 content_meta_count;
            ContentMetaAttribute attributes;
            StorageId storage_id;
        };

        static_assert(sizeof(ContentMetaHeader) == 0x8, "ContentMetaHeader definition!");

        struct ApplicationMetaExtendedHeader {
            ProgramId patch_id;
            u32 required_system_version;
            u32 required_application_version;
        };

        struct PatchMetaExtendedHeader {
            ProgramId application_id;
            u32 required_system_version;
            u32 extended_data_size;
            u8 reserved[0x8];
        };

        struct AddOnContentMetaExtendedHeader {
            ProgramId application_id;
            u32 required_application_version;
            u32 padding;
        };

        struct SystemUpdateMetaExtendedHeader {
            u32 extended_data_size;
        };

        inline const ContentMetaHeader* GetValueHeader(const void* value) {
            return reinterpret_cast<const ContentMetaHeader*>(value);
        }

        template<typename ExtendedHeaderType>
        inline const ExtendedHeaderType* GetValueExtendedHeader(const void* value) {
            return reinterpret_cast<const ExtendedHeaderType*>(reinterpret_cast<const u8*>(value) + sizeof(ContentMetaHeader));
        }

        inline const ContentInfo* GetValueContentInfos(const void* value) {
            return reinterpret_cast<const ContentInfo*>(reinterpret_cast<const u8*>(value) + sizeof(ContentMetaHeader) + GetValueHeader(value)->extended_header_size);
        }

        inline const ContentMetaInfo* GetValueContentMetaInfos(const void* value) {
            auto header = GetValueHeader(value);
            return reinterpret_cast<const ContentMetaInfo*>(reinterpret_cast<const u8*>(GetValueContentInfos(value)) + sizeof(ContentInfo) * header->content_count);
        }

        Result GetContentMetaSize(size_t *out, const ContentMetaKey &key, const kvdb::MemoryKeyValueStore<ContentMetaKey> *kvs) {
            R_TRY_CATCH(kvs->GetValueSize(out, key)) {
                R_CONVERT(kvdb::ResultKeyNotFound, ncm::ResultContentMetaNotFound())
            } R_END_TRY_CATCH;
            
            return ResultSuccess();
        }

        Result GetContentMetaValuePointer(const void **out_value_ptr, size_t *out_size, const ContentMetaKey &key, const kvdb::MemoryKeyValueStore<ContentMetaKey> *kvs) {
            R_TRY(GetContentMetaSize(out_size, key, kvs));
            R_TRY(kvs->GetValuePointer(out_value_ptr, key));
            return ResultSuccess();
        }

    }

    Result ContentMetaDatabaseInterface::GetContentIdByTypeImpl(ContentId* out, const ContentMetaKey& key, ContentType type, std::optional<u8> id_offset) {
        R_TRY(this->EnsureEnabled());

        const auto it = this->kvs->lower_bound(key);
        R_UNLESS(it != this->kvs->end(), ncm::ResultContentMetaNotFound());
        R_UNLESS(it->GetKey().id == key.id, ncm::ResultContentMetaNotFound());

        const auto stored_key = it->GetKey();
        const void* value = nullptr;
        size_t value_size = 0;

        R_TRY(GetContentMetaValuePointer(&value, &value_size, stored_key, this->kvs));
        const auto header = GetValueHeader(value);

        R_UNLESS(header->content_count != 0, ncm::ResultContentNotFound());

        const ContentInfo* content_infos = GetValueContentInfos(value);
        const ContentInfo* found_content_info = nullptr;

        if (id_offset) {
            for (size_t i = 0; i < header->content_count; i++) {
                const ContentInfo* content_info = &content_infos[i];

                if (content_info->content_type == type && content_info->id_offset == *id_offset) {
                    found_content_info = content_info;
                    break;
                }
            }
        } else {
            const ContentInfo* lowest_id_offset_info = nullptr;

            for (size_t i = 0; i < header->content_count; i++) {
                const ContentInfo* content_info = &content_infos[i];

                if (content_info->content_type == type && (!lowest_id_offset_info || lowest_id_offset_info->id_offset > content_info->id_offset)) {
                    lowest_id_offset_info = content_info;
                }
            }

            found_content_info = lowest_id_offset_info;
        }

        R_UNLESS(found_content_info, ncm::ResultContentNotFound());
        *out = found_content_info->content_id;
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetLatestContentMetaKeyImpl(ContentMetaKey* out_key, ProgramId id) {
        R_TRY(this->EnsureEnabled());
        
        ContentMetaKey key = {0};
        key.id = id;

        bool found_key = false;
        for (auto entry = this->kvs->lower_bound(key); entry != this->kvs->end(); entry++) {
            if (entry->GetKey().id != key.id) {
                break;
            }

            if (entry->GetKey().install_type == ContentInstallType::Full) {
                key = entry->GetKey();
                found_key = true;
            }
        }

        R_UNLESS(found_key, ncm::ResultContentMetaNotFound());
        *out_key = key;
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::Set(ContentMetaKey key, sf::InBuffer value) {
        R_TRY(this->EnsureEnabled());
        R_TRY(this->kvs->Set(key, value.GetPointer(), value.GetSize()));
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::Get(sf::Out<u64> out_size, ContentMetaKey key, sf::OutBuffer out_value) {
        R_TRY(this->EnsureEnabled());
        R_TRY(this->kvs->Get(out_size.GetPointer(), out_value.GetPointer(), out_value.GetSize(), key));
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::Remove(ContentMetaKey key) {
        R_TRY(this->EnsureEnabled());
        R_TRY(this->kvs->Remove(key));
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetContentIdByType(sf::Out<ContentId> out_content_id, ContentMetaKey key, ContentType type) {
        ContentId content_id;
        R_TRY(this->GetContentIdByTypeImpl(&content_id, key, type, std::nullopt));
        out_content_id.SetValue(content_id);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::ListContentInfo(sf::Out<u32> out_count, const sf::OutArray<ContentInfo> &out_info, ContentMetaKey key, u32 offset) {
        R_UNLESS(offset <= std::numeric_limits<s32>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());

        const void *value = nullptr;
        size_t value_size = 0;
        R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));
        const auto header = GetValueHeader(value);
        const auto content_infos = GetValueContentInfos(value);
        
        size_t count;
        for (count = 0; offset + count < header->content_count && count < out_info.GetSize(); count++) {
            out_info[count] = content_infos[offset + count];
        }

        out_count.SetValue(count);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::List(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType type, ProgramId application_program_id, ProgramId program_id_min, ProgramId program_id_max, ContentInstallType install_type) {
        R_TRY(this->EnsureEnabled());
        
        size_t entries_total = 0;
        size_t entries_written = 0;

        /* If there are no entries then we've already successfully listed them all. */
        if (this->kvs->GetCount() == 0) {
            out_entries_total.SetValue(entries_total);
            out_entries_written.SetValue(entries_written);
            return ResultSuccess();
        }

        for (auto entry = this->kvs->begin(); entry != this->kvs->end(); entry++) {
            ContentMetaKey key = entry->GetKey();

            /* Check if this entry matches the given filters. */
            if (!((static_cast<u8>(type) == 0 || key.type == type) && (program_id_min <= key.id && key.id <= program_id_max) && (key.install_type == ContentInstallType::Unknown || key.install_type == install_type))) {
                continue;
            }

            if (static_cast<u64>(application_program_id) != 0) {
                const void* value = nullptr;
                size_t value_size = 0;
                R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));

                /* Each of these types are owned by an application. We need to check if their owner application matches the filter. */
                if (key.type == ContentMetaType::Application || key.type == ContentMetaType::Patch || key.type == ContentMetaType::AddOnContent || key.type == ContentMetaType::Delta) {
                    ProgramId entry_application_id = key.id;
                    
                    switch (key.type) {
                        case ContentMetaType::Application:
                            break;
                        default:
                            /* The first u64 of all non-application extended headers is the application program id. */
                            entry_application_id = *GetValueExtendedHeader<ProgramId>(value);
                    }

                    /* Application id doesn't match filter, skip this entry. */
                    if (entry_application_id != application_program_id) {
                        continue;
                    }
                }
            }

            /* Write the entry to the output buffer. */
            if (entries_written < out_info.GetSize()) {
                out_info[entries_written] = key;
                entries_written++;
            }

            entries_total++;
        }

        out_entries_total.SetValue(entries_total);
        out_entries_written.SetValue(entries_written);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId program_id) {   
        R_TRY(this->EnsureEnabled());
        ContentMetaKey key;        
        R_TRY(this->GetLatestContentMetaKeyImpl(&key, program_id));
        out_key.SetValue(key);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::ListApplication(sf::Out<u32> out_entries_total, sf::Out<u32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType type) {
        R_TRY(this->EnsureEnabled());
        
        size_t entries_total = 0;
        size_t entries_written = 0;

        /* If there are no entries then we've already successfully listed them all. */
        if (this->kvs->GetCount() == 0) {
            out_entries_total.SetValue(entries_total);
            out_entries_written.SetValue(entries_written);
            return ResultSuccess();
        }

        for (auto entry = this->kvs->begin(); entry != this->kvs->end(); entry++) {
            ContentMetaKey key = entry->GetKey();

            /* Check if this entry matches the given filters. */
            if (!((static_cast<u8>(type) == 0 || key.type == type))) {
                continue;
            }

            const void* value = nullptr;
            size_t value_size = 0;
            R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));

            if (key.type == ContentMetaType::Application || key.type == ContentMetaType::Patch || key.type == ContentMetaType::AddOnContent || key.type == ContentMetaType::Delta) {
                ProgramId application_id = key.id;
                
                switch (key.type) {
                    case ContentMetaType::Application:
                        break;
                    default:
                        /* The first u64 of all non-application extended headers is the application program id. */
                        application_id = *GetValueExtendedHeader<ProgramId>(value);
                }

                /* Write the entry to the output buffer. */
                if (entries_written < out_keys.GetSize()) {
                    ApplicationContentMetaKey* out_app_key = &out_keys[entries_written];
                    out_app_key->application_program_id = application_id;
                    out_app_key->key = key;
                    entries_written++;
                }

                entries_total++;
            }
        }

        out_entries_total.SetValue(entries_total);
        out_entries_written.SetValue(entries_written);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::Has(sf::Out<bool> out, ContentMetaKey key) {
        R_TRY(this->EnsureEnabled());

        if (this->kvs->GetCount() == 0) {
            out.SetValue(false);
            return ResultSuccess();
        }

        bool has = false;
        const auto it = this->kvs->lower_bound(key);
        if (it != this->kvs->end()) {
            has = it->GetKey() == key;
        }

        out.SetValue(has);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) {
        R_TRY(this->EnsureEnabled());

        bool has = true;
        for (size_t i = 0; i < keys.GetSize() && has; i++) {
            R_TRY(this->Has(&has, keys[i]));
        }

        out.SetValue(has);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetSize(sf::Out<u64> out_size, ContentMetaKey key) {
        R_TRY(this->EnsureEnabled());
        R_UNLESS(this->kvs->GetCount() != 0, ncm::ResultContentMetaNotFound());

        const auto it = this->kvs->lower_bound(key);
        R_UNLESS(it != this->kvs->end(), ncm::ResultContentMetaNotFound());
        R_UNLESS(it->GetKey() == key, ncm::ResultContentMetaNotFound());

        out_size.SetValue(it->GetValueSize());
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetRequiredSystemVersion(sf::Out<u32> out_version, ContentMetaKey key) {
        R_TRY(this->EnsureEnabled());
        R_UNLESS(key.type == ContentMetaType::Application || key.type == ContentMetaType::Patch, ncm::ResultInvalidContentMetaKey());

        const void* value = nullptr;
        size_t value_size = 0;
        R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));

        /* Required system version is at the same offset for the patch and application extended header.
           We use the application header for convenience. */
        const auto ext_header = GetValueExtendedHeader<ApplicationMetaExtendedHeader>(value);
        out_version.SetValue(ext_header->required_system_version);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetPatchId(sf::Out<ProgramId> out_patch_id, ContentMetaKey key) {
        R_TRY(this->EnsureEnabled());
        R_UNLESS(key.type != ContentMetaType::Application, ncm::ResultInvalidContentMetaKey());

        const void* value = nullptr;
        size_t value_size = 0;
        R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));
        const auto ext_header = GetValueExtendedHeader<ApplicationMetaExtendedHeader>(value);

        out_patch_id.SetValue(ext_header->patch_id);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::DisableForcibly() {
        this->disabled = true;
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) {
        R_TRY(this->EnsureEnabled());
        R_UNLESS(out_orphaned.GetSize() >= content_ids.GetSize(), ncm::ResultBufferInsufficient());

        /* Default to orphaned for all content ids. */
        if (out_orphaned.GetSize() > 0) {
            std::fill_n(out_orphaned.GetPointer(), out_orphaned.GetSize(), true);
        }
        
        R_UNLESS(this->kvs->GetCount() != 0, ResultSuccess());
        
        for (auto entry = this->kvs->begin(); entry != this->kvs->end(); entry++) {
            const auto value = entry->GetValuePointer();
            const auto header = GetValueHeader(value);

            if (header->content_count == 0) {
                continue;
            }

            if (content_ids.GetSize() == 0) {
                continue;
            }

            const ContentInfo* content_infos = GetValueContentInfos(value);
            for (size_t i = 0; i < header->content_count; i++) {
                const ContentInfo* content_info = &content_infos[i];

                /* Check if any of this entry's content infos matches one of the provided content ids.
                   If they do, then the content id isn't orphaned. */
                for (size_t j = 0; j < content_ids.GetSize(); j++) {
                    const ContentId content_id = content_ids[j];

                    if (content_id == content_info->content_id) {
                        out_orphaned[j] = false;
                        break;
                    }
                }
            }
        }

        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::Commit() {
        R_TRY(this->EnsureEnabled());
        R_TRY(this->kvs->Save());
        R_TRY(fsdevCommitDevice(this->mount_name));
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::HasContent(sf::Out<bool> out, ContentMetaKey key, ContentId content_id) {
        const void* value = nullptr;
        size_t value_size = 0;
        R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));
        const auto header = GetValueHeader(value);
        const ContentInfo* content_infos = GetValueContentInfos(value);

        if (header->content_count > 0) {
            for (size_t i = 0; i < header->content_count; i++) {
                const ContentInfo* content_info = &content_infos[i];

                if (content_id == content_info->content_id) {
                    out.SetValue(false);
                    return ResultSuccess();
                }
            }
        }

        out.SetValue(false);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::ListContentMetaInfo(sf::Out<u32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, ContentMetaKey key, u32 start_index) {
        R_UNLESS(start_index <= std::numeric_limits<s32>::max(), ncm::ResultInvalidOffset());
        R_TRY(this->EnsureEnabled());
        
        const void* value = nullptr;
        size_t value_size = 0;
        R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));
        const auto header = GetValueHeader(value);
        const auto content_meta_infos = GetValueContentMetaInfos(value);
        size_t entries_written = 0;

        if (out_meta_info.GetSize() == 0) {
            out_entries_written.SetValue(0);
            return ResultSuccess();
        }

        for (size_t i = start_index; i < out_meta_info.GetSize(); i++) {
            /* We have no more entries we can read out. */
            if (header->content_meta_count <= start_index + i) {
                break;
            }

            out_meta_info[i] = content_meta_infos[i];
            entries_written = i + 1;
        }

        out_entries_written.SetValue(entries_written);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetAttributes(sf::Out<ContentMetaAttribute> out_attributes, ContentMetaKey key) {
        R_TRY(this->EnsureEnabled());

        const void* value = nullptr;
        size_t value_size = 0;
        R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));
        const auto header = GetValueHeader(value);
        out_attributes.SetValue(header->attributes);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetRequiredApplicationVersion(sf::Out<u32> out_version, ContentMetaKey key) {
        R_TRY(this->EnsureEnabled());

        const void* value = nullptr;
        size_t value_size = 0;
        R_TRY(GetContentMetaValuePointer(&value, &value_size, key, this->kvs));

        /* As of 9.0.0, applications can be dependent on a specific base application version. */
        if (hos::GetVersion() >= hos::Version_900 && key.type == ContentMetaType::Application) {
            const auto ext_header = GetValueExtendedHeader<ApplicationMetaExtendedHeader>(value);
            out_version.SetValue(ext_header->required_application_version);
            return ResultSuccess();
        }

        R_UNLESS(key.type == ContentMetaType::AddOnContent, ncm::ResultInvalidContentMetaKey());
        const auto ext_header = GetValueExtendedHeader<AddOnContentMetaExtendedHeader>(value);
        out_version.SetValue(ext_header->required_application_version);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, ContentMetaKey key, ContentType type, u8 id_offset) {
        ContentId content_id;
        R_TRY(this->GetContentIdByTypeImpl(&content_id, key, type, std::optional(id_offset)));
        out_content_id.SetValue(content_id);
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetLatestProgram(ContentId* out_content_id, ProgramId program_id) {
        ContentMetaKey key;
        
        R_TRY(this->GetLatestContentMetaKey(&key, program_id));
        R_TRY(this->GetContentIdByType(out_content_id, key, ContentType::Program));
        return ResultSuccess();
    }

    Result ContentMetaDatabaseInterface::GetLatestData(ContentId* out_content_id, ProgramId program_id) {
        ContentMetaKey key;
        
        R_TRY(this->GetLatestContentMetaKey(&key, program_id));
        R_TRY(this->GetContentIdByType(out_content_id, key, ContentType::Data));
        return ResultSuccess();
    }

    Result OnMemoryContentMetaDatabaseInterface::GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId program_id) {
        R_TRY(this->EnsureEnabled());

        const ContentMetaKey key = ContentMetaKey::Make(program_id, 0, ContentMetaType::Unknown);
        
        std::optional<ContentMetaKey> found_key;
        for (auto entry = this->kvs->lower_bound(key); entry != this->kvs->end(); entry++) {
            if (entry->GetKey().id != program_id) {
                break;
            }

            found_key = entry->GetKey();
        }

        R_UNLESS(found_key, ncm::ResultContentMetaNotFound());
        *out_key = *found_key;
        return ResultSuccess();
    }

    Result OnMemoryContentMetaDatabaseInterface::LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) {
        return ResultInvalidContentMetaDatabase();
    }

    Result OnMemoryContentMetaDatabaseInterface::Commit() {
        R_TRY(this->EnsureEnabled());
        return ResultSuccess();
    }

}
