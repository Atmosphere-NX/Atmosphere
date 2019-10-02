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

namespace sts::lr::impl {

    enum RedirectionFlags {
        RedirectionFlags_None        = (0 << 0),
        RedirectionFlags_Application = (1 << 0),
    };

    class LocationRedirection;

    class LocationRedirector {
        NON_COPYABLE(LocationRedirector);
        NON_MOVEABLE(LocationRedirector);
        private:
            sts::util::IntrusiveListBaseTraits<LocationRedirection>::ListType redirection_list;
        public:
            LocationRedirector() { /* ... */ }

            bool FindRedirection(Path *out, ncm::TitleId title_id);
            void SetRedirection(ncm::TitleId title_id, const Path &path, u32 flags = RedirectionFlags_None);
            void SetRedirection(ncm::TitleId title_id, ncm::TitleId title_id_2, const Path &path, u32 flags = RedirectionFlags_None);
            void SetRedirectionFlags(ncm::TitleId title_id, u32 flags);
            void EraseRedirection(ncm::TitleId title_id);
            void ClearRedirections(u32 flags = RedirectionFlags_None);
            void ClearRedirections(const ncm::TitleId* excluding_tids, size_t num_tids);
    };

}
