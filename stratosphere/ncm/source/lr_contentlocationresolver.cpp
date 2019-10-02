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

namespace sts::lr {

    ContentLocationResolverInterface::~ContentLocationResolverInterface() {
        this->ClearRedirections();
    }

    void ContentLocationResolverInterface::GetContentStoragePath(Path* out, ncm::ContentId content_id) {
        R_ASSERT(this->content_storage->GetPath(out, content_id));
    }

    Result ContentLocationResolverInterface::ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (this->GetRedirectedPath(out.pointer, &this->program_redirector, tid)) {
            return ResultSuccess;
        }

        ncm::ContentId program_content_id;

        R_TRY_CATCH(this->content_meta_database->GetLatestProgram(&program_content_id, tid)) {
            R_CATCH(ResultNcmContentMetaNotFound) {
                return ResultLrProgramNotFound;
            }
        } R_END_TRY_CATCH;
        
        this->GetContentStoragePath(out.pointer, program_content_id);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        this->program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (this->GetRedirectedPath(out.pointer, &this->app_control_redirector, tid)) {
            return ResultSuccess;
        }

        return ResultLrControlNotFound;
    }

    Result ContentLocationResolverInterface::ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (this->GetRedirectedPath(out.pointer, &this->html_docs_redirector, tid)) {
            return ResultSuccess;
        }

        return ResultLrHtmlDocumentNotFound;
    }

    Result ContentLocationResolverInterface::ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        ncm::ContentId data_content_id;

        R_TRY(this->content_meta_database->GetLatestData(&data_content_id, tid));
        this->GetContentStoragePath(out.pointer, data_content_id);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->app_control_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->app_control_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->html_docs_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->html_docs_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        if (this->GetRedirectedPath(out.pointer, &this->legal_info_redirector, tid)) {
            return ResultSuccess;
        }

        return ResultLrLegalInformationNotFound;
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->legal_info_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->legal_info_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::Refresh() {
        std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
        std::shared_ptr<ncm::IContentStorage> content_storage;

        R_TRY(ncm::impl::OpenContentMetaDatabase(&content_meta_database, this->storage_id));
        R_TRY(ncm::impl::OpenContentStorage(&content_storage, this->storage_id));
        this->content_meta_database = std::move(content_meta_database);
        this->content_storage = std::move(content_storage);
        this->ClearRedirections();

        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->program_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirectionDeprecated() {
        this->ClearRedirections(impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirection(InBuffer<ncm::TitleId> excluding_tids) {
        this->ClearRedirections(excluding_tids.buffer, excluding_tids.num_elements);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::EraseProgramRedirection(ncm::TitleId tid) {
        this->program_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::EraseApplicationControlRedirection(ncm::TitleId tid) {
        this->app_control_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid) {
        this->html_docs_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::EraseApplicationLegalInformationRedirection(ncm::TitleId tid) {
        this->legal_info_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
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

    Result ContentLocationResolverInterface::RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) {
        this->debug_program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebugDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        this->debug_program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        this->debug_program_redirector.SetRedirection(tid, title_id_2, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
    }

    Result ContentLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::TitleId tid) {
        this->debug_program_redirector.EraseRedirection(tid);
        return ResultSuccess;
    }

}
