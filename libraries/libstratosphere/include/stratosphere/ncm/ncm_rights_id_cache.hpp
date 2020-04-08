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
#include <stratosphere/os.hpp>
#include <stratosphere/ncm/ncm_rights_id.hpp>
#include <stratosphere/ncm/ncm_content_id.hpp>

namespace ams::ncm {

    class RightsIdCache {
        NON_COPYABLE(RightsIdCache);
        NON_MOVEABLE(RightsIdCache);
        private:
            static constexpr size_t MaxEntries = 0x80;
        private:
            struct Entry {
                public:
                    util::Uuid uuid;
                    ncm::RightsId rights_id;
                    u64 last_accessed;
            };
        private:
            Entry entries[MaxEntries];
            u64 counter;
            os::Mutex mutex;
        public:
            RightsIdCache() : mutex(false) {
                this->Invalidate();
            }

            void Invalidate() {
                this->counter = 2;
                for (size_t i = 0; i < MaxEntries; i++) {
                    this->entries[i].last_accessed = 1;
                }
            }

            void Store(ContentId content_id, ncm::RightsId rights_id) {
                std::scoped_lock lk(this->mutex);
                Entry *eviction_candidate = &this->entries[0];

                /* Find a suitable existing entry to store our new one at. */
                for (size_t i = 1; i < MaxEntries; i++) {
                    Entry *entry = &this->entries[i];

                    /* Change eviction candidates if the uuid already matches ours, or if the uuid doesn't already match and the last_accessed count is lower */
                    if (content_id == entry->uuid || (content_id != eviction_candidate->uuid && entry->last_accessed < eviction_candidate->last_accessed)) {
                        eviction_candidate = entry;
                    }
                }

                /* Update the cache. */
                eviction_candidate->uuid = content_id.uuid;
                eviction_candidate->rights_id = rights_id;
                eviction_candidate->last_accessed = this->counter++;
            }

            bool Find(ncm::RightsId *out_rights_id, ContentId content_id) {
                std::scoped_lock lk(this->mutex);

                /* Attempt to locate the content id in the cache. */
                for (size_t i = 0; i < MaxEntries; i++) {
                    Entry *entry = &this->entries[i];

                    if (entry->last_accessed != 1 && content_id == entry->uuid) {
                        entry->last_accessed = this->counter;
                        this->counter++;
                        *out_rights_id = entry->rights_id;
                        return true;
                    }
                }

                return false;
            }
    };

}
