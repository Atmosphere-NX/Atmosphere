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
#include <stratosphere/os/os_sdk_mutex.hpp>
#include <stratosphere/ddsf/ddsf_types.hpp>
#include <stratosphere/ddsf/impl/ddsf_for_each.hpp>
#include <stratosphere/ddsf/ddsf_i_castable.hpp>
#include <stratosphere/ddsf/ddsf_i_session.hpp>

namespace ams::ddsf {

    class IDriver;

    class IDevice : public ICastable {
        friend Result OpenSession(IDevice *device, ISession *session, AccessMode mode);
        friend void CloseSession(ISession *session);
        friend class IDriver;
        public:
            AMS_DDSF_CASTABLE_ROOT_TRAITS(ams::ddsf::IDevice);
        private:
            util::IntrusiveListNode m_list_node;
            IDriver *m_driver;
            ISession::List m_session_list;
            mutable os::SdkMutex m_session_list_lock;
            bool m_is_exclusive_write;
        public:
            using ListTraits = util::IntrusiveListMemberTraits<&IDevice::m_list_node>;
            using List       = typename ListTraits::ListType;
            friend class util::IntrusiveList<IDevice, util::IntrusiveListMemberTraits<&IDevice::m_list_node>>;
        private:
            Result AttachSession(ISession *session) {
                AMS_ASSERT(session != nullptr);
                std::scoped_lock lk(m_session_list_lock);

                /* Check if we're allowed to attach the session. */
                if (m_is_exclusive_write && session->CheckExclusiveWrite()) {
                    for (const auto &attached : m_session_list) {
                        R_UNLESS(!attached.CheckAccess(AccessMode_Write), ddsf::ResultAccessModeDenied());
                    }
                }

                /* Attach the session. */
                m_session_list.push_back(*session);
                return ResultSuccess();
            }

            void DetachSession(ISession *session) {
                AMS_ASSERT(session != nullptr);
                std::scoped_lock lk(m_session_list_lock);
                m_session_list.erase(m_session_list.iterator_to(*session));
            }

            void AttachDriver(IDriver *drv) {
                AMS_ASSERT(drv != nullptr);
                AMS_ASSERT(!this->IsDriverAttached());
                m_driver = drv;
                AMS_ASSERT(this->IsDriverAttached());
            }

            void DetachDriver() {
                AMS_ASSERT(this->IsDriverAttached());
                m_driver = nullptr;
                AMS_ASSERT(!this->IsDriverAttached());
            }
        public:
            IDevice(bool exclusive_write) : m_list_node(), m_driver(nullptr), m_session_list(), m_session_list_lock(), m_is_exclusive_write(exclusive_write) {
                m_session_list.clear();
            }
        protected:
            ~IDevice() {
                m_session_list.clear();
            }
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

            IDriver &GetDriver() {
                AMS_ASSERT(this->IsDriverAttached());
                return *m_driver;
            }

            const IDriver &GetDriver() const {
                AMS_ASSERT(this->IsDriverAttached());
                return *m_driver;
            }

            bool IsDriverAttached() const {
                return m_driver != nullptr;
            }

            template<typename F>
            Result ForEachSession(F f, bool return_on_fail) {
                return impl::ForEach(m_session_list_lock, m_session_list, f, return_on_fail);
            }

            template<typename F>
            Result ForEachSession(F f, bool return_on_fail) const {
                return impl::ForEach(m_session_list_lock, m_session_list, f, return_on_fail);
            }

            template<typename F>
            int ForEachSession(F f) {
                return impl::ForEach(m_session_list_lock, m_session_list, f);
            }

            template<typename F>
            int ForEachSession(F f) const {
                return impl::ForEach(m_session_list_lock, m_session_list, f);
            }

            bool HasAnyOpenSession() const {
                return !m_session_list.empty();
            }
    };

}
