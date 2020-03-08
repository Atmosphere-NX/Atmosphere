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
#include <stratosphere.hpp>

namespace ams::lr {

    enum RedirectionFlags {
        RedirectionFlags_None           = (0 << 0),
        RedirectionFlags_Application    = (1 << 0),
    };

    class LocationRedirector {
        NON_COPYABLE(LocationRedirector);
        NON_MOVEABLE(LocationRedirector);
        private:
            class Redirection;
        private:
            using RedirectionList = ams::util::IntrusiveListBaseTraits<Redirection>::ListType;
        private:
            RedirectionList redirection_list;
        public:
            LocationRedirector() { /* ... */ }
            ~LocationRedirector() { this->ClearRedirections(); }

            /* API. */
            bool FindRedirection(Path *out, ncm::ProgramId program_id) const;
            void SetRedirection(ncm::ProgramId program_id, const Path &path, u32 flags = RedirectionFlags_None);
            void SetRedirection(ncm::ProgramId program_id, ncm::ProgramId owner_id, const Path &path, u32 flags = RedirectionFlags_None);
            void SetRedirectionFlags(ncm::ProgramId program_id, u32 flags);
            void EraseRedirection(ncm::ProgramId program_id);
            void ClearRedirections(u32 flags = RedirectionFlags_None);
            void ClearRedirectionsExcludingOwners(const ncm::ProgramId *excluding_ids, size_t num_ids);
        private:
            inline bool IsExcluded(const ncm::ProgramId id, const ncm::ProgramId *excluding_ids, size_t num_ids) const {
                for (size_t i = 0; i < num_ids; i++) {
                    if (id == excluding_ids[i]) {
                        return true;
                    }
                }

                return false;
            }
    };

}
