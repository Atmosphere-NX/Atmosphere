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
        this->html_docs_redirector.ClearRedirections();
        this->program_redirector.ClearRedirections();
    }

    Result RegisteredLocationResolverInterface::ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (!this->program_redirector.FindRedirection(out.pointer, tid)) {
            if (!this->registered_program_locations.Find(out.pointer, tid)) {
                return ResultLrProgramNotFound;
            }
        }
        
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RegisterProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        const Path& tmp_path = *path.pointer;

        if (!this->registered_program_locations.Register(tid, tmp_path)) {
            this->registered_program_locations.Clear();
            this->registered_program_locations.Register(tid, tmp_path);
        }
        
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::UnregisterProgramPath(ncm::TitleId tid) {
        this->registered_program_locations.Unregister(tid);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        this->program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::ResolveHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (!this->html_docs_redirector.FindRedirection(out.pointer, tid)) {
            if (!this->registered_html_docs_locations.Find(out.pointer, tid)) {
                return ResultLrProgramNotFound;
            }
        }
        
        return ResultLrHtmlDocumentNotFound;
    }

    Result RegisteredLocationResolverInterface::RegisterHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid) {
        const Path& tmp_path = *path.pointer;

        if (!this->registered_html_docs_locations.Register(tid, tmp_path)) {
            this->registered_html_docs_locations.Clear();
            this->registered_html_docs_locations.Register(tid, tmp_path);
        }
        
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::UnregisterHtmlDocumentPath(ncm::TitleId tid) {
        this->registered_html_docs_locations.Unregister(tid);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::RedirectHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid) {
        this->html_docs_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result RegisteredLocationResolverInterface::Refresh() {
        this->registered_program_locations.Clear();
        this->registered_html_docs_locations.Clear();
        return ResultSuccess;
    }

}
