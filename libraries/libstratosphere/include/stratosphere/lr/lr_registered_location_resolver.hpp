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
#include <stratosphere/lr/lr_location_redirector.hpp>
#include <stratosphere/lr/lr_registered_data.hpp>

namespace ams::lr {

    class RegisteredLocationResolverInterface final : public sf::IServiceObject {
        private:
            static constexpr size_t MaxRegisteredLocations = 0x20;
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
        private:
            /* Redirection and registered location storage. */
            LocationRedirector program_redirector;
            RegisteredLocations<ncm::ProgramId, MaxRegisteredLocations> registered_program_locations;
            LocationRedirector html_docs_redirector;
            RegisteredLocations<ncm::ProgramId, MaxRegisteredLocations> registered_html_docs_locations;
        private:
            /* Helper functions. */
            void ClearRedirections(u32 flags = RedirectionFlags_None);
            Result RefreshImpl(const ncm::ProgramId* excluding_ids, size_t num_ids);
        public:
            RegisteredLocationResolverInterface() : registered_program_locations(hos::GetVersion() < hos::Version_900 ? 0x10 : MaxRegisteredLocations), registered_html_docs_locations(hos::GetVersion() < hos::Version_900 ? 0x10 : MaxRegisteredLocations) { /* ... */ }
            ~RegisteredLocationResolverInterface();

            /* Actual commands. */
            Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id);
            Result RegisterProgramPathDeprecated(const Path &path, ncm::ProgramId id);
            Result RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result UnregisterProgramPath(ncm::ProgramId id);
            Result RedirectProgramPathDeprecated(const Path &path, ncm::ProgramId id);
            Result RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result ResolveHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id);
            Result RegisterHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id);
            Result RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result UnregisterHtmlDocumentPath(ncm::ProgramId id);
            Result RedirectHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id);
            Result RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result Refresh();
            Result RefreshExcluding(const sf::InArray<ncm::ProgramId> &ids);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(RegisterProgramPathDeprecated,      hos::Version_100, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RegisterProgramPath,                hos::Version_900),
                MAKE_SERVICE_COMMAND_META(UnregisterProgramPath),
                MAKE_SERVICE_COMMAND_META(RedirectProgramPathDeprecated,      hos::Version_100, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RedirectProgramPath,                hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ResolveHtmlDocumentPath,            hos::Version_200),
                MAKE_SERVICE_COMMAND_META(RegisterHtmlDocumentPathDeprecated, hos::Version_200, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RegisterHtmlDocumentPath,           hos::Version_900),
                MAKE_SERVICE_COMMAND_META(UnregisterHtmlDocumentPath,         hos::Version_200),
                MAKE_SERVICE_COMMAND_META(RedirectHtmlDocumentPathDeprecated, hos::Version_200, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RedirectHtmlDocumentPathDeprecated, hos::Version_900),
                MAKE_SERVICE_COMMAND_META(Refresh,                            hos::Version_700),
                MAKE_SERVICE_COMMAND_META(RefreshExcluding,                   hos::Version_900),
            };
    };

}
