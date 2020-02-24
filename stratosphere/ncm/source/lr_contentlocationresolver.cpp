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
#include "lr_contentlocationresolver.hpp"

namespace ams::lr {

    ContentLocationResolverInterface::~ContentLocationResolverInterface() {
        this->ClearRedirections();
    }

    void ContentLocationResolverInterface::GetContentStoragePath(Path* out, ncm::ContentId content_id) {
        R_ABORT_UNLESS(this->content_storage->GetPath(out, content_id));
    }

    Result ContentLocationResolverInterface::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->program_redirector, id)) {
            return ResultSuccess();
        }

        ncm::ContentId program_content_id;

        R_TRY_CATCH(this->content_meta_database->GetLatestProgram(&program_content_id, id)) {
            R_CATCH(ncm::ResultContentMetaNotFound) {
                return ResultProgramNotFound();
            }
        } R_END_TRY_CATCH;
        
        this->GetContentStoragePath(out.GetPointer(), program_content_id);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectProgramPath(const Path &path, ncm::ProgramId id) {
        this->program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->app_control_redirector, id)) {
            return ResultSuccess();
        }

        return ResultControlNotFound();
    }

    Result ContentLocationResolverInterface::ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->html_docs_redirector, id)) {
            return ResultSuccess();
        }

        return ResultHtmlDocumentNotFound();
    }

    Result ContentLocationResolverInterface::ResolveDataPath(sf::Out<Path> out, ncm::ProgramId id) {
        ncm::ContentId data_content_id;

        R_TRY(this->content_meta_database->GetLatestData(&data_content_id, id));
        this->GetContentStoragePath(out.GetPointer(), data_content_id);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->app_control_redirector.SetRedirection(id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->app_control_redirector.SetRedirection(id, owner_id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->html_docs_redirector.SetRedirection(id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->html_docs_redirector.SetRedirection(id, owner_id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->legal_info_redirector, id)) {
            return ResultSuccess();
        }

        return ResultLegalInformationNotFound();
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->legal_info_redirector.SetRedirection(id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->legal_info_redirector.SetRedirection(id, owner_id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::Refresh() {
        std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
        std::shared_ptr<ncm::IContentStorage> content_storage;

        R_TRY(ncm::impl::OpenContentMetaDatabase(&content_meta_database, this->storage_id));
        R_TRY(ncm::impl::OpenContentStorage(&content_storage, this->storage_id));
        this->content_meta_database = std::move(content_meta_database);
        this->content_storage = std::move(content_storage);
        this->ClearRedirections();

        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
        this->program_redirector.SetRedirection(id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->program_redirector.SetRedirection(id, owner_id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirectionDeprecated() {
        this->ClearRedirections(impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) {
        this->ClearRedirections(excluding_ids.GetPointer(), excluding_ids.GetSize());
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
        if (this->GetRedirectedPath(out.GetPointer(), &this->debug_program_redirector, id)) {
            return ResultSuccess();
        }

        R_TRY_CATCH(this->ResolveProgramPath(out.GetPointer(), id)) {
            R_CATCH(ResultProgramNotFound) {
                return ResultDebugProgramNotFound();
            }
        } R_END_TRY_CATCH;
        
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
        this->debug_program_redirector.SetRedirection(id, path);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) {
        this->debug_program_redirector.SetRedirection(id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
        this->debug_program_redirector.SetRedirection(id, owner_id, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::ProgramId id) {
        this->debug_program_redirector.EraseRedirection(id);
        return ResultSuccess();
    }

}
