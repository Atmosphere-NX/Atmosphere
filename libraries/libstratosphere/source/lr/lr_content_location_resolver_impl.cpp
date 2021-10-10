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

    Result ContentLocationResolverImpl::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
        /* Use a redirection if present. */
        R_SUCCEED_IF(m_program_redirector.FindRedirection(out.GetPointer(), id));

        /* Find the latest program content for the program id. */
        ncm::ContentId program_content_id;
        R_TRY_CATCH(m_content_meta_database.GetLatestProgram(std::addressof(program_content_id), id)) {
            R_CONVERT(ncm::ResultContentMetaNotFound, lr::ResultProgramNotFound())
        } R_END_TRY_CATCH;

        /* Obtain the content path. */
        this->GetContentStoragePath(out.GetPointer(), program_content_id);

        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectProgramPath(const Path &path, ncm::ProgramId id) {
        m_program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(m_app_control_redirector.FindRedirection(out.GetPointer(), id), lr::ResultControlNotFound());
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(m_html_docs_redirector.FindRedirection(out.GetPointer(), id), lr::ResultHtmlDocumentNotFound());
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::ResolveDataPath(sf::Out<Path> out, ncm::DataId id) {
        /* Find the latest data content for the program id. */
        ncm::ContentId data_content_id;
        R_TRY(m_content_meta_database.GetLatestData(std::addressof(data_content_id), id));

        /* Obtain the content path. */
        this->GetContentStoragePath(out.GetPointer(), data_content_id);

        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_app_control_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_app_control_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_html_docs_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_html_docs_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(m_legal_info_redirector.FindRedirection(out.GetPointer(), id), lr::ResultLegalInformationNotFound());
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_legal_info_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_legal_info_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
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

        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        m_program_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_program_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::ClearApplicationRedirectionDeprecated() {
        this->ClearRedirections(RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) {
        this->ClearRedirections(excluding_ids.GetPointer(), excluding_ids.GetSize());
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::EraseProgramRedirection(ncm::ProgramId id) {
        m_program_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::EraseApplicationControlRedirection(ncm::ProgramId id) {
        m_app_control_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) {
        m_html_docs_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::EraseApplicationLegalInformationRedirection(ncm::ProgramId id) {
        m_legal_info_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) {
        /* Use a redirection if present. */
        R_SUCCEED_IF(m_debug_program_redirector.FindRedirection(out.GetPointer(), id));

        /* Otherwise find the path for the program id. */
        R_TRY_CATCH(this->ResolveProgramPath(out.GetPointer(), id)) {
            R_CONVERT(ResultProgramNotFound, lr::ResultDebugProgramNotFound())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
        m_debug_program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) {
        m_debug_program_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        m_debug_program_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverImpl::EraseProgramRedirectionForDebug(ncm::ProgramId id) {
        m_debug_program_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

}
