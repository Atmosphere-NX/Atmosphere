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
#include <stratosphere/lr/lr_types.hpp>
#include <stratosphere/lr/lr_i_registered_location_resolver.hpp>

namespace ams::lr {

    class RegisteredLocationResolver {
        NON_COPYABLE(RegisteredLocationResolver);
        private:
            sf::SharedPointer<IRegisteredLocationResolver> m_interface;
        public:
            RegisteredLocationResolver() { /* ... */ }
            explicit RegisteredLocationResolver(sf::SharedPointer<IRegisteredLocationResolver> intf) : m_interface(intf) { /* ... */ }

            RegisteredLocationResolver(RegisteredLocationResolver &&rhs) {
                m_interface = std::move(rhs.m_interface);
            }

            RegisteredLocationResolver &operator=(RegisteredLocationResolver &&rhs) {
                RegisteredLocationResolver(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(RegisteredLocationResolver &rhs) {
                std::swap(m_interface, rhs.m_interface);
            }
        public:
            /* Actual commands. */
            Result ResolveProgramPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(m_interface);
                return m_interface->ResolveProgramPath(out, id);
            }

            Result RegisterProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return m_interface->RegisterProgramPath(path, id, owner_id);
                } else {
                    return m_interface->RegisterProgramPathDeprecated(path, id);
                }
            }

            Result UnregisterProgramPath(ncm::ProgramId id) {
                AMS_ASSERT(m_interface);
                return m_interface->UnregisterProgramPath(id);
            }

            void RedirectProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(m_interface->RedirectProgramPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(m_interface->RedirectProgramPathDeprecated(path, id));
                }
            }

            Result ResolveHtmlDocumentPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(m_interface);
                return m_interface->ResolveHtmlDocumentPath(out, id);
            }

            Result RegisterHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return m_interface->RegisterHtmlDocumentPath(path, id, owner_id);
                } else {
                    return m_interface->RegisterHtmlDocumentPathDeprecated(path, id);
                }
            }

            Result UnregisterHtmlDocumentPath(ncm::ProgramId id) {
                AMS_ASSERT(m_interface);
                return m_interface->UnregisterHtmlDocumentPath(id);
            }

            void RedirectHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(m_interface->RedirectHtmlDocumentPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(m_interface->RedirectHtmlDocumentPathDeprecated(path, id));
                }
            }

            Result Refresh() {
                AMS_ASSERT(m_interface);
                return m_interface->Refresh();
            }

            Result RefreshExcluding(const ncm::ProgramId *excluding_ids, size_t num_ids) {
                AMS_ASSERT(m_interface);
                return m_interface->RefreshExcluding(sf::InArray<ncm::ProgramId>(excluding_ids, num_ids));
            }

    };

}
