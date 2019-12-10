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

#include "lr_registeredlocationresolver.hpp"

namespace ams::lr {

    RegisteredLocationResolverInterface::~RegisteredLocationResolverInterface() {
        /* Ensure entries are deallocated */
        this->ClearRedirections();
    }

    void RegisteredLocationResolverInterface::ClearRedirections(u32 flags) {
        this->html_docs_redirector.ClearRedirections();
        this->program_redirector.ClearRedirections();
    }

    void RegisteredLocationResolverInterface::RegisterPath(const Path& path, impl::RegisteredLocations<ncm::ProgramId, RegisteredLocationResolverInterface::MaxRegisteredLocations>* locations, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        if (!locations->Register(tid, path, owner_tid)) {
            locations->Clear();
            locations->Register(tid, path, owner_tid);
        }
    }

    bool RegisteredLocationResolverInterface::ResolvePath(Path* out, impl::LocationRedirector* redirector, impl::RegisteredLocations<ncm::ProgramId, RegisteredLocationResolverInterface::MaxRegisteredLocations>* locations, ncm::ProgramId tid) {
        if (!redirector->FindRedirection(out, tid)) {
            if (!locations->Find(out, tid)) {
                return false;
            }
        }
        return true;
    }

    Result RegisteredLocationResolverInterface::RefreshImpl(const ncm::ProgramId* excluding_tids, size_t num_tids) {
        if (hos::GetVersion() < hos::Version_900) {
            this->ClearRedirections();
            return ResultSuccess();
        }

        if (num_tids == 0) {
            this->ClearRedirections();
        } else {
            this->registered_program_locations.ClearExcluding(excluding_tids, num_tids);
            this->registered_html_docs_locations.ClearExcluding(excluding_tids, num_tids);
        }

        this->program_redirector.ClearRedirections(excluding_tids, num_tids);
        this->html_docs_redirector.ClearRedirections(excluding_tids, num_tids);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (!this->ResolvePath(out.GetPointer(), &this->program_redirector, &this->registered_program_locations, tid)) {
            return ResultProgramNotFound();
        }

        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RegisterProgramPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->RegisterPath(path, &this->registered_program_locations, tid, ncm::ProgramId::Invalid);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RegisterProgramPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->RegisterPath(path, &this->registered_program_locations, tid, owner_tid);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::UnregisterProgramPath(ncm::ProgramId tid) {
        this->registered_program_locations.Unregister(tid);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RedirectProgramPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->program_redirector.SetRedirection(tid, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RedirectProgramPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->program_redirector.SetRedirection(tid, owner_tid, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::ResolveHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (!this->ResolvePath(out.GetPointer(), &this->html_docs_redirector, &this->registered_html_docs_locations, tid)) {
            return ResultHtmlDocumentNotFound();
        }
        
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RegisterHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->RegisterPath(path, &this->registered_html_docs_locations, tid, ncm::ProgramId::Invalid);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->RegisterPath(path, &this->registered_html_docs_locations, tid, owner_tid);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::UnregisterHtmlDocumentPath(ncm::ProgramId tid) {
        this->registered_html_docs_locations.Unregister(tid);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RedirectHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->html_docs_redirector.SetRedirection(tid, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->html_docs_redirector.SetRedirection(tid, owner_tid, path);
        return ResultSuccess();
    }

    Result RegisteredLocationResolverInterface::Refresh() {
        return this->RefreshImpl(nullptr, 0);
    }

    Result RegisteredLocationResolverInterface::RefreshExcluding(const sf::InArray<ncm::ProgramId> &tids) {
        return this->RefreshImpl(tids.GetPointer(), tids.GetSize());
    }

}
