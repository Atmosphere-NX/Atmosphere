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

/* TODO: Properly integrate NCM api into libstratosphere to avoid linker hack. */
namespace ams::ncm::impl {

    Result OpenContentMetaDatabase(std::shared_ptr<ncm::IContentMetaDatabase> *, ncm::StorageId);
    Result OpenContentStorage(std::shared_ptr<ncm::IContentStorage> *, ncm::StorageId);

}

namespace ams::lr {

    ContentLocationResolverInterface::~ContentLocationResolverInterface() {
        this->ClearRedirections();
    }

    /* Helper function. */
    void ContentLocationResolverInterface::GetContentStoragePath(Path* out, ncm::ContentId content_id) {
        R_ABORT_UNLESS(this->content_storage->GetPath(out, content_id));
    }

    Result ContentLocationResolverInterface::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
        /* Use a redirection if present. */
        R_UNLESS(!this->program_redirector.FindRedirection(out.GetPointer(), id), ResultSuccess());

        /* Find the latest program content for the program id. */
        ncm::ContentId program_content_id;
        R_TRY_CATCH(this->content_meta_database->GetLatestProgram(&program_content_id, id)) {
            R_CONVERT(ncm::ResultContentMetaNotFound, lr::ResultProgramNotFound())
        } R_END_TRY_CATCH;

        /* Obtain the content path. */
        this->GetContentStoragePath(out.GetPointer(), program_content_id);

        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectProgramPath(const Path &path, ncm::ProgramId id) {
        this->program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(this->app_control_redirector.FindRedirection(out.GetPointer(), id), lr::ResultControlNotFound());
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(this->html_docs_redirector.FindRedirection(out.GetPointer(), id), lr::ResultHtmlDocumentNotFound());
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveDataPath(sf::Out<Path> out, ncm::ProgramId id) {
        /* Find the latest data content for the program id. */
        ncm::ContentId data_content_id;
        R_TRY(this->content_meta_database->GetLatestData(&data_content_id, id));

        /* Obtain the content path. */
        this->GetContentStoragePath(out.GetPointer(), data_content_id);

        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->app_control_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->app_control_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->html_docs_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->html_docs_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) {
        R_UNLESS(this->legal_info_redirector.FindRedirection(out.GetPointer(), id), lr::ResultLegalInformationNotFound());
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->legal_info_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->legal_info_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::Refresh() {
        /* Obtain Content Meta Database and Content Storage objects for this resolver's storage. */
        std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
        std::shared_ptr<ncm::IContentStorage> content_storage;
        R_TRY(ncm::impl::OpenContentMetaDatabase(&content_meta_database, this->storage_id));
        R_TRY(ncm::impl::OpenContentStorage(&content_storage, this->storage_id));

        /* Store the acquired objects. */
        this->content_meta_database = std::move(content_meta_database);
        this->content_storage = std::move(content_storage);

        /* Remove any existing redirections. */
        this->ClearRedirections();

        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->program_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->program_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirectionDeprecated() {
        this->ClearRedirections(RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) {
        this->ClearRedirectionsExcludingOwners(excluding_ids.GetPointer(), excluding_ids.GetSize());
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseProgramRedirection(ncm::ProgramId id) {
        this->program_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseApplicationControlRedirection(ncm::ProgramId id) {
        this->app_control_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) {
        this->html_docs_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseApplicationLegalInformationRedirection(ncm::ProgramId id) {
        this->legal_info_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) {
        /* Use a redirection if present. */
        R_UNLESS(!this->debug_program_redirector.FindRedirection(out.GetPointer(), id), ResultSuccess());

        /* Otherwise find the path for the program id. */
        R_TRY_CATCH(this->ResolveProgramPath(out.GetPointer(), id)) {
            R_CONVERT(ResultProgramNotFound, lr::ResultDebugProgramNotFound())
        } R_END_TRY_CATCH;

        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
        this->debug_program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) {
        this->debug_program_redirector.SetRedirection(id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->debug_program_redirector.SetRedirection(id, owner_id, path, RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::ProgramId id) {
        this->debug_program_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

}
