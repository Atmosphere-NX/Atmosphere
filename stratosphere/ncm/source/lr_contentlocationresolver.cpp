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
        R_ASSERT(this->content_storage->GetPath(out, content_id));
    }

    Result ContentLocationResolverInterface::ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->program_redirector, tid)) {
            return ResultSuccess();
        }

        ncm::ContentId program_content_id;

        R_TRY_CATCH(this->content_meta_database->GetLatestProgram(&program_content_id, tid)) {
            R_CATCH(ncm::ResultContentMetaNotFound) {
                return ResultProgramNotFound();
            }
        } R_END_TRY_CATCH;
        
        this->GetContentStoragePath(out.GetPointer(), program_content_id);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectProgramPath(const Path &path, ncm::ProgramId tid) {
        this->program_redirector.SetRedirection(tid, path);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->app_control_redirector, tid)) {
            return ResultSuccess();
        }

        return ResultControlNotFound();
    }

    Result ContentLocationResolverInterface::ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->html_docs_redirector, tid)) {
            return ResultSuccess();
        }

        return ResultHtmlDocumentNotFound();
    }

    Result ContentLocationResolverInterface::ResolveDataPath(sf::Out<Path> out, ncm::ProgramId tid) {
        ncm::ContentId data_content_id;

        R_TRY(this->content_meta_database->GetLatestData(&data_content_id, tid));
        this->GetContentStoragePath(out.GetPointer(), data_content_id);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->app_control_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->app_control_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->html_docs_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->html_docs_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId tid) {
        if (this->GetRedirectedPath(out.GetPointer(), &this->legal_info_redirector, tid)) {
            return ResultSuccess();
        }

        return ResultLegalInformationNotFound();
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->legal_info_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->legal_info_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
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

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId tid) {
        this->program_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->program_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirectionDeprecated() {
        this->ClearRedirections(impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_tids) {
        this->ClearRedirections(excluding_tids.GetPointer(), excluding_tids.GetSize());
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseProgramRedirection(ncm::ProgramId tid) {
        this->program_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseApplicationControlRedirection(ncm::ProgramId tid) {
        this->app_control_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseApplicationHtmlDocumentRedirection(ncm::ProgramId tid) {
        this->html_docs_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseApplicationLegalInformationRedirection(ncm::ProgramId tid) {
        this->legal_info_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId tid) {
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

    Result ContentLocationResolverInterface::RedirectProgramPathForDebug(const Path &path, ncm::ProgramId tid) {
        this->debug_program_redirector.SetRedirection(tid, path);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId tid) {
        this->debug_program_redirector.SetRedirection(tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) {
        this->debug_program_redirector.SetRedirection(tid, owner_tid, path, impl::RedirectionFlags_Application);
        return ResultSuccess();
    }

    Result ContentLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::ProgramId tid) {
        this->debug_program_redirector.EraseRedirection(tid);
        return ResultSuccess();
    }

}
