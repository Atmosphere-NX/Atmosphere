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
#include "lr_redirect_only_location_resolver_impl.hpp"

namespace ams::lr {

    RedirectOnlyLocationResolverImpl::~RedirectOnlyLocationResolverImpl() {
        /* Ensure entries are deallocated */
        this->ClearRedirections();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(this->program_redirector.FindRedirection(out.GetPointer(), id), lr::ResultProgramNotFound());
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectProgramPath(const Path &path, ncm::ProgramId id) {
        this->program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(this->app_control_redirector.FindRedirection(out.GetPointer(), id), lr::ResultControlNotFound());
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(this->html_docs_redirector.FindRedirection(out.GetPointer(), id), lr::ResultHtmlDocumentNotFound());
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveDataPath(sf::Out<Path> out, ncm::DataId id) {
        return ResultDataNotFound();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->app_control_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->app_control_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->html_docs_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->html_docs_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(this->legal_info_redirector.FindRedirection(out.GetPointer(), id), lr::ResultLegalInformationNotFound());
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->legal_info_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->legal_info_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::Refresh() {
        this->ClearRedirections();
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->program_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->program_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::ClearApplicationRedirectionDeprecated() {
        this->ClearRedirections(RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) {
        this->ClearRedirections(excluding_ids.GetPointer(), excluding_ids.GetSize());
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::EraseProgramRedirection(ncm::ProgramId id) {
        this->program_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::EraseApplicationControlRedirection(ncm::ProgramId id) {
        this->app_control_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) {
        this->html_docs_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::EraseApplicationLegalInformationRedirection(ncm::ProgramId id) {
        this->legal_info_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) {
        /* If a debug program redirection is present, use it. */
        R_SUCCEED_IF(this->debug_program_redirector.FindRedirection(out.GetPointer(), id));

        /* Otherwise, try to find a normal program redirection. */
        R_UNLESS(this->program_redirector.FindRedirection(out.GetPointer(), id), lr::ResultDebugProgramNotFound());

        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
        this->debug_program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) {
        this->debug_program_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->debug_program_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result RedirectOnlyLocationResolverImpl::EraseProgramRedirectionForDebug(ncm::ProgramId id) {
        this->debug_program_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

}
