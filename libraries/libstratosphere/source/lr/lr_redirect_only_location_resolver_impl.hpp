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
#include "lr_location_resolver_impl_base.hpp"

namespace ams::lr {

    class RedirectOnlyLocationResolverImpl : public LocationResolverImplBase {
        public:
            ~RedirectOnlyLocationResolverImpl();
        public:
            /* Actual commands. */
            Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id);
            Result RedirectProgramPath(const Path &path, ncm::ProgramId id);
            Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id);
            Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id);
            Result ResolveDataPath(sf::Out<Path> out, ncm::DataId id);
            Result RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id);
            Result RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id);
            Result RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id);
            Result RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id);
            Result RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result Refresh();
            Result RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id);
            Result RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result ClearApplicationRedirectionDeprecated();
            Result ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids);
            Result EraseProgramRedirection(ncm::ProgramId id);
            Result EraseApplicationControlRedirection(ncm::ProgramId id);
            Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id);
            Result EraseApplicationLegalInformationRedirection(ncm::ProgramId id);
            Result ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id);
            Result RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id);
            Result RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id);
            Result RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id);
            Result EraseProgramRedirectionForDebug(ncm::ProgramId id);
    };
    static_assert(lr::IsILocationResolver<RedirectOnlyLocationResolverImpl>);

}
