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
#include <stratosphere.hpp>
#include "lr_registered_location_resolver_impl.hpp"

namespace ams::lr {

    namespace {

        template<size_t N>
        bool ResolvePath(Path *out, const LocationRedirector &redirector, const RegisteredLocations<ncm::ProgramId, N> &locations, ncm::ProgramId id) {
            /* Attempt to use a redirection if present. */
            if (!redirector.FindRedirection(out, id)) {
                /* Otherwise try and use a registered location. */
                if (!locations.Find(out, id)) {
                    return false;
                }
            }
            return true;
        }

        template<size_t N>
        void RegisterPath(RegisteredLocations<ncm::ProgramId, N> &locations, ncm::ProgramId id, const Path& path, ncm::ProgramId owner_id) {
            /* If we register successfully, we're good. */
            if (locations.Register(id, path, owner_id)) {
                return;
            }

            /* Otherwise, clear and register (this should always succeed). */
            locations.Clear();
            locations.Register(id, path, owner_id);
        }

    }

    RegisteredLocationResolverImpl::~RegisteredLocationResolverImpl() {
        /* Ensure entries are deallocated */
        this->ClearRedirections();
    }

    /* Helper function. */
    void RegisteredLocationResolverImpl::ClearRedirections(u32 flags) {
        m_html_docs_redirector.ClearRedirections(flags);
        m_program_redirector.ClearRedirections(flags);
    }

    Result RegisteredLocationResolverImpl::RefreshImpl(const ncm::ProgramId *excluding_ids, size_t num_ids) {
        /* On < 9.0.0, exclusion lists were not supported yet, so simply clear and return. */
        if (hos::GetVersion() < hos::Version_9_0_0) {
            this->ClearRedirections();
            return ResultSuccess();
        }

        if (num_ids) {
            /* If we have exclusion lists, explicitly clear our locations. */
            m_registered_program_locations.ClearExcluding(excluding_ids, num_ids);
            m_registered_html_docs_locations.ClearExcluding(excluding_ids, num_ids);
        } else {
            /* If we don't, just perform a general clear (as pre 9.0.0 did). */
            this->ClearRedirections();
        }

        /* Clear redirectors using exclusion lists. */
        m_program_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
        m_html_docs_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(ResolvePath(out.GetPointer(), m_program_redirector, m_registered_program_locations, id), lr::ResultProgramNotFound());
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::RegisterProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        RegisterPath(m_registered_program_locations, id, path, ncm::InvalidProgramId);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        RegisterPath(m_registered_program_locations, id, path, owner_id);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::UnregisterProgramPath(ncm::ProgramId id) {
        m_registered_program_locations.Unregister(id);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::RedirectProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_program_redirector.SetRedirection(id, owner_id, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::ResolveHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(ResolvePath(out.GetPointer(), m_html_docs_redirector, m_registered_html_docs_locations, id), lr::ResultHtmlDocumentNotFound());
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::RegisterHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        RegisterPath(m_registered_html_docs_locations, id, path, ncm::InvalidProgramId);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        RegisterPath(m_registered_html_docs_locations, id, path, owner_id);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::UnregisterHtmlDocumentPath(ncm::ProgramId id) {
        m_registered_html_docs_locations.Unregister(id);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::RedirectHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_html_docs_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_html_docs_redirector.SetRedirection(id, owner_id, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverImpl::Refresh() {
        return this->RefreshImpl(nullptr, 0);
    }

    Result RegisteredLocationResolverImpl::RefreshExcluding(const sf::InArray<ncm::ProgramId> &ids) {
        return this->RefreshImpl(ids.GetPointer(), ids.GetSize());
    }

}
