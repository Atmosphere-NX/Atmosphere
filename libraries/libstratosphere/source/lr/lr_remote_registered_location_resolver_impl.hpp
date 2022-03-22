/*
 * Copyright (c) Atmosphère-NX
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

    class RemoteRegisteredLocationResolverImpl {
        private:
            ::LrRegisteredLocationResolver m_srv;
        public:
            RemoteRegisteredLocationResolverImpl(::LrRegisteredLocationResolver &l) : m_srv(l) { /* ... */ }

            ~RemoteRegisteredLocationResolverImpl() { ::serviceClose(std::addressof(m_srv.s)); }
        public:
            /* Actual commands. */
            Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) {
                return ::lrRegLrResolveProgramPath(std::addressof(m_srv), id.value, out->str);
            }

            Result RegisterProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(path, id);
                AMS_ABORT();
            }

            Result RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(path, id, owner_id);
                AMS_ABORT();
            }

            Result UnregisterProgramPath(ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(id);
                AMS_ABORT();
            }

            Result RedirectProgramPathDeprecated(const Path &path, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(path, id);
                AMS_ABORT();
            }

            Result RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(path, id, owner_id);
                AMS_ABORT();
            }

            Result ResolveHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(out, id);
                AMS_ABORT();
            }

            Result RegisterHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(path, id);
                AMS_ABORT();
            }

            Result RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(path, id, owner_id);
                AMS_ABORT();
            }

            Result UnregisterHtmlDocumentPath(ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(id);
                AMS_ABORT();
            }

            Result RedirectHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(path, id);
                AMS_ABORT();
            }

            Result RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                /* TODO: libnx bindings */
                AMS_UNUSED(path, id, owner_id);
                AMS_ABORT();
            }

            Result Refresh() {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            Result RefreshExcluding(const sf::InArray<ncm::ProgramId> &ids) {
                /* TODO: libnx bindings */
                AMS_UNUSED(ids);
                AMS_ABORT();
            }
    };
    static_assert(lr::IsIRegisteredLocationResolver<RemoteRegisteredLocationResolverImpl>);

}
