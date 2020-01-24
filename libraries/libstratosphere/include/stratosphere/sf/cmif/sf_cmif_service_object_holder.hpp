/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "../sf_service_object.hpp"
#include "sf_cmif_service_dispatch.hpp"

namespace ams::sf::cmif {

    class ServiceObjectHolder {
        private:
            std::shared_ptr<sf::IServiceObject> srv;
            const ServiceDispatchMeta *dispatch_meta;
        private:
            /* Copy constructor. */
            ServiceObjectHolder(const ServiceObjectHolder &o) : srv(o.srv), dispatch_meta(o.dispatch_meta) { /* ... */ }
            ServiceObjectHolder &operator=(const ServiceObjectHolder &o) = delete;
        public:
            /* Default constructor, null all members. */
            ServiceObjectHolder() : srv(nullptr), dispatch_meta(nullptr) { /* ... */ }

            ~ServiceObjectHolder() {
                this->dispatch_meta = nullptr;
            }

            /* Ensure correct type id at runtime through template constructor. */
            template<typename ServiceImpl>
            constexpr explicit ServiceObjectHolder(std::shared_ptr<ServiceImpl> &&s) {
                this->srv = std::move(s);
                this->dispatch_meta = GetServiceDispatchMeta<ServiceImpl>();
            }

            /* Move constructor, assignment operator. */
            ServiceObjectHolder(ServiceObjectHolder &&o) : srv(std::move(o.srv)), dispatch_meta(std::move(o.dispatch_meta)) {
                o.dispatch_meta = nullptr;
            }

            ServiceObjectHolder &operator=(ServiceObjectHolder &&o) {
                ServiceObjectHolder tmp(std::move(o));
                tmp.Swap(*this);
                return *this;
            }

            /* State management. */
            void Swap(ServiceObjectHolder &o) {
                std::swap(this->srv, o.srv);
                std::swap(this->dispatch_meta, o.dispatch_meta);
            }

            void Reset() {
                this->srv = nullptr;
                this->dispatch_meta = nullptr;
            }

            ServiceObjectHolder Clone() const {
                return ServiceObjectHolder(*this);
            }

            /* Boolean operators. */
            explicit constexpr operator bool() const {
                return this->dispatch_meta != nullptr;
            }

            constexpr bool operator!() const {
                return this->dispatch_meta == nullptr;
            }

            /* Getters. */
            constexpr uintptr_t GetServiceId() const {
                if (this->dispatch_meta) {
                    return this->dispatch_meta->GetServiceId();
                }
                return 0;
            }

            template<typename ServiceImpl>
            constexpr inline bool IsServiceObjectValid() const {
                return this->GetServiceId() == GetServiceDispatchMeta<ServiceImpl>()->GetServiceId();
            }

            template<typename ServiceImpl>
            inline std::shared_ptr<ServiceImpl> GetServiceObject() const {
                if (this->GetServiceId() == GetServiceDispatchMeta<ServiceImpl>()->GetServiceId()) {
                    return std::static_pointer_cast<ServiceImpl>(this->srv);
                }
                return nullptr;
            }

            inline sf::IServiceObject *GetServiceObjectUnsafe() const {
                return this->srv.get();
            }

            /* Processing. */
            Result ProcessMessage(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const;
    };

}
