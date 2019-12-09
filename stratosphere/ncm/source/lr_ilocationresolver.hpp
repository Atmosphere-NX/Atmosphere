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
                RedirectApplicationControlPathDeprecated = 5,
                RedirectApplicationControlPath = 5,
                RedirectApplicationHtmlDocumentPathDeprecated = 6,
                RedirectApplicationHtmlDocumentPath = 6,
                ResolveApplicationLegalInformationPath = 7,
                RedirectApplicationLegalInformationPathDeprecated = 8,
                RedirectApplicationLegalInformationPath = 8,
                Refresh = 9,
                RedirectApplicationProgramPathDeprecated = 10,
                RedirectApplicationProgramPath = 10,
                ClearApplicationRedirectionDeprecated = 11,
                ClearApplicationRedirection = 11,
                EraseProgramRedirection = 12,
                EraseApplicationControlRedirection = 13,
                EraseApplicationHtmlDocumentRedirection = 14,
                EraseApplicationLegalInformationRedirection = 15,
                ResolveProgramPathForDebug = 16,
                RedirectProgramPathForDebug = 17,
                RedirectApplicationProgramPathForDebugDeprecated = 18,
                RedirectApplicationProgramPathForDebug = 18,
                EraseProgramRedirectionForDebug = 19,
            };
        protected:
            impl::LocationRedirector program_redirector;
            impl::LocationRedirector debug_program_redirector;
            impl::LocationRedirector app_control_redirector;
            impl::LocationRedirector html_docs_redirector;
            impl::LocationRedirector legal_info_redirector;
        protected:
            bool GetRedirectedPath(Path* out, impl::LocationRedirector* redirector, ncm::TitleId tid) {
                return redirector->FindRedirection(out, tid);
            }

            void ClearRedirections(u32 flags = impl::RedirectionFlags_None) {
                this->program_redirector.ClearRedirections(flags);
                this->debug_program_redirector.ClearRedirections(flags);
                this->app_control_redirector.ClearRedirections(flags);
                this->html_docs_redirector.ClearRedirections(flags);
                this->legal_info_redirector.ClearRedirections(flags);
            }

            void ClearRedirections(const ncm::TitleId* excluding_tids, size_t num_tids) {
                this->program_redirector.ClearRedirections(excluding_tids, num_tids);
                this->debug_program_redirector.ClearRedirections(excluding_tids, num_tids);
                this->app_control_redirector.ClearRedirections(excluding_tids, num_tids);
                this->html_docs_redirector.ClearRedirections(excluding_tids, num_tids);
                this->legal_info_redirector.ClearRedirections(excluding_tids, num_tids);
            }
        public:
            virtual Result ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RedirectApplicationControlPathDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid);
            virtual Result RedirectApplicationHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid);
            virtual Result ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RedirectApplicationLegalInformationPathDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid);
            virtual Result Refresh();
            virtual Result RedirectApplicationProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid);
            virtual Result ClearApplicationRedirectionDeprecated();
            virtual Result ClearApplicationRedirection(InBuffer<ncm::TitleId> excluding_tids);
            virtual Result EraseProgramRedirection(ncm::TitleId tid);
            virtual Result EraseApplicationControlRedirection(ncm::TitleId tid);
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid);
            virtual Result EraseApplicationLegalInformationRedirection(ncm::TitleId tid);
            virtual Result ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result RedirectApplicationProgramPathForDebugDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            virtual Result RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid);
            virtual Result EraseProgramRedirectionForDebug(ncm::TitleId tid);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ILocationResolver, ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectProgramPath),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, ResolveApplicationControlPath),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, ResolveApplicationHtmlDocumentPath),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, ResolveDataPath),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationControlPathDeprecated,          FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationControlPath,                    FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationHtmlDocumentPathDeprecated,     FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationHtmlDocumentPath,               FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, ResolveApplicationLegalInformationPath),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationLegalInformationPathDeprecated, FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationLegalInformationPath,           FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, Refresh),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationProgramPathDeprecated,          FirmwareVersion_500, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationProgramPath,                    FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, ClearApplicationRedirectionDeprecated,             FirmwareVersion_500, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, ClearApplicationRedirection,                       FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, EraseProgramRedirection,                           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, EraseApplicationControlRedirection,                FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, EraseApplicationHtmlDocumentRedirection,           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, EraseApplicationLegalInformationRedirection,       FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, ResolveProgramPathForDebug,                        FirmwareVersion_700),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectProgramPathForDebug,                       FirmwareVersion_700),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationProgramPathForDebugDeprecated,  FirmwareVersion_700, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, RedirectApplicationProgramPathForDebug,            FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ILocationResolver, EraseProgramRedirectionForDebug,                   FirmwareVersion_700),
            };
    };

}
