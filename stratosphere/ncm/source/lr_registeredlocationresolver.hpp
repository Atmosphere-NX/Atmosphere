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
#include "lr_types.hpp"

namespace sts::lr {

    class RegisteredLocationResolverInterface final : public IServiceObject {
        protected:
            enum class CommandId {
                ResolveProgramPath = 0,
                RegisterProgramPath = 1,
                UnregisterProgramPath = 2,
                RedirectProgramPath = 3,
                ResolveHtmlDocumentPath = 4,
                RegisterHtmlDocumentPath = 5,
                UnregisterHtmlDocumentPath = 6,
                RedirectHtmlDocumentPath = 7,
                Refresh = 8,
            };
        private:
            impl::LocationRedirector program_redirector;
            impl::RegisteredLocationRedirector registered_program_redirector;
            impl::LocationRedirector html_docs_redirector;
            impl::RegisteredLocationRedirector registered_html_docs_redirector;
        public:
            RegisteredLocationResolverInterface();
            ~RegisteredLocationResolverInterface();

            Result ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            Result RegisterProgramPath(ncm::TitleId tid, InPointer<const Path> path);
            Result UnregisterProgramPath(ncm::TitleId tid);
            Result RedirectProgramPath(ncm::TitleId tid, InPointer<const Path> path);
            Result ResolveHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            Result RegisterHtmlDocumentPath(ncm::TitleId tid, InPointer<const Path> path);
            Result UnregisterHtmlDocumentPath(ncm::TitleId tid);
            Result RedirectHtmlDocumentPath(ncm::TitleId tid, InPointer<const Path> path);
            Result Refresh();
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RegisterProgramPath),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, UnregisterProgramPath),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RedirectProgramPath),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, ResolveHtmlDocumentPath,    FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RegisterHtmlDocumentPath,   FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, UnregisterHtmlDocumentPath, FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, RedirectHtmlDocumentPath,   FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(RegisteredLocationResolverInterface, Refresh,                    FirmwareVersion_700),
            };
    };

}