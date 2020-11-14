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
            util::IntrusiveListNode list_node;
            IDriver *driver;
            ISession::List session_list;
            mutable os::SdkMutex session_list_lock;
            bool is_exclusive_write;
        public:
            using ListTraits = util::IntrusiveListMemberTraitsDeferredAssert<&IDevice::list_node>;
            using List       = typename ListTraits::ListType;
            friend class util::IntrusiveList<IDevice, util::IntrusiveListMemberTraitsDeferredAssert<&IDevice::list_node>>;
        private:
            Result AttachSession(ISession *session) {
                AMS_ASSERT(session != nullptr);
                std::scoped_lock lk(this->session_list_lock);

                /* Check if we're allowed to attach the session. */
                if (this->is_exclusive_write && session->CheckExclusiveWrite()) {
                    for (const auto &attached : this->session_list) {
                        R_UNLESS(!attached.CheckAccess(AccessMode_Write), ddsf::ResultAccessModeDenied());
                    }
                }

                /* Attach the session. */
                this->session_list.push_back(*session);
                return ResultSuccess();
            }

            void DetachSession(ISession *session) {
                AMS_ASSERT(session != nullptr);
                std::scoped_lock lk(this->session_list_lock);
                this->session_list.erase(this->session_list.iterator_to(*session));
            }

            void AttachDriver(IDriver *drv) {
                AMS_ASSERT(drv != nullptr);
                AMS_ASSERT(!this->IsDriverAttached());
                this->driver = drv;
                AMS_ASSERT(this->IsDriverAttached());
            }

            void DetachDriver() {
                AMS_ASSERT(this->IsDriverAttached());
                this->driver = nullptr;
                AMS_ASSERT(!this->IsDriverAttached());
            }
        public:
            IDevice(bool exclusive_write) : list_node(), driver(nullptr), session_list(), session_list_lock(), is_exclusive_write(exclusive_write) {
                this->session_list.clear();
            }
        protected:
            ~IDevice() {
                this->session_list.clear();
            }
        public:
            void AddTo(List &list) {
                list.push_back(*this);
            }

            void RemoveFrom(List list) {
                list.erase(list.iterator_to(*this));
            }

            bool IsLinkedToList() const {
                return this->list_node.IsLinked();
            }

            IDriver &GetDriver() {
                AMS_ASSERT(this->IsDriverAttached());
                return *this->driver;
            }

            const IDriver &GetDriver() const {
                AMS_ASSERT(this->IsDriverAttached());
                return *this->driver;
            }

            bool IsDriverAttached() const {
                return this->driver != nullptr;
            }

            template<typename F>
            Result ForEachSession(F f, bool return_on_fail) {
                return impl::ForEach(this->session_list_lock, this->session_list, f, return_on_fail);
            }

            template<typename F>
            Result ForEachSession(F f, bool return_on_fail) const {
                return impl::ForEach(this->session_list_lock, this->session_list, f, return_on_fail);
            }

            template<typename F>
            int ForEachSession(F f) {
                return impl::ForEach(this->session_list_lock, this->session_list, f);
            }

            template<typename F>
            int ForEachSession(F f) const {
                return impl::ForEach(this->session_list_lock, this->session_list, f);
            }

            bool HasAnyOpenSession() const {
                return !this->session_list.empty();
            }
    };
    static_assert(IDevice::ListTraits::IsValid());

}
