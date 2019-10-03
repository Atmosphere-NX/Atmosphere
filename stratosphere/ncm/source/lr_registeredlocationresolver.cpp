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

namespace sts::lr {

    RegisteredLocationResolverInterface::~RegisteredLocationResolverInterface() {
        /* Ensure entries are deallocated */
        this->ClearRedirections();
    }

    void RegisteredLocationResolverInterface::ClearRedirections(u32 flags) {
        this->html_docs_redirector.ClearRedirections();
        this->program_redirector.ClearRedirections();
    }

    void RegisteredLocationResolverInterface::RegisterPath(const Path& path, impl::RegisteredLocations<ncm::TitleId, RegisteredLocationResolverInterface::MaxRegisteredLocations>* locations, ncm::TitleId tid, ncm::TitleId owner_tid) {
        if (!locations->Register(tid, path, owner_tid)) {
            locations->Clear();
            locations->Register(tid, path, owner_tid);
        }
    }

    bool RegisteredLocationResolverInterface::ResolvePath(Path* out, impl::LocationRedirector* redirector, impl::RegisteredLocations<ncm::TitleId, RegisteredLocationResolverInterface::MaxRegisteredLocations>* locations, ncm::TitleId tid) {
        if (!redirector->FindRedirection(out, tid)) {
            if (!locations->Find(out, tid)) {
                return false;
            }
        }
        return true;
    }

    Result RegisteredLocationResolverInterface::RefreshImpl(const ncm::TitleId* excluding_tids, size_t num_tids) {
        if (GetRuntimeFirmwareVersion() < FirmwareVersion_900) {
            this->ClearRedirections();
            return ResultSuccess;
        }

        if (num_tids == 0) {
            this->ClearRedirections();
        } else {
            this->registered_program_locations.ClearExcluding(excluding_tids, num_tids);
            this->registered_html_docs_locations.ClearExcluding(excluding_tids, num_tids);
        }

        this->program_redirector.ClearRedirections(excluding_tids, num_tids);
        this->html_docs_redirector.ClearRedirections(excluding_tids, num_tids);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (!this->ResolvePath(out.pointer, &this->program_redirector, &this->registered_program_locations, tid)) {
            return ResultLrProgramNotFound;
        }

        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RegisterProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->RegisterPath(*path.pointer, &this->registered_program_locations, tid, ncm::TitleId::Invalid);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RegisterProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) {
        this->RegisterPath(*path.pointer, &this->registered_program_locations, tid, owner_tid);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::UnregisterProgramPath(ncm::TitleId tid) {
        this->registered_program_locations.Unregister(tid);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RedirectProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) {
        this->program_redirector.SetRedirection(tid, owner_tid, *path.pointer);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::ResolveHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (!this->ResolvePath(out.pointer, &this->html_docs_redirector, &this->registered_html_docs_locations, tid)) {
            return ResultLrHtmlDocumentNotFound;
        }
        
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RegisterHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->RegisterPath(*path.pointer, &this->registered_html_docs_locations, tid, ncm::TitleId::Invalid);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RegisterHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) {
        this->RegisterPath(*path.pointer, &this->registered_html_docs_locations, tid, owner_tid);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::UnregisterHtmlDocumentPath(ncm::TitleId tid) {
        this->registered_html_docs_locations.Unregister(tid);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RedirectHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->html_docs_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RedirectHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) {
        this->html_docs_redirector.SetRedirection(tid, owner_tid, *path.pointer);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::Refresh() {
        return this->RefreshImpl(nullptr, 0);
    }

    Result RegisteredLocationResolverInterface::RefreshExcluding(InBuffer<ncm::TitleId> tids) {
        return this->RefreshImpl(tids.buffer, tids.num_elements);
    }

}
