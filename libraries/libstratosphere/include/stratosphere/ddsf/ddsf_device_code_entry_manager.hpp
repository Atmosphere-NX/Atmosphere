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
#include <stratosphere/ddsf/ddsf_types.hpp>
#include <stratosphere/ddsf/impl/ddsf_for_each.hpp>
#include <stratosphere/ddsf/ddsf_device_code_entry.hpp>

namespace ams::ddsf {

    class IDevice;

    class DeviceCodeEntryManager {
        private:
            ams::MemoryResource *memory_resource;
            ddsf::DeviceCodeEntryHolder::List entry_list;
            mutable os::SdkMutex entry_list_lock;
        private:
            void DestroyAllEntries() {
                auto it = this->entry_list.begin();
                while (it != this->entry_list.end()) {
                    ddsf::DeviceCodeEntryHolder *entry = std::addressof(*it);
                    it = this->entry_list.erase(it);

                    AMS_ASSERT(entry->IsConstructed());
                    if (entry->IsConstructed()) {
                        entry->Destroy();
                    }

                    this->memory_resource->Deallocate(entry, sizeof(*entry));
                }
            }
        public:
            DeviceCodeEntryManager(ams::MemoryResource *mr) : memory_resource(mr), entry_list(), entry_list_lock() { /* ... */ }

            ~DeviceCodeEntryManager() {
                this->DestroyAllEntries();
            }

            void Reset() {
                std::scoped_lock lk(this->entry_list_lock);
                this->DestroyAllEntries();
            }

            Result Add(DeviceCode device_code, IDevice *device);
            bool Remove(DeviceCode device_code);

            Result FindDeviceCodeEntry(DeviceCodeEntry **out, DeviceCode device_code);
            Result FindDeviceCodeEntry(const DeviceCodeEntry **out, DeviceCode device_code) const;

            Result FindDevice(IDevice **out, DeviceCode device_code);
            Result FindDevice(const IDevice **out, DeviceCode device_code) const;

            template<typename F>
            int ForEachEntry(F f) {
                return impl::ForEach(this->entry_list_lock, this->entry_list, [&](DeviceCodeEntryHolder &holder) -> bool {
                    AMS_ASSERT(holder.IsConstructed());
                    return f(holder.Get());
                });
            }

            template<typename F>
            int ForEachEntry(F f) const {
                return impl::ForEach(this->entry_list_lock, this->entry_list, [&](const DeviceCodeEntryHolder &holder) -> bool {
                    AMS_ASSERT(holder.IsConstructed());
                    return f(holder.Get());
                });
            }
    };

}
