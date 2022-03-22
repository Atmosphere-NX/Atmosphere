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
#include <stratosphere/lr/lr_i_add_on_content_location_resolver.hpp>

namespace ams::lr {

    class AddOnContentLocationResolver {
        NON_COPYABLE(AddOnContentLocationResolver);
        private:
            sf::SharedPointer<IAddOnContentLocationResolver> m_interface;
        public:
            AddOnContentLocationResolver() { /* ... */ }
            explicit AddOnContentLocationResolver(sf::SharedPointer<IAddOnContentLocationResolver> intf) : m_interface(intf) { /* ... */ }

            AddOnContentLocationResolver(AddOnContentLocationResolver &&rhs) {
                m_interface = std::move(rhs.m_interface);
            }

            AddOnContentLocationResolver &operator=(AddOnContentLocationResolver &&rhs) {
                AddOnContentLocationResolver(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(AddOnContentLocationResolver &rhs) {
                std::swap(m_interface, rhs.m_interface);
            }
        public:
            /* Actual commands. */
            Result ResolveAddOnContentPath(Path *out, ncm::DataId id) {
                AMS_ASSERT(m_interface);
                return m_interface->ResolveAddOnContentPath(out, id);
            }

            Result RegisterAddOnContentStorage(ncm::DataId id, ncm::ApplicationId application_id, ncm::StorageId storage_id) {
                AMS_ASSERT(m_interface);
                if (hos::GetVersion() >= hos::Version_9_0_0) {
                    return m_interface->RegisterAddOnContentStorage(id, application_id, storage_id);
                } else {
                    return m_interface->RegisterAddOnContentStorageDeprecated(id, storage_id);
                }
            }

            Result UnregisterAllAddOnContentPath() {
                AMS_ASSERT(m_interface);
                return m_interface->UnregisterAllAddOnContentPath();
            }

            Result RefreshApplicationAddOnContent(const ncm::ApplicationId *ids, size_t num_ids) {
                AMS_ASSERT(m_interface);
                return m_interface->RefreshApplicationAddOnContent(sf::InArray<ncm::ApplicationId>(ids, num_ids));
            }

            Result UnregisterApplicationAddOnContent(ncm::ApplicationId id) {
                AMS_ASSERT(m_interface);
                return m_interface->UnregisterApplicationAddOnContent(id);
            }
    };

}
