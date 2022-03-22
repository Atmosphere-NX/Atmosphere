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
#include <stratosphere/sf/sf_service_object.hpp>
#include <stratosphere/sf/cmif/sf_cmif_service_dispatch.hpp>

namespace ams::sf::cmif {

    class ServiceObjectHolder {
        private:
            SharedPointer<IServiceObject> m_srv;
            const ServiceDispatchMeta *m_dispatch_meta;
        private:
            /* Copy constructor. */
            ServiceObjectHolder(const ServiceObjectHolder &o) : m_srv(o.m_srv), m_dispatch_meta(o.m_dispatch_meta) { /* ... */ }
            ServiceObjectHolder &operator=(const ServiceObjectHolder &o) = delete;
        public:
            /* Default constructor, null all members. */
            ServiceObjectHolder() : m_srv(nullptr, false), m_dispatch_meta(nullptr) { /* ... */ }

            ~ServiceObjectHolder() {
                m_dispatch_meta = nullptr;
            }

            /* Ensure correct type id at runtime through template constructor. */
            template<typename ServiceImpl>
            constexpr explicit ServiceObjectHolder(SharedPointer<ServiceImpl> &&s) : m_srv(std::move(s)), m_dispatch_meta(GetServiceDispatchMeta<ServiceImpl>()) {
                /* ... */
            }

            /* Move constructor, assignment operator. */
            ServiceObjectHolder(ServiceObjectHolder &&o) : m_srv(std::move(o.m_srv)), m_dispatch_meta(std::move(o.m_dispatch_meta)) {
                o.m_dispatch_meta = nullptr;
            }

            ServiceObjectHolder &operator=(ServiceObjectHolder &&o) {
                ServiceObjectHolder tmp(std::move(o));
                tmp.swap(*this);
                return *this;
            }

            /* State management. */
            void swap(ServiceObjectHolder &o) {
                m_srv.swap(o.m_srv);
                std::swap(m_dispatch_meta, o.m_dispatch_meta);
            }

            void Reset() {
                m_srv = nullptr;
                m_dispatch_meta = nullptr;
            }

            ServiceObjectHolder Clone() const {
                return ServiceObjectHolder(*this);
            }

            /* Boolean operators. */
            explicit constexpr operator bool() const {
                return m_srv != nullptr;
            }

            constexpr bool operator!() const {
                return m_srv == nullptr;
            }

            /* Getters. */
            constexpr uintptr_t GetServiceId() const {
                if (m_dispatch_meta) {
                    return m_dispatch_meta->GetServiceId();
                }
                return 0;
            }

            template<typename Interface>
            constexpr inline bool IsServiceObjectValid() const {
                return this->GetServiceId() == GetServiceDispatchMeta<Interface>()->GetServiceId();
            }

            template<typename Interface>
            inline Interface *GetServiceObject() const {
                if (this->GetServiceId() == GetServiceDispatchMeta<Interface>()->GetServiceId()) {
                    return static_cast<Interface *>(m_srv.Get());
                }
                return nullptr;
            }

            inline sf::IServiceObject *GetServiceObjectUnsafe() const {
                return static_cast<sf::IServiceObject *>(m_srv.Get());
            }

            /* Processing. */
            Result ProcessMessage(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const;
    };

}
