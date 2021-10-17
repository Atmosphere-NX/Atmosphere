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

namespace ams::ddsf {

    class IDevice;

    class DeviceCodeEntry {
        NON_COPYABLE(DeviceCodeEntry);
        NON_MOVEABLE(DeviceCodeEntry);
        private:
            ams::DeviceCode m_device_code = ams::InvalidDeviceCode;
            IDevice *m_device = nullptr;
        public:
            constexpr DeviceCodeEntry(ams::DeviceCode dc, IDevice *dev) : m_device_code(dc), m_device(dev) {
                AMS_ASSERT(dev != nullptr);
            }

            constexpr ams::DeviceCode GetDeviceCode() const {
                return m_device_code;
            }

            constexpr IDevice &GetDevice() {
                return *m_device;
            }

            constexpr const IDevice &GetDevice() const {
                return *m_device;
            }
    };

    class DeviceCodeEntryHolder {
        NON_COPYABLE(DeviceCodeEntryHolder);
        NON_MOVEABLE(DeviceCodeEntryHolder);
        private:
            util::IntrusiveListNode m_list_node;
            util::TypedStorage<DeviceCodeEntry> m_entry_storage;
            bool m_is_constructed;
        public:
            using ListTraits = util::IntrusiveListMemberTraits<&DeviceCodeEntryHolder::m_list_node>;
            using List       = typename ListTraits::ListType;
            friend class util::IntrusiveList<DeviceCodeEntryHolder, util::IntrusiveListMemberTraits<&DeviceCodeEntryHolder::m_list_node>>;
        public:
            DeviceCodeEntryHolder() : m_list_node(), m_entry_storage(), m_is_constructed(false) {
                /* ... */
            }

            ~DeviceCodeEntryHolder() {
                if (this->IsConstructed()) {
                    this->Destroy();
                }
            }

            void AddTo(List &list) {
                list.push_back(*this);
            }

            void RemoveFrom(List list) {
                list.erase(list.iterator_to(*this));
            }

            bool IsLinkedToList() const {
                return m_list_node.IsLinked();
            }

            DeviceCodeEntry &Construct(DeviceCode dc, IDevice *dev) {
                AMS_ASSERT(!this->IsConstructed());
                DeviceCodeEntry *entry = util::ConstructAt(m_entry_storage, dc, dev);
                m_is_constructed = true;
                return *entry;
            }

            bool IsConstructed() const {
                return m_is_constructed;
            }

            void Destroy() {
                AMS_ASSERT(this->IsConstructed());
                util::DestroyAt(m_entry_storage);
                m_is_constructed = false;
            }

            DeviceCodeEntry &Get() {
                AMS_ASSERT(this->IsConstructed());
                return GetReference(m_entry_storage);
            }

            const DeviceCodeEntry &Get() const {
                AMS_ASSERT(this->IsConstructed());
                return GetReference(m_entry_storage);
            }
    };

}
