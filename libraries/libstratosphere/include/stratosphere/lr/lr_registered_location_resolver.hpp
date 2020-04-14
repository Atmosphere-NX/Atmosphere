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
#include <stratosphere/lr/lr_i_registered_location_resolver.hpp>

namespace ams::lr {

    class RegisteredLocationResolver {
        NON_COPYABLE(RegisteredLocationResolver);
        private:
            std::shared_ptr<IRegisteredLocationResolver> interface;
        public:
            RegisteredLocationResolver() { /* ... */ }
            explicit RegisteredLocationResolver(std::shared_ptr<IRegisteredLocationResolver> intf) : interface(std::move(intf)) { /* ... */ }

            RegisteredLocationResolver(RegisteredLocationResolver &&rhs) {
                this->interface = std::move(rhs.interface);
            }

            RegisteredLocationResolver &operator=(RegisteredLocationResolver &&rhs) {
                RegisteredLocationResolver(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(RegisteredLocationResolver &rhs) {
                std::swap(this->interface, rhs.interface);
            }
        public:
            /* Actual commands. */
            Result ResolveProgramPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(this->interface);
                return this->interface->ResolveProgramPath(out, id);
            }

            Result RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return this->interface->RegisterProgramPath(path, id, owner_id);
                } else {
                    return this->interface->RegisterProgramPathDeprecated(path, id);
                }
            }

            Result UnregisterProgramPath(ncm::ProgramId id) {
                AMS_ASSERT(this->interface);
                return this->interface->UnregisterProgramPath(id);
            }

            void RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(this->interface->RedirectProgramPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(this->interface->RedirectProgramPathDeprecated(path, id));
                }
            }

            Result ResolveHtmlDocumentPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(this->interface);
                return this->interface->ResolveHtmlDocumentPath(out, id);
            }

            Result RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return this->interface->RegisterHtmlDocumentPath(path, id, owner_id);
                } else {
                    return this->interface->RegisterHtmlDocumentPathDeprecated(path, id);
                }
            }

            Result UnregisterHtmlDocumentPath(ncm::ProgramId id) {
                AMS_ASSERT(this->interface);
                return this->interface->UnregisterHtmlDocumentPath(id);
            }

            void RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(this->interface->RedirectHtmlDocumentPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(this->interface->RedirectHtmlDocumentPathDeprecated(path, id));
                }
            }

            Result Refresh() {
                AMS_ASSERT(this->interface);
                return this->interface->Refresh();
            }

            Result RefreshExcluding(const ncm::ProgramId *excluding_ids, size_t num_ids) {
                AMS_ASSERT(this->interface);
                return this->interface->RefreshExcluding(sf::InArray<ncm::ProgramId>(excluding_ids, num_ids));
            }

    };

}
