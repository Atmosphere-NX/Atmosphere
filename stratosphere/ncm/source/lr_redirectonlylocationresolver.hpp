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

namespace sts::lr {

    class RedirectOnlyLocationResolverInterface : public ILocationResolver {
        public:
            ~RedirectOnlyLocationResolverInterface();
        public:
            virtual Result ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result RedirectApplicationControlPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) override;
            virtual Result RedirectApplicationHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) override;
            virtual Result ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result RedirectApplicationLegalInformationPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) override;
            virtual Result Refresh() override;
            virtual Result RedirectApplicationProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) override;
            virtual Result ClearApplicationRedirectionDeprecated() override;
            virtual Result ClearApplicationRedirection(InBuffer<ncm::TitleId> excluding_tids) override;
            virtual Result EraseProgramRedirection(ncm::TitleId tid) override;
            virtual Result EraseApplicationControlRedirection(ncm::TitleId tid) override;
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid) override;
            virtual Result EraseApplicationLegalInformationRedirection(ncm::TitleId tid) override;
            virtual Result ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationProgramPathForDebugDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) override;
            virtual Result EraseProgramRedirectionForDebug(ncm::TitleId tid) override;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectProgramPath),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, ResolveApplicationControlPath),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, ResolveApplicationHtmlDocumentPath),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, ResolveDataPath),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationControlPathDeprecated,          FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationControlPath,                    FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationHtmlDocumentPathDeprecated,     FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationHtmlDocumentPath,               FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, ResolveApplicationLegalInformationPath),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationLegalInformationPathDeprecated, FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationLegalInformationPath,           FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, Refresh),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationProgramPathDeprecated,          FirmwareVersion_500, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationProgramPath,                    FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, ClearApplicationRedirectionDeprecated,             FirmwareVersion_500, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, ClearApplicationRedirection,                       FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, EraseProgramRedirection,                           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, EraseApplicationControlRedirection,                FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, EraseApplicationHtmlDocumentRedirection,           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, EraseApplicationLegalInformationRedirection,       FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, ResolveProgramPathForDebug,                        FirmwareVersion_700),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectProgramPathForDebug,                       FirmwareVersion_700),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationProgramPathForDebugDeprecated,  FirmwareVersion_700, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, RedirectApplicationProgramPathForDebug,            FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RedirectOnlyLocationResolverInterface, EraseProgramRedirectionForDebug,                   FirmwareVersion_700),
            };
    };

}
