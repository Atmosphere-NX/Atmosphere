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

namespace ams::lr::impl {

    enum RedirectionFlags {
        RedirectionFlags_None        = (0 << 0),
        RedirectionFlags_Application = (1 << 0),
    };

    class LocationRedirection;

    class LocationRedirector {
        NON_COPYABLE(LocationRedirector);
        NON_MOVEABLE(LocationRedirector);
        private:
            ams::util::IntrusiveListBaseTraits<LocationRedirection>::ListType redirection_list;
        public:
            LocationRedirector() { /* ... */ }

            bool FindRedirection(Path *out, ncm::ProgramId title_id);
            void SetRedirection(ncm::ProgramId title_id, const Path &path, u32 flags = RedirectionFlags_None);
            void SetRedirection(ncm::ProgramId title_id, ncm::ProgramId owner_tid, const Path &path, u32 flags = RedirectionFlags_None);
            void SetRedirectionFlags(ncm::ProgramId title_id, u32 flags);
            void EraseRedirection(ncm::ProgramId title_id);
            void ClearRedirections(u32 flags = RedirectionFlags_None);
            void ClearRedirections(const ncm::ProgramId* excluding_tids, size_t num_tids);
    };

}
