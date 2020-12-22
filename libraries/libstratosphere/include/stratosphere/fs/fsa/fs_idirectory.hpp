/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "../fs_common.hpp"
#include "../fs_directory.hpp"

namespace ams::fs::fsa {

    class IDirectory {
        public:
            virtual ~IDirectory() { /* ... */ }

            Result Read(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries) {
                R_UNLESS(out_count != nullptr, fs::ResultNullptrArgument());
                if (max_entries == 0) {
                    *out_count = 0;
                    return ResultSuccess();
                }
                R_UNLESS(out_entries != nullptr, fs::ResultNullptrArgument());
                R_UNLESS(max_entries > 0, fs::ResultInvalidArgument());
                return this->DoRead(out_count, out_entries, max_entries);
            }

            Result GetEntryCount(s64 *out) {
                R_UNLESS(out != nullptr, fs::ResultNullptrArgument());
                return this->DoGetEntryCount(out);
            }
        public:
            /* TODO: This is a hack to allow the mitm API to work. Find a better way? */
            virtual sf::cmif::DomainObjectId GetDomainObjectId() const = 0;
        protected:
            /* ...? */
        private:
            virtual Result DoRead(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries) = 0;
            virtual Result DoGetEntryCount(s64 *out) = 0;
    };

}
