/*
 * Copyright (c) 2019 Adubbz
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

#include "../ncm_types.hpp"

namespace sts::ncm::impl {

    class RightsIdCache {
        public:
            static constexpr size_t MaxEntries = 0x80;
        public:
            struct Entry {
                public:
                    Uuid uuid;
                    FsRightsId rights_id;
                    u64 key_generation;
                    u64 last_accessed = 1;
            };

            Entry entries[MaxEntries];
            u64 counter = 2;
            HosMutex mutex;
    };

    RightsIdCache* GetRightsIdCache();

}