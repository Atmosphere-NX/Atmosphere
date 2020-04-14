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
#include "lr_location_redirector.hpp"
#include "lr_registered_data.hpp"

namespace ams::lr {

    class RegisteredLocationResolverImpl : public IRegisteredLocationResolver {
        private:
            static constexpr size_t MaxRegisteredLocationsDeprecated = 0x10;
            static constexpr size_t MaxRegisteredLocations           = 0x20;
            static_assert(MaxRegisteredLocations >= MaxRegisteredLocationsDeprecated);
        private:
            static ALWAYS_INLINE size_t GetMaxRegisteredLocations() {
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return MaxRegisteredLocations;
                } else {
                    return MaxRegisteredLocationsDeprecated;
                }
            }
        private:
            /* Redirection and registered location storage. */
            LocationRedirector program_redirector;
            RegisteredLocations<ncm::ProgramId, MaxRegisteredLocations> registered_program_locations;
            LocationRedirector html_docs_redirector;
            RegisteredLocations<ncm::ProgramId, MaxRegisteredLocations> registered_html_docs_locations;
        private:
            /* Helper functions. */
            void ClearRedirections(u32 flags = RedirectionFlags_None);
            Result RefreshImpl(const ncm::ProgramId *excluding_ids, size_t num_ids);
        public:
            RegisteredLocationResolverImpl() : registered_program_locations(GetMaxRegisteredLocations()), registered_html_docs_locations(GetMaxRegisteredLocations()) { /* ... */ }
            ~RegisteredLocationResolverImpl();
        public:
            /* Actual commands. */
            virtual Result ResolveProgramPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result RegisterProgramPathDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result UnregisterProgramPath(ncm::ProgramId id) override;
            virtual Result RedirectProgramPathDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result ResolveHtmlDocumentPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result RegisterHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result UnregisterHtmlDocumentPath(ncm::ProgramId id) override;
            virtual Result RedirectHtmlDocumentPathDeprecated(const Path &path, ncm::ProgramId id) override;
            virtual Result RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) override;
            virtual Result Refresh() override;
            virtual Result RefreshExcluding(const sf::InArray<ncm::ProgramId> &ids) override;
    };

}
