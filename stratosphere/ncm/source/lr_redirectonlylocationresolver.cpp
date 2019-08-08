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

#include "debug.hpp"

namespace sts::lr {

    RedirectOnlyLocationResolverInterface::~RedirectOnlyLocationResolverInterface() {
        this->program_redirector.ClearRedirections();
        this->debug_program_redirector.ClearRedirections();
        this->app_control_redirector.ClearRedirections();
        this->html_docs_redirector.ClearRedirections();
        this->legal_info_redirector.ClearRedirections();
    }

    Result RedirectOnlyLocationResolverInterface::ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (this->program_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrProgramNotFound;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (this->app_control_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrControlNotFound;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (this->html_docs_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrHtmlDocumentNotFound;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        return ResultLrDataNotFound;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->app_control_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->html_docs_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (this->legal_info_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrLegalInformationNotFound;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->legal_info_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::Refresh() {
        R_DEBUG_START
        this->program_redirector.ClearRedirections();
        this->debug_program_redirector.ClearRedirections();
        this->app_control_redirector.ClearRedirections();
        this->html_docs_redirector.ClearRedirections();
        this->legal_info_redirector.ClearRedirections();
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::ClearApplicationRedirection() {
        R_DEBUG_START
        this->program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->debug_program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->app_control_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->html_docs_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->legal_info_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::EraseProgramRedirection(ncm::TitleId tid) {
        R_DEBUG_START
        this->program_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationControlRedirection(ncm::TitleId tid) {
        R_DEBUG_START
        this->app_control_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid) {
        R_DEBUG_START
        this->html_docs_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationLegalInformationRedirection(ncm::TitleId tid) {
        R_DEBUG_START
        this->legal_info_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
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
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->debug_program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->debug_program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result RedirectOnlyLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::TitleId tid) {
        R_DEBUG_START
        this->debug_program_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

}