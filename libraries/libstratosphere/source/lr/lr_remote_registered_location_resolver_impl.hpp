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
#include <stratosphere.hpp>

namespace ams::lr {

    class RemoteRegisteredLocationResolverImpl : public IRegisteredLocationResolver {
        private:
            ::LrRegisteredLocationResolver srv;
        public:
            RemoteRegisteredLocationResolverImpl(::LrRegisteredLocationResolver &l) : srv(l) { /* ... */ }

            ~RemoteRegisteredLocationResolverImpl() { ::serviceClose(&srv.s); }
        public:
            /* Actual commands. */
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) override {
                return lrRegLrResolveProgramPath(std::addressof(this->srv), static_cast<u64>(id), out->str);
            }

            virtual Result RegisterProgramPathDeprecated(const Path &path, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result UnregisterProgramPath(ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RedirectProgramPathDeprecated(const Path &path, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result ResolveHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RegisterHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result UnregisterHtmlDocumentPath(ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RedirectHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result Refresh() override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }

            virtual Result RefreshExcluding(const sf::InArray<ncm::ProgramId> &ids) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }
    };

}
