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

namespace ams::lr {

    class ILocationResolver : public sf::IServiceObject {
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
            bool GetRedirectedPath(Path* out, impl::LocationRedirector* redirector, ncm::ProgramId tid) {
                return redirector->FindRedirection(out, tid);
            }

            void ClearRedirections(u32 flags = impl::RedirectionFlags_None) {
                this->program_redirector.ClearRedirections(flags);
                this->debug_program_redirector.ClearRedirections(flags);
                this->app_control_redirector.ClearRedirections(flags);
                this->html_docs_redirector.ClearRedirections(flags);
                this->legal_info_redirector.ClearRedirections(flags);
            }

            void ClearRedirections(const ncm::ProgramId* excluding_tids, size_t num_tids) {
                this->program_redirector.ClearRedirections(excluding_tids, num_tids);
                this->debug_program_redirector.ClearRedirections(excluding_tids, num_tids);
                this->app_control_redirector.ClearRedirections(excluding_tids, num_tids);
                this->html_docs_redirector.ClearRedirections(excluding_tids, num_tids);
                this->legal_info_redirector.ClearRedirections(excluding_tids, num_tids);
            }
        public:
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId tid);
            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId tid);
            virtual Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId tid);
            virtual Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId tid);
            virtual Result ResolveDataPath(sf::Out<Path> out, ncm::ProgramId tid);
            virtual Result RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId tid);
            virtual Result RedirectApplicationControlPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid);
            virtual Result RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId tid);
            virtual Result RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid);
            virtual Result ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId tid);
            virtual Result RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId tid);
            virtual Result RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid);
            virtual Result Refresh();
            virtual Result RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId tid);
            virtual Result RedirectApplicationProgramPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid);
            virtual Result ClearApplicationRedirectionDeprecated();
            virtual Result ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_tids);
            virtual Result EraseProgramRedirection(ncm::ProgramId tid);
            virtual Result EraseApplicationControlRedirection(ncm::ProgramId tid);
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId tid);
            virtual Result EraseApplicationLegalInformationRedirection(ncm::ProgramId tid);
            virtual Result ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId tid);
            virtual Result RedirectProgramPathForDebug(const Path &path, ncm::ProgramId tid);
            virtual Result RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId tid);
            virtual Result RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid);
            virtual Result EraseProgramRedirectionForDebug(ncm::ProgramId tid);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(RedirectProgramPath),
                MAKE_SERVICE_COMMAND_META(ResolveApplicationControlPath),
                MAKE_SERVICE_COMMAND_META(ResolveApplicationHtmlDocumentPath),
                MAKE_SERVICE_COMMAND_META(ResolveDataPath),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationControlPathDeprecated,          hos::Version_100, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationControlPath,                    hos::Version_900),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationHtmlDocumentPathDeprecated,     hos::Version_100, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationHtmlDocumentPath,               hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ResolveApplicationLegalInformationPath),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationLegalInformationPathDeprecated, hos::Version_100, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationLegalInformationPath,           hos::Version_900),
                MAKE_SERVICE_COMMAND_META(Refresh),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationProgramPathDeprecated,          hos::Version_500, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationProgramPath,                    hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ClearApplicationRedirectionDeprecated,             hos::Version_500, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(ClearApplicationRedirection,                       hos::Version_900),
                MAKE_SERVICE_COMMAND_META(EraseProgramRedirection,                           hos::Version_500),
                MAKE_SERVICE_COMMAND_META(EraseApplicationControlRedirection,                hos::Version_500),
                MAKE_SERVICE_COMMAND_META(EraseApplicationHtmlDocumentRedirection,           hos::Version_500),
                MAKE_SERVICE_COMMAND_META(EraseApplicationLegalInformationRedirection,       hos::Version_500),
                MAKE_SERVICE_COMMAND_META(ResolveProgramPathForDebug,                        hos::Version_700),
                MAKE_SERVICE_COMMAND_META(RedirectProgramPathForDebug,                       hos::Version_700),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationProgramPathForDebugDeprecated,  hos::Version_700, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationProgramPathForDebug,            hos::Version_900),
                MAKE_SERVICE_COMMAND_META(EraseProgramRedirectionForDebug,                   hos::Version_700),
            };
    };

}
