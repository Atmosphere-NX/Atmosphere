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

#include "debug.hpp"

namespace sts::lr {

    ContentLocationResolverInterface::~ContentLocationResolverInterface() {
        this->program_redirector.ClearRedirections();
        this->debug_program_redirector.ClearRedirections();
        this->app_control_redirector.ClearRedirections();
        this->html_docs_redirector.ClearRedirections();
        this->legal_info_redirector.ClearRedirections();
    }

    Result ContentLocationResolverInterface::ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;

        if (this->program_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        ncm::ContentId program_content_id;

        R_TRY_CATCH(this->content_meta_database->GetLatestProgram(&program_content_id, tid)) {
            R_CATCH(ResultNcmContentMetaNotFound) {
                return ResultLrProgramNotFound;
            }
        } R_END_TRY_CATCH;
        
        R_ASSERT(this->content_storage->GetPath(&path, program_content_id));
        *out.pointer = path;
        debug::DebugLog("Resolved program path to %s\n", path.path);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (this->app_control_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrControlNotFound;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (this->html_docs_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrHtmlDocumentNotFound;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        ncm::ContentId data_content_id;

        R_TRY(this->content_meta_database->GetLatestData(&data_content_id, tid));
        R_ASSERT(this->content_storage->GetPath(&path, data_content_id));
        *out.pointer = path;
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->app_control_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->html_docs_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        debug::DebugLog("Redirected application html document path to %s\n", (*path.pointer).path);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;
        
        if (this->legal_info_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        return ResultLrLegalInformationNotFound;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->legal_info_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::Refresh() {
        R_DEBUG_START
        std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
        std::shared_ptr<ncm::IContentStorage> content_storage;
        R_TRY(ncm::impl::OpenContentMetaDatabase(&content_meta_database, this->storage_id));
        R_TRY(ncm::impl::OpenContentStorage(&content_storage, this->storage_id));
        this->content_meta_database = std::move(content_meta_database);
        this->content_storage = std::move(content_storage);

        this->program_redirector.ClearRedirections();
        this->debug_program_redirector.ClearRedirections();
        this->app_control_redirector.ClearRedirections();
        this->html_docs_redirector.ClearRedirections();
        this->legal_info_redirector.ClearRedirections();

        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::ClearApplicationRedirection() {
        R_DEBUG_START
        this->program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->debug_program_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->app_control_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->html_docs_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        this->legal_info_redirector.ClearRedirections(impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::EraseProgramRedirection(ncm::TitleId tid) {
        R_DEBUG_START
        this->program_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::EraseApplicationControlRedirection(ncm::TitleId tid) {
        R_DEBUG_START
        this->app_control_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid) {
        R_DEBUG_START
        this->html_docs_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::EraseApplicationLegalInformationRedirection(ncm::TitleId tid) {
        R_DEBUG_START
        this->legal_info_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        R_DEBUG_START
        Path path;

        if (this->debug_program_redirector.FindRedirection(&path, tid)) {
            *out.pointer = path;
            return ResultSuccess;
        }

        R_TRY_CATCH(this->ResolveProgramPath(&path, tid)) {
            R_CATCH(ResultLrProgramNotFound) {
                return ResultLrDebugProgramNotFound;
            }
        } R_END_TRY_CATCH;
        
        *out.pointer = path;
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->debug_program_redirector.SetRedirection(tid, *path.pointer);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) {
        R_DEBUG_START
        this->debug_program_redirector.SetRedirection(tid, *path.pointer, impl::RedirectionFlags_Application);
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentLocationResolverInterface::EraseProgramRedirectionForDebug(ncm::TitleId tid) {
        R_DEBUG_START
        this->debug_program_redirector.EraseRedirection(tid);
        return ResultSuccess;
        R_DEBUG_END
    }

}