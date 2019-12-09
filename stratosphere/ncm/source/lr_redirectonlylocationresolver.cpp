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

namespace ams::lr {

    RedirectOnlyLocationResolverInterface::~RedirectOnlyLocationResolverInterface() {
        this->program_redirector.ClearRedirections();
        this->debug_program_redirector.ClearRedirections();
        this->app_control_redirector.ClearRedirections();
        this->html_docs_redirector.ClearRedirections();
        this->legal_info_redirector.ClearRedirections();
    }

    Result RedirectOnlyLocationResolverInterface::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->program_redirector, tid)) {
            return ResultSuccess();
        }

        return ResultProgramNotFound();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectProgramPath(const Path &path, ncm::ProgramId tid) {
        this->program_redirector.SetRedirection(tid, path);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->app_control_redirector, tid)) {
            return ResultSuccess();
        }

        return ResultControlNotFound();
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->html_docs_redirector, tid)) {
            return ResultSuccess();
        }

        return ResultHtmlDocumentNotFound();
    }

    Result RedirectOnlyLocationResolverInterface::ResolveDataPath(sf::Out<Path> out, ncm::ProgramId tid) {
        return ResultDataNotFound();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->app_control_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationControlPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->app_control_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->html_docs_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->html_docs_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->legal_info_redirector, tid)) {
            return ResultSuccess();
        }

        return ResultLegalInformationNotFound();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->legal_info_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->legal_info_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::Refresh() {
        this->program_redirector.ClearRedirections();
        this->debug_program_redirector.ClearRedirections();
        this->app_control_redirector.ClearRedirections();
        this->html_docs_redirector.ClearRedirections();
        this->legal_info_redirector.ClearRedirections();
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->program_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->program_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::ClearApplicationRedirectionDeprecated() {
        this->program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->debug_program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->app_control_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->html_docs_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->legal_info_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_tids) {
        this->ClearRedirections(excluding_tids.GetPointer(), excluding_tids.GetSize());
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::EraseProgramRedirection(ncm::ProgramId tid) {
        this->program_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationControlRedirection(ncm::ProgramId tid) {
        this->app_control_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationHtmlDocumentRedirection(ncm::ProgramId tid) {
        this->html_docs_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::EraseApplicationLegalInformationRedirection(ncm::ProgramId tid) {
        this->legal_info_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->debug_program_redirector, tid)) {
            return ResultSuccess();
        }

        R_TRY_CATCH(this->ResolveProgramPath(out.GetPointer(), tid)) {
            R_CATCH(ResultProgramNotFound) {
                return ResultDebugProgramNotFound();
            }
        } R_END_TRY_CATCH;
        
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectProgramPathForDebug(const Path &path, ncm::ProgramId tid) {
        this->debug_program_redirector.SetRedirection(tid, path);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId tid) {
        this->debug_program_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->debug_program_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::ProgramId tid) {
        this->debug_program_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

}
