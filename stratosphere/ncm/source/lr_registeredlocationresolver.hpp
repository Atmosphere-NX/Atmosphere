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
#include "impl/lr_registered_data.hpp"

namespace sts::lr {

    class RegisteredLocationResolverInterface final : public IServiceObject {
        private:
            static constexpr size_t MaxRegisteredLocations = 0x20;
        protected:
            enum class CommandId {
                ResolveProgramPath = 0,
                RegisterProgramPathDeprecated = 1,
                RegisterProgramPath = 1,
                UnregisterProgramPath = 2,
                RedirectProgramPathDeprecated = 3,
                RedirectProgramPath = 3,
                ResolveHtmlDocumentPath = 4,
                RegisterHtmlDocumentPathDeprecated = 5,
                RegisterHtmlDocumentPath = 5,
                UnregisterHtmlDocumentPath = 6,
                RedirectHtmlDocumentPathDeprecated = 7,
                RedirectHtmlDocumentPath = 7,
                Refresh = 8,
                RefreshExcluding = 9,
            };
        private:
            impl::LocationRedirector program_redirector;
            impl::RegisteredLocations<ncm::TitleId, MaxRegisteredLocations> registered_program_locations;
            impl::LocationRedirector html_docs_redirector;
            impl::RegisteredLocations<ncm::TitleId, MaxRegisteredLocations> registered_html_docs_locations;
        private:
            void ClearRedirections(u32 flags = impl::RedirectionFlags_None);
            void RegisterPath(const Path& path, impl::RegisteredLocations<ncm::TitleId, MaxRegisteredLocations>* locations, ncm::TitleId tid, ncm::TitleId application_id);
            bool ResolvePath(Path* out, impl::LocationRedirector* redirector, impl::RegisteredLocations<ncm::TitleId, MaxRegisteredLocations>* locations, ncm::TitleId tid);
            Result RefreshImpl(const ncm::TitleId* excluding_tids, size_t num_tids);
        public:
            RegisteredLocationResolverInterface() : registered_program_locations(GetRuntimeFirmwareVersion() < FirmwareVersion_900 ? 0x10 : MaxRegisteredLocations), registered_html_docs_locations(GetRuntimeFirmwareVersion() < FirmwareVersion_900 ? 0x10 : MaxRegisteredLocations) { /* ... */ }
            ~RegisteredLocationResolverInterface();

            Result ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            Result RegisterProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            Result RegisterProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId application_id);
            Result UnregisterProgramPath(ncm::TitleId tid);
            Result RedirectProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            Result RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId application_id);
            Result ResolveHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            Result RegisterHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            Result RegisterHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId application_id);
            Result UnregisterHtmlDocumentPath(ncm::TitleId tid);
            Result RedirectHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid);
            Result RedirectHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId application_id);
            Result Refresh();
            Result RefreshExcluding(InBuffer<ncm::TitleId> tids);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RegisterProgramPathDeprecated,      FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RegisterProgramPath,                FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, UnregisterProgramPath),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RedirectProgramPathDeprecated,      FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RedirectProgramPath,                FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, ResolveHtmlDocumentPath,            FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RegisterHtmlDocumentPathDeprecated, FirmwareVersion_200, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RegisterHtmlDocumentPath,           FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, UnregisterHtmlDocumentPath,         FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RedirectHtmlDocumentPathDeprecated, FirmwareVersion_200, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RedirectHtmlDocumentPathDeprecated, FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, Refresh,                            FirmwareVersion_700),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RefreshExcluding,                   FirmwareVersion_900),
            };
    };

}
