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

    Result OnMemoryContentMetaDatabaseImpl::GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, ProgramId program_id) {
        R_TRY(this->EnsureEnabled());

        std::optional<ContentMetaKey> found_key;

        /* Find the last key with the desired program id. */
        for (auto entry = this->kvs->lower_bound(ContentMetaKey::Make(program_id, 0, ContentMetaType::Unknown)); entry != this->kvs->end(); entry++) {
            /* No further entries will match the program id, discontinue. */
            if (entry->GetKey().id != program_id) {
                break;
            }

            found_key = entry->GetKey();
        }
        R_UNLESS(found_key, ncm::ResultContentMetaNotFound());

        *out_key = *found_key;
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
