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
#include <stratosphere/lr/lr_i_add_on_content_location_resolver.hpp>

namespace ams::lr {

    class AddOnContentLocationResolver {
        NON_COPYABLE(AddOnContentLocationResolver);
        private:
            std::shared_ptr<IAddOnContentLocationResolver> interface;
        public:
            AddOnContentLocationResolver() { /* ... */ }
            explicit AddOnContentLocationResolver(std::shared_ptr<IAddOnContentLocationResolver> intf) : interface(std::move(intf)) { /* ... */ }

            AddOnContentLocationResolver(AddOnContentLocationResolver &&rhs) {
                this->interface = std::move(rhs.interface);
            }

            AddOnContentLocationResolver &operator=(AddOnContentLocationResolver &&rhs) {
                AddOnContentLocationResolver(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(AddOnContentLocationResolver &rhs) {
                std::swap(this->interface, rhs.interface);
            }
        public:
            /* Actual commands. */
            Result ResolveAddOnContentPath(Path *out, ncm::DataId id) {
                AMS_ASSERT(this->interface);
                return this->interface->ResolveAddOnContentPath(out, id);
            }

            Result RegisterAddOnContentStorage(ncm::DataId id, ncm::ApplicationId application_id, ncm::StorageId storage_id) {
                AMS_ASSERT(this->interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return this->interface->RegisterAddOnContentStorage(id, application_id, storage_id);
                } else {
                    return this->interface->RegisterAddOnContentStorageDeprecated(id, storage_id);
                }
            }

            Result UnregisterAllAddOnContentPath() {
                AMS_ASSERT(this->interface);
                return this->interface->UnregisterAllAddOnContentPath();
            }

            Result RefreshApplicationAddOnContent(const ncm::ApplicationId *ids, size_t num_ids) {
                AMS_ASSERT(this->interface);
                return this->interface->RefreshApplicationAddOnContent(sf::InArray<ncm::ApplicationId>(ids, num_ids));
            }

            Result UnregisterApplicationAddOnContent(ncm::ApplicationId id) {
                AMS_ASSERT(this->interface);
                return this->interface->UnregisterApplicationAddOnContent(id);
            }
    };

}
