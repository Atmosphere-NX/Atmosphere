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
#include <vapours.hpp>
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_service_object_base.hpp>

namespace ams::tipc {

    class ObjectHolder {
        public:
            enum ObjectType : u8 {
                ObjectType_Invalid = 0,
                ObjectType_Port    = 1,
                ObjectType_Session = 2,

                ObjectType_Deferral = ObjectType_Invalid,
            };
        private:
            os::NativeHandle m_handle;
            ObjectType m_type;
            bool m_managed;
            tipc::ServiceObjectBase *m_object;
        private:
            void InitializeImpl(ObjectType type, os::NativeHandle handle, bool managed, tipc::ServiceObjectBase *object) {
                /* Validate that the object isn't already constructed. */
                AMS_ASSERT(m_type == ObjectType_Invalid);

                /* Set all fields. */
                m_handle  = handle;
                m_type    = type;
                m_managed = managed;
                m_object  = object;
            }
        public:
            constexpr inline ObjectHolder() : m_handle(os::InvalidNativeHandle), m_type(ObjectType_Invalid), m_managed(false), m_object(nullptr) { /* ... */ }

            void InitializeAsPort(os::NativeHandle handle) {
                /* NOTE: Nintendo sets ports as managed, but this will cause a nullptr-deref if one is ever closed. */
                /* This is theoretically a non-issue, as ports can't be closed, but we will set ours as unmanaged, */
                /* just in case. */
                this->InitializeImpl(ObjectType_Port, handle, false, nullptr);
            }

            void InitializeAsSession(os::NativeHandle handle, bool managed, tipc::ServiceObjectBase *object) {
                this->InitializeImpl(ObjectType_Session, handle, managed, object);
            }

            void InitializeForDeferralManager(os::NativeHandle handle, tipc::ServiceObjectBase *object) {
                this->InitializeImpl(ObjectType_Deferral, handle, false, object);
            }

            void Destroy() {
                /* Validate that the object is constructed. */
                AMS_ASSERT(m_type != ObjectType_Invalid);

                /* If we're managed, destroy the associated object. */
                if (m_managed) {
                    if (auto * const deleter = m_object->GetDeleter(); deleter != nullptr) {
                        deleter->DeleteServiceObject(m_object);
                    }
                }

                /* Reset all fields. */
                m_handle  = os::InvalidNativeHandle;
                m_type    = ObjectType_Invalid;
                m_managed = false;
                m_object  = nullptr;
            }

            constexpr os::NativeHandle GetHandle() const {
                return m_handle;
            }

            constexpr ObjectType GetType() const {
                return m_type;
            }

            constexpr tipc::ServiceObjectBase *GetObject() const {
                return m_object;
            }
    };

}

