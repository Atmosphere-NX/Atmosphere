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
#include <stratosphere.hpp>

namespace ams::lr {

    namespace {

        template<size_t N>
        bool ResolvePath(Path *out, const LocationRedirector &redirector, const RegisteredLocations<ncm::ProgramId, N> &locations, ncm::ProgramId id) {
            if (!redirector.FindRedirection(out, id)) {
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

    RegisteredLocationResolverInterface::~RegisteredLocationResolverInterface() {
        /* Ensure entries are deallocated */
        this->ClearRedirections();
    }

    void RegisteredLocationResolverInterface::ClearRedirections(u32 flags) {
        this->html_docs_redirector.ClearRedirections(flags);
        this->program_redirector.ClearRedirections(flags);
    }

    Result RegisteredLocationResolverInterface::RefreshImpl(const ncm::ProgramId* excluding_ids, size_t num_ids) {
        /* On < 9.0.0, exclusion lists were not supported yet, so simply clear and return. */
        if (hos::GetVersion() < hos::Version_900) {
            this->ClearRedirections();
            return ResultSuccess();
        }

        if (num_ids) {
            /* If we have exclusion lists, explicitly clear our locations. */
            this->registered_program_locations.ClearExcluding(excluding_ids, num_ids);
            this->registered_html_docs_locations.ClearExcluding(excluding_ids, num_ids);
        } else {
            /* If we don't, just perform a general clear (as pre 9.0.0 did). */
            this->ClearRedirections();
        }

        /* Clear redirectors using exclusion lists. */
        this->program_redirector.ClearRedirections(excluding_ids, num_ids);
        this->html_docs_redirector.ClearRedirections(excluding_ids, num_ids);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(ResolvePath(out.GetPointer(), this->program_redirector, this->registered_program_locations, id), lr::ResultProgramNotFound());
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RegisterProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        RegisterPath(this->registered_program_locations, id, path, ncm::ProgramId::Invalid);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        RegisterPath(this->registered_program_locations, id, path, owner_id);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::UnregisterProgramPath(ncm::ProgramId id) {
        this->registered_program_locations.Unregister(id);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RedirectProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->program_redirector.SetRedirection(id, owner_id, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::ResolveHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(ResolvePath(out.GetPointer(), this->html_docs_redirector, this->registered_html_docs_locations, id), lr::ResultHtmlDocumentNotFound());
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RegisterHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        RegisterPath(this->registered_html_docs_locations, id, path, ncm::ProgramId::Invalid);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        RegisterPath(this->registered_html_docs_locations, id, path, owner_id);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::UnregisterHtmlDocumentPath(ncm::ProgramId id) {
        this->registered_html_docs_locations.Unregister(id);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RedirectHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->html_docs_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->html_docs_redirector.SetRedirection(id, owner_id, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::Refresh() {
        return this->RefreshImpl(nullptr, 0);
    }

    Result RegisteredLocationResolverInterface::RefreshExcluding(const sf::InArray<ncm::ProgramId> &ids) {
        return this->RefreshImpl(ids.GetPointer(), ids.GetSize());
    }

}
