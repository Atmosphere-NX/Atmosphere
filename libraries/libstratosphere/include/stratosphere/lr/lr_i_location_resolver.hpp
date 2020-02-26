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

namespace ams::lr {

    class ILocationResolver : public sf::IServiceObject {
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
        protected:
            /* Location redirectors. */
            LocationRedirector program_redirector;
            LocationRedirector debug_program_redirector;
            LocationRedirector app_control_redirector;
            LocationRedirector html_docs_redirector;
            LocationRedirector legal_info_redirector;
        protected:
            /* Helper functions. */
            void ClearRedirections(u32 flags = RedirectionFlags_None) {
                this->program_redirector.ClearRedirections(flags);
                this->debug_program_redirector.ClearRedirections(flags);
                this->app_control_redirector.ClearRedirections(flags);
                this->html_docs_redirector.ClearRedirections(flags);
                this->legal_info_redirector.ClearRedirections(flags);
            }

            void ClearRedirections(const ncm::ProgramId* excluding_ids, size_t num_ids) {
                this->program_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                this->debug_program_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                this->app_control_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                this->html_docs_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
                this->legal_info_redirector.ClearRedirectionsExcludingOwners(excluding_ids, num_ids);
            }
        public:
            /* Actual commands. */
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId id) = 0;
            virtual Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
            virtual Result ResolveDataPath(sf::Out<Path> out, ncm::ProgramId id) = 0;
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
    };

}
