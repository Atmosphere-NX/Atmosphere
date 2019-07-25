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

#include "impl/ncm_content_manager.hpp"
#include "lr_redirectonlylocationresolver.hpp"

namespace sts::lr {

    RedirectOnlyLocationResolverInterface::~RedirectOnlyLocationResolverInterface() {
        this->program_redirector.ClearRedirections();
        this->debug_program_redirector.ClearRedirections();
        this->app_control_redirector.ClearRedirections();
        this->html_docs_redirector.ClearRedirections();
        this->legal_info_redirector.ClearRedirections();
    }

    Result RedirectOnlyLocationResolverInterface::ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        Path path;
        
        if (this->program_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrProgramNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectProgramPath(ncm::TitleId tid, InPointer<const Path> path) {
        this->program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        Path path;
        
        if (this->app_control_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrControlNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        Path path;
        
        if (this->html_docs_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrHtmlDocumentNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        return ResultLrDataNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationControlPath(ncm::TitleId tid, InPointer<const Path> path) {
        this->app_control_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationHtmlDocumentPath(ncm::TitleId tid, InPointer<const Path> path) {
        this->html_docs_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        Path path;
        
        if (this->legal_info_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrLegalInformationNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationLegalInformationPath(ncm::TitleId tid, InPointer<const Path> path) {
        this->legal_info_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::Refresh() {
        this->program_redirector.ClearRedirections();
        this->debug_program_redirector.ClearRedirections();
        this->app_control_redirector.ClearRedirections();
        this->html_docs_redirector.ClearRedirections();
        this->legal_info_redirector.ClearRedirections();
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPath(ncm::TitleId tid, InPointer<const Path> path) {
        this->program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::ClearApplicationRedirection() {
        this->program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->debug_program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->app_control_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->html_docs_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->legal_info_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::EraseProgramRedirection(ncm::TitleId tid) {
        this->program_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationControlRedirection(ncm::TitleId tid) {
        this->app_control_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid) {
        this->html_docs_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationLegalInformationRedirection(ncm::TitleId tid) {
        this->legal_info_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        Path path;

        if (this->debug_program_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        R_TRY_CATCH(this->ResolveProgramPath(&path, tid)) {
            R_CATCH(ResultLrProgramNotFound) {
                return ResultLrDebugProgramNotFound;
            }
        } R_END_TRY_CATCH;
        
        *out.pointer = path;
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectProgramPathForDebug(ncm::TitleId tid, InPointer<const Path> path) {
        this->debug_program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPathForDebug(ncm::TitleId tid, InPointer<const Path> path) {
        this->debug_program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::TitleId tid) {
        this->debug_program_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

}