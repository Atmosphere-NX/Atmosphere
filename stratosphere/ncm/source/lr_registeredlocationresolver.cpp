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

#include "debug.hpp"

namespace sts::lr {

    RegisteredLocationResolverInterface::RegisteredLocationResolverInterface() {
        this->registered_program_redirector.ClearRedirections();
        this->registered_html_docs_redirector.ClearRedirections();
    }

    RegisteredLocationResolverInterface::~RegisteredLocationResolverInterface() {
        /* Ensure entries are deallocated */
        this->html_docs_redirector.ClearRedirections();
        this->program_redirector.ClearRedirections();
    }

    Result RegisteredLocationResolverInterface::ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (!this->program_redirector.FindRedirection(&path, tid)) {
            if (!this->registered_program_redirector.FindRedirection(&path, tid)) {
                return ResultLrProgramNotFound;
            }
        }
        
        *out.pointer = path;
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RegisteredLocationResolverInterface::RegisterProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        Path tmp_path = *path.pointer;

        if (!this->registered_program_redirector.SetRedirection(tid, tmp_path)) {
            this->registered_program_redirector.ClearRedirections();
            this->registered_program_redirector.SetRedirection(tid, tmp_path);
        }
        
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RegisteredLocationResolverInterface::UnregisterProgramPath(ncm::TitleId tid) {
        R_DEBUG_START
        this->registered_program_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RegisteredLocationResolverInterface::RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        Path tmp_path = *path.pointer;
        this->program_redirector.SetRedirection(tid, tmp_path);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RegisteredLocationResolverInterface::ResolveHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (!this->html_docs_redirector.FindRedirection(&path, tid)) {
            if (!this->registered_html_docs_redirector.FindRedirection(&path, tid)) {
                return ResultLrProgramNotFound;
            }
        }
        
        *out.pointer = path;
        return ResultLrHtmlDocumentNotFound;
        R_DEBUG_END
    }

    Result RegisteredLocationResolverInterface::RegisterHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        Path tmp_path = *path.pointer;

        if (!this->registered_html_docs_redirector.SetRedirection(tid, tmp_path)) {
            this->registered_html_docs_redirector.ClearRedirections();
            this->registered_html_docs_redirector.SetRedirection(tid, tmp_path);
        }
        
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RegisteredLocationResolverInterface::UnregisterHtmlDocumentPath(ncm::TitleId tid) {
        R_DEBUG_START
        this->registered_html_docs_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RegisteredLocationResolverInterface::RedirectHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        Path tmp_path = *path.pointer;
        this->html_docs_redirector.SetRedirection(tid, tmp_path);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RegisteredLocationResolverInterface::Refresh() {
        R_DEBUG_START
        this->registered_program_redirector.ClearRedirections();
        this->registered_html_docs_redirector.ClearRedirections();
        return ResultSuccess;
        R_DEBUG_END
    }

}