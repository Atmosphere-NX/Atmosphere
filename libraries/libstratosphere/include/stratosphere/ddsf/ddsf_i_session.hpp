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
#include <stratosphere/ddsf/ddsf_types.hpp>
#include <stratosphere/ddsf/impl/ddsf_for_each.hpp>
#include <stratosphere/ddsf/ddsf_i_castable.hpp>

namespace ams::ddsf {

    class ISession;
    class IDevice;

    Result OpenSession(IDevice *device, ISession *session, AccessMode access_mode);
    void CloseSession(ISession *session);

    class ISession : public ICastable {
        friend Result OpenSession(IDevice *device, ISession *session, AccessMode mode);
        friend void CloseSession(ISession *session);
        public:
            AMS_DDSF_CASTABLE_ROOT_TRAITS(ams::ddsf::IDevice);
        private:
            util::IntrusiveListNode m_list_node;
            IDevice *m_device;
            AccessMode m_access_mode;
        public:
            using ListTraits = util::IntrusiveListMemberTraits<&ISession::m_list_node>;
            using List       = typename ListTraits::ListType;
            friend class util::IntrusiveList<ISession, util::IntrusiveListMemberTraits<&ISession::m_list_node>>;
        private:
            void AttachDevice(IDevice *dev, AccessMode mode) {
                AMS_ASSERT(dev != nullptr);
                AMS_ASSERT(!this->IsOpen());
                m_device      = dev;
                m_access_mode = mode;
                AMS_ASSERT(this->IsOpen());
            }

            void DetachDevice() {
                /* AMS_ASSERT(this->IsOpen()); */
                m_device      = nullptr;
                m_access_mode = AccessMode_None;
                AMS_ASSERT(!this->IsOpen());
            }
        public:
            ISession() : m_list_node(), m_device(nullptr), m_access_mode() { /* ... */ }
        protected:
            virtual ~ISession() { this->DetachDevice(); AMS_ASSERT(!this->IsOpen()); }
        public:
            void AddTo(List &list) {
                list.push_back(*this);
            }

            void RemoveFrom(List list) {
                list.erase(list.iterator_to(*this));
            }

            bool IsLinkedToList() const {
                return m_list_node.IsLinked();
            }

            IDevice &GetDevice() {
                AMS_ASSERT(this->IsOpen());
                return *m_device;
            }

            const IDevice &GetDevice() const {
                AMS_ASSERT(this->IsOpen());
                return *m_device;
            }

            bool IsOpen() const {
                return m_device != nullptr;
            }

            bool CheckAccess(AccessMode mode) const {
                AMS_ASSERT(this->IsOpen());
                return ((~m_access_mode) & mode) == 0;
            }

            bool CheckExclusiveWrite() const {
                return this->CheckAccess(AccessMode_Write) && !this->CheckAccess(AccessMode_Shared);
            }
    };

}
