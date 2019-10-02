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
        if (this->GetRedirectedPath(out.pointer, &this->program_redirector, tid)) {
            return ResultSuccess;
        }

        return ResultLrProgramNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        this->program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (this->GetRedirectedPath(out.pointer, &this->app_control_redirector, tid)) {
            return ResultSuccess;
        }

        return ResultLrControlNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (this->GetRedirectedPath(out.pointer, &this->html_docs_redirector, tid)) {
            return ResultSuccess;
        }

        return ResultLrHtmlDocumentNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        return ResultLrDataNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationControlPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->app_control_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->app_control_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->html_docs_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->html_docs_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (this->GetRedirectedPath(out.pointer, &this->legal_info_redirector, tid)) {
            return ResultSuccess;
        }

        return ResultLrLegalInformationNotFound;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationLegalInformationPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->legal_info_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->legal_info_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
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

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->program_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::ClearApplicationRedirectionDeprecated() {
        this->program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->debug_program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->app_control_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->html_docs_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->legal_info_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::ClearApplicationRedirection(InBuffer<ncm::TitleId> excluding_tids) {
        this->ClearRedirections(excluding_tids.buffer, excluding_tids.num_elements);
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
        if (this->GetRedirectedPath(out.pointer, &this->debug_program_redirector, tid)) {
            return ResultSuccess;
        }

        R_TRY_CATCH(this->ResolveProgramPath(out.pointer, tid)) {
            R_CATCH(ResultLrProgramNotFound) {
                return ResultLrDebugProgramNotFound;
            }
        } R_END_TRY_CATCH;
        
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) {
        this->debug_program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPathForDebugDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->debug_program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->debug_program_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result RedirectOnlyLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::TitleId tid) {
        this->debug_program_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

}
