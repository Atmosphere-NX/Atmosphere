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

    class ContentLocationResolverImpl : public LocationResolverImplBase {
        private:
            ncm::StorageId storage_id;

            /* Objects for this storage type. */
            ncm::ContentMetaDatabase content_meta_database;
            ncm::ContentStorage content_storage;
        public:
            ContentLocationResolverImpl(ncm::StorageId storage_id) : storage_id(storage_id) { /* ... */ }

            ~ContentLocationResolverImpl();
        private:
            /* Helper functions. */
            void GetContentStoragePath(Path *out, ncm::ContentId content_id);
        public:
            /* Actual commands. */
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId id) override;
            virtual Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result ResolveDataPath(sf::Out<Path> out, ncm::DataId id) override;
            virtual Result RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result Refresh() override;
            virtual Result RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result ClearApplicationRedirectionDeprecated() override;
            virtual Result ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) override;
            virtual Result EraseProgramRedirection(ncm::ProgramId id) override;
            virtual Result EraseApplicationControlRedirection(ncm::ProgramId id) override;
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) override;
            virtual Result EraseApplicationLegalInformationRedirection(ncm::ProgramId id) override;
            virtual Result ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) override;
            virtual Result RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result EraseProgramRedirectionForDebug(ncm::ProgramId id) override;
    };

}
