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
#include <memory>
#include <type_traits>

#include "ipc_out.hpp"

#include "../firmware_version.hpp"

class IpcResponseContext;

struct ServiceCommandMeta {
    FirmwareVersion fw_low = FirmwareVersion_Max;
    FirmwareVersion fw_high = FirmwareVersion_Max;
    u32 cmd_id = 0;
    Result (*handler)(IpcResponseContext *)  = nullptr;
};

class IServiceObject  {
    public:
        virtual ~IServiceObject() { }
        virtual bool IsMitmObject() const { return false; }
};

#define SERVICE_DISPATCH_TABLE_NAME s_DispatchTable
#define DEFINE_SERVICE_DISPATCH_TABLE static constexpr ServiceCommandMeta SERVICE_DISPATCH_TABLE_NAME[]

template <typename T>
static constexpr size_t DispatchTableEntryCount() {
    static_assert(std::is_base_of<IServiceObject, T>::value, "DispatchTable owners must derive from IServiceObject");
    return sizeof(T::SERVICE_DISPATCH_TABLE_NAME)/sizeof(ServiceCommandMeta);
}
template <typename T>
static constexpr const ServiceCommandMeta* DispatchTable() {
    static_assert(std::is_base_of<IServiceObject, T>::value, "DispatchTable owners must derive from IServiceObject");
    return reinterpret_cast<const ServiceCommandMeta*>(&T::SERVICE_DISPATCH_TABLE_NAME);
}

template <typename T>
static constexpr uintptr_t ServiceObjectId() {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    return reinterpret_cast<uintptr_t>(&T::SERVICE_DISPATCH_TABLE_NAME);
}

class ServiceObjectHolder {
    private:
        std::shared_ptr<IServiceObject> srv;
        const ServiceCommandMeta *dispatch_table;
        size_t entry_count;

        /* Private copy constructor. */
        ServiceObjectHolder(const ServiceObjectHolder& other) : srv(other.srv), dispatch_table(other.dispatch_table), entry_count(other.entry_count) { }
        ServiceObjectHolder& operator=(const ServiceObjectHolder& other);
    public:
        /* Templated constructor ensures correct type id at runtime. */
        template <typename ServiceImpl>
        explicit ServiceObjectHolder(std::shared_ptr<ServiceImpl>&& s) : srv(std::move(s)), dispatch_table(DispatchTable<ServiceImpl>()), entry_count(DispatchTableEntryCount<ServiceImpl>()) { }

        template <typename ServiceImpl>
        ServiceImpl *GetServiceObject() const {
            if (GetServiceId() == ServiceObjectId<ServiceImpl>()) {
                return static_cast<ServiceImpl *>(this->srv.get());
            }
            return nullptr;
        }

        template<typename ServiceImpl>
        ServiceImpl *GetServiceObjectUnsafe() const {
            return static_cast<ServiceImpl *>(this->srv.get());
        }

        const ServiceCommandMeta *GetDispatchTable() const {
            return this->dispatch_table;
        }

        size_t GetDispatchTableEntryCount() const {
            return this->entry_count;
        }

        constexpr uintptr_t GetServiceId() const {
            return reinterpret_cast<uintptr_t>(this->dispatch_table);
        }

        bool IsMitmObject() const {
            return this->srv->IsMitmObject();
        }

        /* Default constructor, move constructor, move assignment operator. */
        ServiceObjectHolder() : srv(nullptr), dispatch_table(nullptr) { }

        ServiceObjectHolder(ServiceObjectHolder&& other) : srv(std::move(other.srv)), dispatch_table(std::move(other.dispatch_table)), entry_count(std::move(other.entry_count)) { }

        ServiceObjectHolder& operator=(ServiceObjectHolder&& other) {
            this->srv = other.srv;
            this->dispatch_table = other.dispatch_table;
            this->entry_count = other.entry_count;
            other.Reset();
            return *this;
        }

        explicit operator bool() const {
            return this->srv != nullptr;
        }

        bool operator!() const {
            return this->srv == nullptr;
        }

        void Reset() {
            this->srv.reset();
            this->dispatch_table = nullptr;
            this->entry_count = 0;
        }

        ServiceObjectHolder Clone() const {
            ServiceObjectHolder clone(*this);
            return clone;
        }
};
