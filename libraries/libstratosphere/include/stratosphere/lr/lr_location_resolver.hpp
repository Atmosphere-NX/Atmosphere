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
#include <stratosphere/lr/lr_i_location_resolver.hpp>

namespace ams::lr {

    class LocationResolver {
        NON_COPYABLE(LocationResolver);
        private:
            sf::SharedPointer<ILocationResolver> m_interface;
        public:
            LocationResolver() { /* ... */ }
            explicit LocationResolver(sf::SharedPointer<ILocationResolver> intf) : m_interface(intf) { /* ... */ }

            LocationResolver(LocationResolver &&rhs) {
                m_interface = std::move(rhs.m_interface);
            }

            LocationResolver &operator=(LocationResolver &&rhs) {
                LocationResolver(std::move(rhs)).swap(*this);
                return *this;
            }

            void swap(LocationResolver &rhs) {
                std::swap(m_interface, rhs.m_interface);
            }
        public:
            Result ResolveProgramPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ResolveProgramPath(out, id);
            }

            void RedirectProgramPath(const Path &path, ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                R_ABORT_UNLESS(m_interface->RedirectProgramPath(path, id));
            }

            Result ResolveApplicationControlPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ResolveApplicationControlPath(out, id);
            }

            Result ResolveApplicationHtmlDocumentPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ResolveApplicationHtmlDocumentPath(out, id);
            }

            Result ResolveDataPath(Path *out, ncm::DataId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ResolveDataPath(out, id);
            }

            void RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationControlPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationControlPathDeprecated(path, id));
                }
            }

            void RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationHtmlDocumentPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationHtmlDocumentPathDeprecated(path, id));
                }
            }

            Result ResolveApplicationLegalInformationPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ResolveApplicationLegalInformationPath(out, id);
            }

            void RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationLegalInformationPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationLegalInformationPathDeprecated(path, id));
                }
            }

            Result Refresh() {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->Refresh();
            }

            void RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationProgramPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationProgramPathDeprecated(path, id));
                }
            }

            Result ClearApplicationRedirection() {
                AMS_ASSERT(m_interface != nullptr);
                AMS_ASSERT(hos::GetVersion() < hos::Version_9_0_0);
                return this->ClearApplicationRedirection(nullptr, 0);
            }

            Result ClearApplicationRedirection(const ncm::ProgramId *excluding_ids, size_t num_ids) {
                AMS_ASSERT(m_interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return m_interface->ClearApplicationRedirection(sf::InArray<ncm::ProgramId>(excluding_ids, num_ids));
                } else {
                    return m_interface->ClearApplicationRedirectionDeprecated();
                }
            }

            Result EraseProgramRedirection(ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->EraseProgramRedirection(id);
            }

            Result EraseApplicationControlRedirection(ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->EraseApplicationControlRedirection(id);
            }

            Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->EraseApplicationHtmlDocumentRedirection(id);
            }

            Result EraseApplicationLegalInformationRedirection(ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->EraseApplicationLegalInformationRedirection(id);
            }

            Result ResolveProgramPathForDebug(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->ResolveProgramPathForDebug(out, id);
            }

            void RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                R_ABORT_UNLESS(m_interface->RedirectProgramPathForDebug(path, id));
            }

            void RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(m_interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationProgramPathForDebug(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(m_interface->RedirectApplicationProgramPathForDebugDeprecated(path, id));
                }
            }

            Result EraseProgramRedirectionForDebug(ncm::ProgramId id) {
                AMS_ASSERT(m_interface != nullptr);
                return m_interface->EraseProgramRedirectionForDebug(id);
            }
    };

}
