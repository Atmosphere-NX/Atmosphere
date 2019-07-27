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
            impl::LocationRedirector program_redirector;
            impl::LocationRedirector debug_program_redirector;
            impl::LocationRedirector app_control_redirector;
            impl::LocationRedirector html_docs_redirector;
            impl::LocationRedirector legal_info_redirector;
        public:
            virtual Result ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result Refresh();
            virtual Result RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result ClearApplicationRedirection();
            virtual Result EraseProgramRedirection(ncm::TitleId tid);
            virtual Result EraseApplicationControlRedirection(ncm::TitleId tid);
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid);
            virtual Result EraseApplicationLegalInformationRedirection(ncm::TitleId tid);
            virtual Result ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result EraseProgramRedirectionForDebug(ncm::TitleId tid);
            
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {};
    };

}