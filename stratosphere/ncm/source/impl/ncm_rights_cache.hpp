/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphere-NX
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
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::ncm::impl {

    class RightsIdCache {
        public:
            static constexpr size_t MaxEntries = 0x80;
        public:
            struct Entry {
                public:
                    util::Uuid uuid;
                    FsRightsId rights_id;
                    u64 key_generation;
                    u64 last_accessed;
            };

            Entry entries[MaxEntries];
            u64 counter;
            os::Mutex mutex;

            RightsIdCache() {
                this->Invalidate();
            }

            void Invalidate() {
                this->counter = 2;
                for (size_t i = 0; i < MaxEntries; i++) {
                    this->entries[i].last_accessed = 1;
                }
            }
    };

}
