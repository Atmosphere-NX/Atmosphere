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

#include "impl/lr_manager.hpp"
#include "lr_ilocationresolver.hpp"
#include "ncm_icontentmetadatabase.hpp"
#include "ncm_icontentstorage.hpp"

namespace sts::lr {

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
            virtual Result ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result RedirectApplicationControlPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) override;
            virtual Result RedirectApplicationHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) override;
            virtual Result ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result RedirectApplicationLegalInformationPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) override;
            virtual Result Refresh() override;
            virtual Result RedirectApplicationProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) override;
            virtual Result ClearApplicationRedirectionDeprecated() override;
            virtual Result ClearApplicationRedirection(InBuffer<ncm::TitleId> excluding_tids) override;
            virtual Result EraseProgramRedirection(ncm::TitleId tid) override;
            virtual Result EraseApplicationControlRedirection(ncm::TitleId tid) override;
            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid) override;
            virtual Result EraseApplicationLegalInformationRedirection(ncm::TitleId tid) override;
            virtual Result ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) override;
            virtual Result RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationProgramPathForDebugDeprecated(InPointer<const Path> path, ncm::TitleId tid) override;
            virtual Result RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId owner_tid) override;
            virtual Result EraseProgramRedirectionForDebug(ncm::TitleId tid) override;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveProgramPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectProgramPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveApplicationControlPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveApplicationHtmlDocumentPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveDataPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationControlPathDeprecated,          FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationControlPath,                    FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationHtmlDocumentPathDeprecated,     FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationHtmlDocumentPath,               FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveApplicationLegalInformationPath),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationLegalInformationPathDeprecated, FirmwareVersion_100, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationLegalInformationPath,           FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, Refresh),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationProgramPathDeprecated,          FirmwareVersion_500, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationProgramPath,                    FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ClearApplicationRedirectionDeprecated,             FirmwareVersion_500, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ClearApplicationRedirection,                       FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseProgramRedirection,                           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseApplicationControlRedirection,                FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseApplicationHtmlDocumentRedirection,           FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseApplicationLegalInformationRedirection,       FirmwareVersion_500),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, ResolveProgramPathForDebug,                        FirmwareVersion_700),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectProgramPathForDebug,                       FirmwareVersion_700),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationProgramPathForDebugDeprecated,  FirmwareVersion_700, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, RedirectApplicationProgramPathForDebug,            FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(ContentLocationResolverInterface, EraseProgramRedirectionForDebug,                   FirmwareVersion_700),
            };
    };

}
