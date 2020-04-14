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
#include <stratosphere/lr/lr_i_location_resolver.hpp>

namespace ams::lr {

    class LocationResolver {
        NON_COPYABLE(LocationResolver);
        private:
            std::shared_ptr<ILocationResolver> interface;
        public:
            LocationResolver() { /* ... */ }
            explicit LocationResolver(std::shared_ptr<ILocationResolver> intf) : interface(std::move(intf)) { /* ... */ }

            LocationResolver(LocationResolver &&rhs) {
                this->interface = std::move(rhs.interface);
            }

            LocationResolver &operator=(LocationResolver &&rhs) {
                LocationResolver(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(LocationResolver &rhs) {
                std::swap(this->interface, rhs.interface);
            }
        public:
            Result ResolveProgramPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->ResolveProgramPath(out, id);
            }

            void RedirectProgramPath(const Path &path, ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                R_ABORT_UNLESS(this->interface->RedirectProgramPath(path, id));
            }

            Result ResolveApplicationControlPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->ResolveApplicationControlPath(out, id);
            }

            Result ResolveApplicationHtmlDocumentPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->ResolveApplicationHtmlDocumentPath(out, id);
            }

            Result ResolveDataPath(Path *out, ncm::DataId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->ResolveDataPath(out, id);
            }

            void RedirectApplicationControlPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationControlPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationControlPathDeprecated(path, id));
                }
            }

            void RedirectApplicationHtmlDocumentPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationHtmlDocumentPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationHtmlDocumentPathDeprecated(path, id));
                }
            }

            Result ResolveApplicationLegalInformationPath(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->ResolveApplicationLegalInformationPath(out, id);
            }

            void RedirectApplicationLegalInformationPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationLegalInformationPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationLegalInformationPathDeprecated(path, id));
                }
            }

            Result Refresh() {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->Refresh();
            }

            void RedirectApplicationProgramPath(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationProgramPath(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationProgramPathDeprecated(path, id));
                }
            }

            Result ClearApplicationRedirection() {
                AMS_ASSERT(this->interface != nullptr);
                AMS_ASSERT(hos::GetVersion() < hos::Version_9_0_0);
                return this->ClearApplicationRedirection(nullptr, 0);
            }

            Result ClearApplicationRedirection(const ncm::ProgramId *excluding_ids, size_t num_ids) {
                AMS_ASSERT(this->interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return this->interface->ClearApplicationRedirection(sf::InArray<ncm::ProgramId>(excluding_ids, num_ids));
                } else {
                    return this->interface->ClearApplicationRedirectionDeprecated();
                }
            }

            Result EraseProgramRedirection(ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->EraseProgramRedirection(id);
            }

            Result EraseApplicationControlRedirection(ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->EraseApplicationControlRedirection(id);
            }

            Result EraseApplicationHtmlDocumentRedirection(ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->EraseApplicationHtmlDocumentRedirection(id);
            }

            Result EraseApplicationLegalInformationRedirection(ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->EraseApplicationLegalInformationRedirection(id);
            }

            Result ResolveProgramPathForDebug(Path *out, ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->ResolveProgramPathForDebug(out, id);
            }

            void RedirectProgramPathForDebug(const Path &path, ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                R_ABORT_UNLESS(this->interface->RedirectProgramPathForDebug(path, id));
            }

            void RedirectApplicationProgramPathForDebug(const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id) {
                AMS_ASSERT(this->interface != nullptr);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationProgramPathForDebug(path, id, owner_id));
                } else {
                    R_ABORT_UNLESS(this->interface->RedirectApplicationProgramPathForDebugDeprecated(path, id));
                }
            }

            Result EraseProgramRedirectionForDebug(ncm::ProgramId id) {
                AMS_ASSERT(this->interface != nullptr);
                return this->interface->EraseProgramRedirectionForDebug(id);
            }
    };

}
