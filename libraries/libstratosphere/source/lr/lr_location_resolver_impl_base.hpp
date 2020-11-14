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
#include "lr_location_redirector.hpp"

namespace ams::lr {

    class LocationResolverImplBase {
        NON_COPYABLE(LocationResolverImplBase);
        NON_MOVEABLE(LocationResolverImplBase);
        protected:
            /* Location redirectors. */
            LocationRedirector program_redirector;
            LocationRedirector debug_program_redirector;
            LocationRedirector app_control_redirector;
            LocationRedirector html_docs_redirector;
            LocationRedirector legal_info_redirector;
        protected:
            LocationResolverImplBase() : program_redirector(), debug_program_redirector(), app_control_redirector(), html_docs_redirector(), legal_info_redirector() { /* ... */ }
        protected:
            /* Helper functions. */
            void ClearRedirections(u32 flags = RedirectionFlags_None) {
                this->program_redirector.ClearRedirections(flags);
                this->debug_program_redirector.ClearRedirections(flags);
                this->app_control_redirector.ClearRedirections(flags);
                this->html_docs_redirector.ClearRedirections(flags);
                this->legal_info_redirector.ClearRedirections(flags);
            }

            void ClearRedirections(const ncm::ProgramId *excluding_ids, size_t num_ids) {
                this->program_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                this->debug_program_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                this->app_control_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                this->html_docs_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                this->legal_info_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
            }
    };

}