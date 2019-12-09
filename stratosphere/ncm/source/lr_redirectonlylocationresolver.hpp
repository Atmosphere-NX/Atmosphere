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

#include "lr_contentlocationresolver.hpp"

namespace ams::lr {

    class RedirectOnlyLocationResolverInterface : public ILocationResolver {
        public:
            ~RedirectOnlyLocationResolverInterface();
        public:
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId tid) override;
            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId tid) override;
            virtual Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId tid) override;
            virtual Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId tid) override;
            virtual Result ResolveDataPath(sf::Out<Path> out, ncm::ProgramId tid) override;
            virtual Result RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId tid) override;
            virtual Result RedirectApplicationControlPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) override;
            virtual Result RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId tid) override;
            virtual Result RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) override;
            virtual Result ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId tid) override;
            virtual Result RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId tid) override;
            virtual Result RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) override;
            virtual Result Refresh() override;
            virtual Result RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId tid) override;
            virtual Result RedirectApplicationProgramPath(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) override;
            virtual Result ClearApplicationRedirectionDeprecated() override;
            virtual Result ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_tids) override;
            virtual Result EraseProgramRedirection(ncm::ProgramId tid) override;
            virtual Result EraseApplicationControlRedirection(ncm::ProgramId tid) override;
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId tid) override;
            virtual Result EraseApplicationLegalInformationRedirection(ncm::ProgramId tid) override;
            virtual Result ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId tid) override;
            virtual Result RedirectProgramPathForDebug(const Path &path, ncm::ProgramId tid) override;
            virtual Result RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId tid) override;
            virtual Result RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId tid, ncm::ProgramId owner_tid) override;
            virtual Result EraseProgramRedirectionForDebug(ncm::ProgramId tid) override;
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
