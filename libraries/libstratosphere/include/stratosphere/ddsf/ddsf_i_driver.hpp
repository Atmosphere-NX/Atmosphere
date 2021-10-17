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
#include <stratosphere/ddsf/ddsf_i_device.hpp>

namespace ams::ddsf {

    class IDriver : public ICastable {
        public:
            AMS_DDSF_CASTABLE_ROOT_TRAITS(ams::ddsf::IDriver);
        private:
            util::IntrusiveListNode m_list_node;
            IDevice::List m_device_list;
            mutable os::SdkMutex m_device_list_lock;
        public:
            using ListTraits = util::IntrusiveListMemberTraits<&IDriver::m_list_node>;
            using List       = typename ListTraits::ListType;
            friend class util::IntrusiveList<IDriver, util::IntrusiveListMemberTraits<&IDriver::m_list_node>>;
        private:
        public:
            IDriver() : m_list_node(), m_device_list(), m_device_list_lock() {
                m_device_list.clear();
            }
        protected:
            ~IDriver() {
                m_device_list.clear();
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

            bool HasAnyDevice() const {
                return !m_device_list.empty();
            }

            void RegisterDevice(IDevice *dev) {
                AMS_ASSERT(dev != nullptr);
                std::scoped_lock lk(m_device_list_lock);
                dev->AttachDriver(this);
                m_device_list.push_back(*dev);
            }

            void UnregisterDevice(IDevice *dev) {
                AMS_ASSERT(dev != nullptr);
                std::scoped_lock lk(m_device_list_lock);
                m_device_list.erase(m_device_list.iterator_to(*dev));
                dev->DetachDriver();
            }

            template<typename F>
            Result ForEachDevice(F f, bool return_on_fail) {
                return impl::ForEach(m_device_list_lock, m_device_list, f, return_on_fail);
            }

            template<typename F>
            Result ForEachDevice(F f, bool return_on_fail) const {
                return impl::ForEach(m_device_list_lock, m_device_list, f, return_on_fail);
            }

            template<typename F>
            int ForEachDevice(F f) {
                return impl::ForEach(m_device_list_lock, m_device_list, f);
            }

            template<typename F>
            int ForEachDevice(F f) const {
                return impl::ForEach(m_device_list_lock, m_device_list, f);
            }
    };

}
