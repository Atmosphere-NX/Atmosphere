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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "impl/lr_redirection.hpp"

namespace sts::lr {

    class ILocationResolver : public IServiceObject {
        protected:
            enum class CommandId {
                ResolveProgramPath = 0,
                RedirectProgramPath = 1,
                ResolveApplicationControlPath = 2,
                ResolveApplicationHtmlDocumentPath = 3,
                ResolveDataPath = 4,
                RedirectApplicationControlPath = 5,
                RedirectApplicationHtmlDocumentPath = 6,
                ResolveApplicationLegalInformationPath = 7,
                RedirectApplicationLegalInformationPath = 8,
                Refresh = 9,
                RedirectApplicationProgramPath = 10,
                ClearApplicationRedirection = 11,
                EraseProgramRedirection = 12,
                EraseApplicationControlRedirection = 13,
                EraseApplicationHtmlDocumentRedirection = 14,
                EraseApplicationLegalInformationRedirection = 15,
                ResolveProgramPathForDebug = 16,
                RedirectProgramPathForDebug = 17,
                RedirectApplicationProgramPathForDebug = 18,
                EraseProgramRedirectionForDebug = 19,
            };
        protected:
            impl::LocationRedirector program_redirector;
            impl::LocationRedirector debug_program_redirector;
            impl::LocationRedirector app_control_redirector;
            impl::LocationRedirector html_docs_redirector;
            impl::LocationRedirector legal_info_redirector;
        public:
            virtual Result ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) = 0;
            virtual Result RedirectProgramPath(ncm::TitleId tid, InPointer<const Path> path) = 0;
            virtual Result ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) = 0;
            virtual Result ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) = 0;
            virtual Result ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) = 0;
            virtual Result RedirectApplicationControlPath(ncm::TitleId tid, InPointer<const Path> path) = 0;
            virtual Result RedirectApplicationHtmlDocumentPath(ncm::TitleId tid, InPointer<const Path> path) = 0;
            virtual Result ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) = 0;
            virtual Result RedirectApplicationLegalInformationPath(ncm::TitleId tid, InPointer<const Path> path) = 0;
            virtual Result Refresh() = 0;
            virtual Result RedirectApplicationProgramPath(ncm::TitleId tid, InPointer<const Path> path) = 0;
            virtual Result ClearApplicationRedirection() = 0;
            virtual Result EraseProgramRedirection(ncm::TitleId tid) = 0;
            virtual Result EraseApplicationControlRedirection(ncm::TitleId tid) = 0;
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid) = 0;
            virtual Result EraseApplicationLegalInformationRedirection(ncm::TitleId tid) = 0;
            virtual Result ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) = 0;
            virtual Result RedirectProgramPathForDebug(ncm::TitleId tid, InPointer<const Path> path) = 0;
            virtual Result RedirectApplicationProgramPathForDebug(ncm::TitleId tid, InPointer<const Path> path) = 0;
            virtual Result EraseProgramRedirectionForDebug(ncm::TitleId tid) = 0;
            
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {};
    };

}