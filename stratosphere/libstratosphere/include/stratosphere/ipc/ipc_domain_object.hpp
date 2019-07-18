/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <algorithm>
#include <memory>
#include <type_traits>

#include "ipc_service_object.hpp"

class IDomainObject;

class DomainManager {
    public:
        static constexpr u32 MinimumDomainId = 1;
    public:
        virtual std::shared_ptr<IDomainObject> AllocateDomain() = 0;
        virtual void FreeDomain(IDomainObject *domain) = 0;
        virtual Result ReserveObject(IDomainObject *domain, u32 *out_object_id) = 0;
        virtual Result ReserveSpecificObject(IDomainObject *domain, u32 object_id) = 0;
        virtual void SetObject(IDomainObject *domain, u32 object_id, ServiceObjectHolder&& holder) = 0;
        virtual ServiceObjectHolder *GetObject(IDomainObject *domain, u32 object_id) = 0;
        virtual Result FreeObject(IDomainObject *domain, u32 object_id) = 0;
        virtual Result ForceFreeObject(u32 object_id) = 0;
};

class IDomainObject : public IServiceObject {
    private:
        DomainManager *manager;
    public:
        IDomainObject(DomainManager *m) : manager(m) {}

        virtual ~IDomainObject() override {
            this->manager->FreeDomain(this);
        }

        DomainManager *GetManager() {
            return this->manager;
        }

        ServiceObjectHolder *GetObject(u32 object_id) {
            return this->manager->GetObject(this, object_id);
        }

        Result ReserveObject(u32 *out_object_id) {
            return this->manager->ReserveObject(this, out_object_id);
        }

        Result ReserveSpecificObject(u32 object_id) {
            return this->manager->ReserveSpecificObject(this, object_id);
        }

        void SetObject(u32 object_id, ServiceObjectHolder&& holder) {
            this->manager->SetObject(this, object_id, std::move(holder));
        }

        Result FreeObject(u32 object_id) {
            return this->manager->FreeObject(this, object_id);
        }

        Result ForceFreeObject(u32 object_id) {
            return this->manager->ForceFreeObject(object_id);
        }

    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* IDomainObject has no callable functions. */
        };
};

static constexpr bool IsDomainObject(ServiceObjectHolder &holder) {
    return holder.GetServiceId() == ServiceObjectId<IDomainObject>();
}

static constexpr bool IsDomainObject(ServiceObjectHolder *holder) {
    return holder->GetServiceId() == ServiceObjectId<IDomainObject>();
}

/* Out for service impl. */
template <typename ServiceImpl>
class Out<std::shared_ptr<ServiceImpl>> : public OutSessionTag {
    static_assert(std::is_base_of_v<IServiceObject, ServiceImpl>, "OutSessions must be shared_ptr<IServiceObject>!");

    template<typename, typename>
    friend class Out;

    private:
        std::shared_ptr<ServiceImpl> *srv;
        IDomainObject *domain = nullptr;
        u32 *object_id = nullptr;
    public:
        Out<std::shared_ptr<ServiceImpl>>(std::shared_ptr<IServiceObject> *s, IDomainObject *dm, u32 *o) : srv(reinterpret_cast<std::shared_ptr<ServiceImpl> *>(s)), domain(dm), object_id(o) { }

        ServiceObjectHolder GetHolder() {
            std::shared_ptr<ServiceImpl> clone = *srv;
            return ServiceObjectHolder(std::move(clone));
        }

        bool IsDomain() {
            return domain != nullptr;
        }

        u32 GetObjectId() {
            return *object_id;
        }

        void ChangeObjectId(u32 o) {
            domain->ForceFreeObject(*object_id);
            domain->ReserveSpecificObject(o);
            *object_id = o;
        }

        void SetValue(std::shared_ptr<ServiceImpl> &&s) {
            *this->srv = std::move(s);
        }
};
