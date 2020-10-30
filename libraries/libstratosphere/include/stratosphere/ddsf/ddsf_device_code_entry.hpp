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

namespace ams::ddsf {

    class IDevice;

    class DeviceCodeEntry {
        NON_COPYABLE(DeviceCodeEntry);
        NON_MOVEABLE(DeviceCodeEntry);
        private:
            ams::DeviceCode device_code = ams::InvalidDeviceCode;
            IDevice *device = nullptr;
        public:
            constexpr DeviceCodeEntry(ams::DeviceCode dc, IDevice *dev) : device_code(dc), device(dev) {
                AMS_ASSERT(dev != nullptr);
            }

            constexpr ams::DeviceCode GetDeviceCode() const {
                return this->device_code;
            }

            constexpr IDevice &GetDevice() {
                return *this->device;
            }

            constexpr const IDevice &GetDevice() const {
                return *this->device;
            }
    };

    class DeviceCodeEntryHolder {
        NON_COPYABLE(DeviceCodeEntryHolder);
        NON_MOVEABLE(DeviceCodeEntryHolder);
        private:
            util::IntrusiveListNode list_node;
            TYPED_STORAGE(DeviceCodeEntry) entry_storage;
            bool is_constructed;
        public:
            using ListTraits = util::IntrusiveListMemberTraitsDeferredAssert<&DeviceCodeEntryHolder::list_node>;
            using List       = typename ListTraits::ListType;
            friend class util::IntrusiveList<DeviceCodeEntryHolder, util::IntrusiveListMemberTraitsDeferredAssert<&DeviceCodeEntryHolder::list_node>>;
        public:
            DeviceCodeEntryHolder() : list_node(), entry_storage(), is_constructed(false) {
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
                return this->list_node.IsLinked();
            }

            DeviceCodeEntry &Construct(DeviceCode dc, IDevice *dev) {
                AMS_ASSERT(!this->IsConstructed());
                DeviceCodeEntry *entry = new (GetPointer(this->entry_storage)) DeviceCodeEntry(dc, dev);
                this->is_constructed = true;
                return *entry;
            }

            bool IsConstructed() const {
                return this->is_constructed;
            }

            void Destroy() {
                AMS_ASSERT(this->IsConstructed());
                GetReference(this->entry_storage).~DeviceCodeEntry();
                this->is_constructed = false;
            }

            DeviceCodeEntry &Get() {
                AMS_ASSERT(this->IsConstructed());
                return GetReference(this->entry_storage);
            }

            const DeviceCodeEntry &Get() const {
                AMS_ASSERT(this->IsConstructed());
                return GetReference(this->entry_storage);
            }
    };
    static_assert(DeviceCodeEntryHolder::ListTraits::IsValid());

}
