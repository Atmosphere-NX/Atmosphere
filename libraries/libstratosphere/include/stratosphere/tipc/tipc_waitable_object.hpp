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
#include <vapours.hpp>
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_service_object.hpp>

namespace ams::tipc {

    class WaitableObject {
        public:
            enum ObjectType : u8 {
                ObjectType_Invalid = 0,
                ObjectType_Port    = 1,
                ObjectType_Session = 2,
            };
        private:
            svc::Handle m_handle;
            ObjectType m_type;
            bool m_managed;
            tipc::ServiceObjectBase *m_object;
        private:
            void InitializeImpl(ObjectType type, svc::Handle handle, bool managed, tipc::ServiceObjectBase *object) {
                /* Validate that the object isn't already constructed. */
                AMS_ASSERT(m_type == ObjectType_Invalid);

                /* Set all fields. */
                m_handle  = handle;
                m_type    = type;
                m_managed = managed;
                m_object  = object;
            }
        public:
            constexpr inline WaitableObject() : m_handle(svc::InvalidHandle), m_type(ObjectType_Invalid), m_managed(false), m_object(nullptr) { /* ... */ }

            void InitializeAsPort(svc::Handle handle) {
                /* NOTE: Nintendo sets ports as managed, but this will cause a nullptr-deref if one is ever closed. */
                /* This is theoretically a non-issue, as ports can't be closed, but we will set ours as unmanaged, */
                /* just in case. */
                this->InitializeImpl(ObjectType_Port, handle, false, nullptr);
            }

            void InitializeAsSession(svc::Handle handle, bool managed, tipc::ServiceObjectBase *object) {
                this->InitializeImpl(ObjectType_Session, handle, managed, object);
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
                m_handle  = svc::InvalidHandle;
                m_type    = ObjectType_Invalid;
                m_managed = false;
                m_object  = nullptr;
            }

            constexpr svc::Handle GetHandle() const {
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

