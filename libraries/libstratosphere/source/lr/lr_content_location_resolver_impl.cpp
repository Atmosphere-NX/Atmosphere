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
#include "lr_content_location_resolver_impl.hpp"

namespace ams::lr {

    ContentLocationResolverImpl::~ContentLocationResolverImpl() {
        this->ClearRedirections();
    }

    /* Helper function. */
    void ContentLocationResolverImpl::GetContentStoragePath(Path *out, ncm::ContentId content_id) {
        static_assert(sizeof(lr::Path) == sizeof(ncm::Path));
        m_content_storage.GetPath(reinterpret_cast<ncm::Path *>(out), content_id);
    }

    Result ContentLocationResolverImpl::ResolveProgramPath(Path *out, RedirectionAttributes *out_attr, ncm::ProgramId id) {
        /* Use a redirection if present. */
        R_SUCCEED_IF(m_program_redirector.FindRedirection(out, out_attr, id));

        /* If we're not enabled, we can't resolve a program. */
        R_UNLESS(m_enabled, lr::ResultProgramNotFound())

        /* Find the latest program content for the program id. */
        ncm::ContentInfo program_content_info;
        R_TRY_CATCH(m_content_meta_database.GetLatestProgram(std::addressof(program_content_info), id)) {
            R_CONVERT(ncm::ResultContentMetaNotFound, lr::ResultProgramNotFound())
        } R_END_TRY_CATCH;

        /* Obtain the content path and attributes. */
        this->GetContentStoragePath(out, program_content_info.GetId());
        *out_attr = RedirectionAttributes::Make(program_content_info.GetContentAttributes());

        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
        RedirectionAttributes attr;
        R_RETURN(this->ResolveProgramPath(out.GetPointer(), std::addressof(attr), id));
    }

    Result ContentLocationResolverImpl::RedirectProgramPath(const Path &path, ncm::ProgramId id) {
        m_program_redirector.SetRedirection(id, path, DefaultRedirectionAttributes);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) {
        RedirectionAttributes attr;
        R_UNLESS(m_app_control_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id), lr::ResultControlNotFound());
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
        RedirectionAttributes attr;
        R_UNLESS(m_html_docs_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id), lr::ResultHtmlDocumentNotFound());
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::ResolveDataPath(sf::Out<Path> out, ncm::DataId id) {
        /* If we're not enabled, we can't resolve data. */
        R_UNLESS(m_enabled, lr::ResultDataNotFound())

        /* Find the latest data content info for the program id. */
        ncm::ContentInfo data_content_info;
        R_TRY(m_content_meta_database.GetLatestData(std::addressof(data_content_info), id));

        /* Obtain the content path. */
        this->GetContentStoragePath(out.GetPointer(), data_content_info.GetId());

        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_app_control_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_app_control_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_html_docs_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_html_docs_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) {
        RedirectionAttributes attr;
        R_UNLESS(m_legal_info_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id), lr::ResultLegalInformationNotFound());
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_legal_info_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_legal_info_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::Refresh() {
        /* Obtain content meta database and content storage objects for this resolver's storage. */
        ncm::ContentMetaDatabase meta_db;
        ncm::ContentStorage storage;
        R_TRY(ncm::OpenContentMetaDatabase(std::addressof(meta_db), m_storage_id));
        R_TRY(ncm::OpenContentStorage(std::addressof(storage), m_storage_id));

        /* Store the acquired objects. */
        m_content_meta_database = std::move(meta_db);
        m_content_storage       = std::move(storage);

        /* Remove any existing redirections. */
        this->ClearRedirections();

        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_program_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_program_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::ClearApplicationRedirectionDeprecated() {
        this->ClearRedirections(RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) {
        this->ClearRedirections(excluding_ids.GetPointer(), excluding_ids.GetSize());
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::EraseProgramRedirection(ncm::ProgramId id) {
        m_program_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::EraseApplicationControlRedirection(ncm::ProgramId id) {
        m_app_control_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) {
        m_html_docs_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::EraseApplicationLegalInformationRedirection(ncm::ProgramId id) {
        m_legal_info_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) {
        /* Use a redirection if present. */
        RedirectionAttributes attr;
        R_SUCCEED_IF(m_debug_program_redirector.FindRedirection(out.GetPointer(), std::addressof(attr), id));

        /* If we're not enabled, we can't resolve a program. */
        R_UNLESS(m_enabled, lr::ResultDebugProgramNotFound())

        /* Otherwise find the path for the program id. */
        R_TRY_CATCH(this->ResolveProgramPath(out.GetPointer(), std::addressof(attr), id)) {
            R_CONVERT(lr::ResultProgramNotFound, lr::ResultDebugProgramNotFound())
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
        m_debug_program_redirector.SetRedirection(id, path, DefaultRedirectionAttributes);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) {
        m_debug_program_redirector.SetRedirection(id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_debug_program_redirector.SetRedirection(id, owner_id, path, DefaultRedirectionAttributes, RedirectionFlags_Application);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::EraseProgramRedirectionForDebug(ncm::ProgramId id) {
        m_debug_program_redirector.EraseRedirection(id);
        R_SUCCEED();
    }

    Result ContentLocationResolverImpl::Disable() {
        m_enabled = false;
        R_SUCCEED();
    }

}
