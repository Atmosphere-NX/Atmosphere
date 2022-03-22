/*
 * Copyright (c) Atmosphère-NX
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
            LocationRedirector m_program_redirector;
            LocationRedirector m_debug_program_redirector;
            LocationRedirector m_app_control_redirector;
            LocationRedirector m_html_docs_redirector;
            LocationRedirector m_legal_info_redirector;
        protected:
            LocationResolverImplBase() : m_program_redirector(), m_debug_program_redirector(), m_app_control_redirector(), m_html_docs_redirector(), m_legal_info_redirector() { /* ... */ }
        protected:
            /* Helper functions. */
            void ClearRedirections(u32 flags = RedirectionFlags_None) {
                m_program_redirector.ClearRedirections(flags);
                m_debug_program_redirector.ClearRedirections(flags);
                m_app_control_redirector.ClearRedirections(flags);
                m_html_docs_redirector.ClearRedirections(flags);
                m_legal_info_redirector.ClearRedirections(flags);
            }

            void ClearRedirections(const ncm::ProgramId *excluding_ids, size_t num_ids) {
                m_program_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                m_debug_program_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                m_app_control_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                m_html_docs_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                m_legal_info_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
            }
    };

}