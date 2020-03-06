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
#include <stratosphere.hpp>
#include "ncm_on_memory_content_meta_database_impl.hpp"

namespace ams::ncm {

    Result OnMemoryContentMetaDatabaseImpl::List(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type) {
        /* NOTE: This function is *almost* identical to the ContentMetaDatabaseImpl equivalent. */
        /* The only difference is that the min max comparison is exclusive for OnMemoryContentMetaDatabaseImpl, */
        /* but inclusive for ContentMetaDatabaseImpl. This may or may not be a Nintendo bug? */
        R_TRY(this->EnsureEnabled());

        size_t entries_total = 0;
        size_t entries_written = 0;

        /* Iterate over all entries. */
        for (auto entry = this->kvs->begin(); entry != this->kvs->end(); entry++) {
            const ContentMetaKey key = entry->GetKey();

            /* Check if this entry matches the given filters. */
            if (!((meta_type == ContentMetaType::Unknown || key.type == meta_type) && (min < key.id && key.id < max) && (install_type == ContentInstallType::Unknown || key.install_type == install_type))) {
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
        return ResultSuccess();
    }

    Result OnMemoryContentMetaDatabaseImpl::GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, u64 id) {
        R_TRY(this->EnsureEnabled());

        std::optional<ContentMetaKey> found_key = std::nullopt;

        /* Find the last key with the desired program id. */
        for (auto entry = this->kvs->lower_bound(ContentMetaKey::MakeUnknownType(id, 0)); entry != this->kvs->end(); entry++) {
            /* No further entries will match the program id, discontinue. */
            if (entry->GetKey().id != id) {
                break;
            }

            /* On memory content database is interested in all keys. */
            found_key = entry->GetKey();
        }

        /* Check if the key is absent. */
        R_UNLESS(found_key, ncm::ResultContentMetaNotFound());

        out_key.SetValue(*found_key);
        return ResultSuccess();
    }

    Result OnMemoryContentMetaDatabaseImpl::LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) {
        return ResultInvalidContentMetaDatabase();
    }

    Result OnMemoryContentMetaDatabaseImpl::Commit() {
        R_TRY(this->EnsureEnabled());
        return ResultSuccess();
    }

}
