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
#include <switch.h>
#include <stratosphere.hpp>

#include "lr_ilocationresolver.hpp"
#include "ncm_icontentmetadatabase.hpp"
#include "ncm_icontentstorage.hpp"

namespace ams::lr {

    class ContentLocationResolverInterface : public ILocationResolver {
        private:
            ncm::StorageId storage_id;
            std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
            std::shared_ptr<ncm::IContentStorage> content_storage;
        public:
            ContentLocationResolverInterface(ncm::StorageId storage_id) : storage_id(storage_id) { /* ... */ }

            ~ContentLocationResolverInterface();
        private:
            void GetContentStoragePath(Path* out, ncm::ContentId content_id);
        public:
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId id) override;
            virtual Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result ResolveDataPath(sf::Out<Path> out, ncm::ProgramId id) override;
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
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectProgramPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveApplicationControlPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveApplicationHtmlDocumentPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveDataPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationControlPathDeprecated,          hos::Version_100, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationControlPath,                    hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationHtmlDocumentPathDeprecated,     hos::Version_100, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationHtmlDocumentPath,               hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveApplicationLegalInformationPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationLegalInformationPathDeprecated, hos::Version_100, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationLegalInformationPath,           hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, Refresh),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationProgramPathDeprecated,          hos::Version_500, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationProgramPath,                    hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ClearApplicationRedirectionDeprecated,             hos::Version_500, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ClearApplicationRedirection,                       hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseProgramRedirection,                           hos::Version_500),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseApplicationControlRedirection,                hos::Version_500),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseApplicationHtmlDocumentRedirection,           hos::Version_500),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseApplicationLegalInformationRedirection,       hos::Version_500),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveProgramPathForDebug,                        hos::Version_700),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectProgramPathForDebug,                       hos::Version_700),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationProgramPathForDebugDeprecated,  hos::Version_700, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationProgramPathForDebug,            hos::Version_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseProgramRedirectionForDebug,                   hos::Version_700),
            };
    };

}
