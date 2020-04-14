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

    class IRegisteredLocationResolver : public sf::IServiceObject {
        protected:
            enum class CommandId {
                ResolveProgramPath                    = 0,
                RegisterProgramPathDeprecated         = 1,
                RegisterProgramPath                   = 1,
                UnregisterProgramPath                 = 2,
                RedirectProgramPathDeprecated         = 3,
                RedirectProgramPath                   = 3,
                ResolveHtmlDocumentPath               = 4,
                RegisterHtmlDocumentPathDeprecated    = 5,
                RegisterHtmlDocumentPath              = 5,
                UnregisterHtmlDocumentPath            = 6,
                RedirectHtmlDocumentPathDeprecated    = 7,
                RedirectHtmlDocumentPath              = 7,
                Refresh                               = 8,
                RefreshExcluding                      = 9,
            };
        public:
            /* Actual commands. */
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result RegisterProgramPathDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result UnregisterProgramPath(ncm::ProgramId id) = 0;
            virtual Result RedirectProgramPathDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result ResolveHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result RegisterHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result UnregisterHtmlDocumentPath(ncm::ProgramId id) = 0;
            virtual Result RedirectHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) = 0;
            virtual Result RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) = 0;
            virtual Result Refresh() = 0;
            virtual Result RefreshExcluding(const sf::InArray<ncm::ProgramId> &ids) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(RegisterProgramPathDeprecated,      hos::Version_1_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RegisterProgramPath,                hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(UnregisterProgramPath),
                MAKE_SERVICE_COMMAND_META(RedirectProgramPathDeprecated,      hos::Version_1_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RedirectProgramPath,                hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(ResolveHtmlDocumentPath,            hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(RegisterHtmlDocumentPathDeprecated, hos::Version_2_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RegisterHtmlDocumentPath,           hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(UnregisterHtmlDocumentPath,         hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(RedirectHtmlDocumentPathDeprecated, hos::Version_2_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RedirectHtmlDocumentPath,           hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(Refresh,                            hos::Version_7_0_0),
                MAKE_SERVICE_COMMAND_META(RefreshExcluding,                   hos::Version_9_0_0),
            };
    };

}
