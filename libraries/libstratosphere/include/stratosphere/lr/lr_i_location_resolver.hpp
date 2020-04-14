/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/lr/lr_types.hpp>

namespace ams::lr {

    class ILocationResolver : public sf::IServiceObject {
        NON_COPYABLE(ILocationResolver);
        NON_MOVEABLE(ILocationResolver);
        protected:
            enum class CommandId {
                ResolveProgramPath                                   = 0,
                RedirectProgramPath                                  = 1,
                ResolveApplicationControlPath                        = 2,
                ResolveApplicationHtmlDocumentPath                   = 3,
                ResolveDataPath                                      = 4,
                RedirectApplicationControlPathDeprecated             = 5,
                RedirectApplicationControlPath                       = 5,
                RedirectApplicationHtmlDocumentPathDeprecated        = 6,
                RedirectApplicationHtmlDocumentPath                  = 6,
                ResolveApplicationLegalInformationPath               = 7,
                RedirectApplicationLegalInformationPathDeprecated    = 8,
                RedirectApplicationLegalInformationPath              = 8,
                Refresh                                              = 9,
                RedirectApplicationProgramPathDeprecated             = 10,
                RedirectApplicationProgramPath                       = 10,
                ClearApplicationRedirectionDeprecated                = 11,
                ClearApplicationRedirection                          = 11,
                EraseProgramRedirection                              = 12,
                EraseApplicationControlRedirection                   = 13,
                EraseApplicationHtmlDocumentRedirection              = 14,
                EraseApplicationLegalInformationRedirection          = 15,
                ResolveProgramPathForDebug                           = 16,
                RedirectProgramPathForDebug                          = 17,
                RedirectApplicationProgramPathForDebugDeprecated     = 18,
                RedirectApplicationProgramPathForDebug               = 18,
                EraseProgramRedirectionForDebug                      = 19,
            };
        public:
            ILocationResolver() { /* ... */ }
        public:
            /* Actual commands. */
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId id) = 0;
            virtual Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result ResolveDataPath(sf::Out<Path> out, ncm::DataId id) = 0;
            virtual Result RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result Refresh() = 0;
            virtual Result RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result ClearApplicationRedirectionDeprecated() = 0;
            virtual Result ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) = 0;
            virtual Result EraseProgramRedirection(ncm::ProgramId id) = 0;
            virtual Result EraseApplicationControlRedirection(ncm::ProgramId id) = 0;
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) = 0;
            virtual Result EraseApplicationLegalInformationRedirection(ncm::ProgramId id) = 0;
            virtual Result ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result EraseProgramRedirectionForDebug(ncm::ProgramId id) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(RedirectProgramPath),
                MAKE_SERVICE_COMMAND_META(ResolveApplicationControlPath),
                MAKE_SERVICE_COMMAND_META(ResolveApplicationHtmlDocumentPath),
                MAKE_SERVICE_COMMAND_META(ResolveDataPath),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationControlPathDeprecated,          hos::Version_1_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationControlPath,                    hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationHtmlDocumentPathDeprecated,     hos::Version_1_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationHtmlDocumentPath,               hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(ResolveApplicationLegalInformationPath),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationLegalInformationPathDeprecated, hos::Version_1_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationLegalInformationPath,           hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(Refresh),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationProgramPathDeprecated,          hos::Version_5_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationProgramPath,                    hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(ClearApplicationRedirectionDeprecated,             hos::Version_5_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(ClearApplicationRedirection,                       hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(EraseProgramRedirection,                           hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(EraseApplicationControlRedirection,                hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(EraseApplicationHtmlDocumentRedirection,           hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(EraseApplicationLegalInformationRedirection,       hos::Version_5_0_0),
                MAKE_SERVICE_COMMAND_META(ResolveProgramPathForDebug,                        hos::Version_7_0_0),
                MAKE_SERVICE_COMMAND_META(RedirectProgramPathForDebug,                       hos::Version_7_0_0),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationProgramPathForDebugDeprecated,  hos::Version_7_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RedirectApplicationProgramPathForDebug,            hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(EraseProgramRedirectionForDebug,                   hos::Version_7_0_0),
            };
    };

}
