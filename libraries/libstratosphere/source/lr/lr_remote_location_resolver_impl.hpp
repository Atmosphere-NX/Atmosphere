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

    class RemoteLocationResolverImpl {
        private:
            ::LrLocationResolver srv;
        public:
            RemoteLocationResolverImpl(::LrLocationResolver &l) : srv(l) { /* ... */ }

            ~RemoteLocationResolverImpl() { ::serviceClose(&srv.s); }
        public:
            /* Actual commands. */
            Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
                return ::lrLrResolveProgramPath(std::addressof(this->srv), id.value, out->str);
            }

            Result RedirectProgramPath(const Path &path, ncm::ProgramId id) {
                return ::lrLrRedirectProgramPath(std::addressof(this->srv), id.value, path.str);
            }

            Result ResolveApplicationControlPath(sf::Out<Path> out, ncm::ProgramId id) {
                return ::lrLrResolveApplicationControlPath(std::addressof(this->srv), id.value, out->str);
            }

            Result ResolveApplicationHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
                return ::lrLrResolveApplicationHtmlDocumentPath(std::addressof(this->srv), id.value, out->str);
            }

            Result ResolveDataPath(sf::Out<Path> out, ncm::DataId id) {
                return ::lrLrResolveDataPath(std::addressof(this->srv), id.value, out->str);
            }

            Result RedirectApplicationControlPathDeprecated(const Path &path, ncm::ProgramId id) {
                return ::lrLrRedirectApplicationControlPath(std::addressof(this->srv), id.value, 0, path.str);
            }

            Result RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                return ::lrLrRedirectApplicationControlPath(std::addressof(this->srv), id.value, owner_id.value, path.str);
            }

            Result RedirectApplicationHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
                return ::lrLrRedirectApplicationHtmlDocumentPath(std::addressof(this->srv), id.value, 0, path.str);
            }

            Result RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                return ::lrLrRedirectApplicationHtmlDocumentPath(std::addressof(this->srv), id.value, owner_id.value, path.str);
            }

            Result ResolveApplicationLegalInformationPath(sf::Out<Path> out, ncm::ProgramId id) {
                return ::lrLrResolveApplicationLegalInformationPath(std::addressof(this->srv), id.value, out->str);
            }

            Result RedirectApplicationLegalInformationPathDeprecated(const Path &path, ncm::ProgramId id) {
                return ::lrLrRedirectApplicationLegalInformationPath(std::addressof(this->srv), id.value, 0, path.str);
            }

            Result RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                return ::lrLrRedirectApplicationLegalInformationPath(std::addressof(this->srv), id.value, owner_id.value, path.str);
            }

            Result Refresh() {
                return ::lrLrRefresh(std::addressof(this->srv));
            }

            Result RedirectApplicationProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result ClearApplicationRedirectionDeprecated() {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result ClearApplicationRedirection(const sf::InArray<ncm::ProgramId> &excluding_ids) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result EraseProgramRedirection(ncm::ProgramId id) {
                return ::lrLrEraseProgramRedirection(std::addressof(this->srv), id.value);
            }

            Result EraseApplicationControlRedirection(ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result EraseApplicationLegalInformationRedirection(ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result ResolveProgramPathForDebug(sf::Out<Path> out, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result RedirectApplicationProgramPathForDebugDeprecated(const Path &path, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result EraseProgramRedirectionForDebug(ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }
    };
    static_assert(lr::IsILocationResolver<RemoteLocationResolverImpl>);

}
