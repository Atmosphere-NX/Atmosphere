/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>

namespace ams::lr {

    class RemoteLocationResolverImpl : public ILocationResolver {
        private:
            ::LrLocationResolver srv;
        public:
            RemoteLocationResolverImpl(::LrLocationResolver &l) : srv(l) { /* ... */ }

            ~RemoteLocationResolverImpl() { ::serviceClose(&srv.s); }
        public:
            /* Actual commands. */
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) override {
                return lrLrResolveProgramPath(std::addressof(this->srv), id.value, out->str);
            }

            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId id) override {
                return lrLrRedirectProgramPath(std::addressof(this->srv), id.value, path.str);
            }

            virtual Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) override {
                return lrLrResolveApplicationControlPath(std::addressof(this->srv), id.value, out->str);
            }

            virtual Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) override {
                return lrLrResolveApplicationHtmlDocumentPath(std::addressof(this->srv), id.value, out->str);
            }

            virtual Result ResolveDataPath(sf::Out<Path> out, ncm::DataId id) override {
                return lrLrResolveDataPath(std::addressof(this->srv), id.value, out->str);
            }

            virtual Result RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) override {
                return lrLrRedirectApplicationControlPath(std::addressof(this->srv), id.value, 0, path.str);
            }

            virtual Result RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                return lrLrRedirectApplicationControlPath(std::addressof(this->srv), id.value, owner_id.value, path.str);
            }

            virtual Result RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) override {
                return lrLrRedirectApplicationHtmlDocumentPath(std::addressof(this->srv), id.value, 0, path.str);
            }

            virtual Result RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                return lrLrRedirectApplicationHtmlDocumentPath(std::addressof(this->srv), id.value, owner_id.value, path.str);
            }

            virtual Result ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) override {
                return lrLrResolveApplicationLegalInformationPath(std::addressof(this->srv), id.value, out->str);
            }

            virtual Result RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) override {
                return lrLrRedirectApplicationLegalInformationPath(std::addressof(this->srv), id.value, 0, path.str);
            }

            virtual Result RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                return lrLrRedirectApplicationLegalInformationPath(std::addressof(this->srv), id.value, owner_id.value, path.str);
            }

            virtual Result Refresh() override {
                return lrLrRefresh(std::addressof(this->srv));
            }

            virtual Result RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result ClearApplicationRedirectionDeprecated() override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result EraseProgramRedirection(ncm::ProgramId id) override {
                return lrLrEraseProgramRedirection(std::addressof(this->srv), id.value);
            }

            virtual Result EraseApplicationControlRedirection(ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result EraseApplicationLegalInformationRedirection(ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result EraseProgramRedirectionForDebug(ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }
    };

}
