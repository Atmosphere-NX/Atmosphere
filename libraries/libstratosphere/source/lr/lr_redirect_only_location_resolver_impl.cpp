/*
 * Copyright (c) Atmosphère-NX
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
        RedirectionAttributes attr;
        R_UNLESS(m_program_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id), lr::ResultProgramNotFound());
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectProgramPath(const Path &path, ncm::ProgramId id) {
        m_program_redirector.SetRedirection(id, path, DefaultRedirectionAttributes);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) {
        RedirectionAttributes attr;
        R_UNLESS(m_app_control_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id), lr::ResultControlNotFound());
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
        RedirectionAttributes attr;
        R_UNLESS(m_html_docs_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id), lr::ResultHtmlDocumentNotFound());
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveDataPath(sf::Out<Path> out, ncm::DataId id) {
        AMS_UNUSED(out, id);
        R_THROW(lr::ResultDataNotFound());
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_app_control_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_app_control_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_html_docs_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_html_docs_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) {
        RedirectionAttributes attr;
        R_UNLESS(m_legal_info_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id), lr::ResultLegalInformationNotFound());
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_legal_info_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_legal_info_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::Refresh() {
        this->ClearRedirections();
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_program_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_program_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::ClearApplicationRedirectionDeprecated() {
        this->ClearRedirections(RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) {
        this->ClearRedirections(excluding_ids.GetPointer(), excluding_ids.GetSize());
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::EraseProgramRedirection(ncm::ProgramId id) {
        m_program_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::EraseApplicationControlRedirection(ncm::ProgramId id) {
        m_app_control_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) {
        m_html_docs_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::EraseApplicationLegalInformationRedirection(ncm::ProgramId id) {
        m_legal_info_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) {
        /* If a debug program redirection is present, use it. */
        RedirectionAttributes attr;
        R_SUCCEED_IF(m_debug_program_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id));

        /* Otherwise, try to find a normal program redirection. */
        R_UNLESS(m_program_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id), lr::ResultDebugProgramNotFound());

        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
        m_debug_program_redirector.SetRedirection(id, path, DefaultRedirectionAttributes);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) {
        m_debug_program_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_debug_program_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::EraseProgramRedirectionForDebug(ncm::ProgramId id) {
        m_debug_program_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result RedirectOnlyLocationResolverImpl::Disable() {
        R_SUCCEED();
    }

}
