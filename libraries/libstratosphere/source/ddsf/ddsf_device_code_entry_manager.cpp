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
#include <stratosphere.hpp>

namespace ams::ddsf {

    Result DeviceCodeEntryManager::Add(DeviceCode device_code, IDevice *device) {
        /* Check pre-conditions. */
        AMS_ASSERT(device != nullptr);
        AMS_ASSERT(device->IsDriverAttached());

        /* Acquire exclusive access to the manager. */
        std::scoped_lock lk(this->entry_list_lock);

        /* Check that we don't already have an entry with the code. */
        for (const auto &holder : this->entry_list) {
            AMS_ASSERT(holder.IsConstructed());
            AMS_ASSERT(holder.Get().GetDeviceCode() != device_code);
        }

        /* Allocate memory for a new device code entry holder. */
        void *holder_storage = this->memory_resource->Allocate(sizeof(DeviceCodeEntryHolder));
        R_UNLESS(holder_storage != nullptr, ddsf::ResultOutOfResource());

        /* Initialize the new holder. */
        auto *holder = new (static_cast<DeviceCodeEntryHolder *>(holder_storage)) DeviceCodeEntryHolder;
        holder->Construct(device_code, device);

        /* Link the new holder. */
        holder->AddTo(this->entry_list);

        return ResultSuccess();
    }

    bool DeviceCodeEntryManager::Remove(DeviceCode device_code) {
        /* Acquire exclusive access to the manager. */
        std::scoped_lock lk(this->entry_list_lock);

        /* Find and erase the entry. */
        bool erased = false;
        for (auto it = this->entry_list.begin(); it != this->entry_list.end(); /* ... */) {
            /* Get the current entry, and advance the iterator. */
            DeviceCodeEntryHolder *cur = std::addressof(*(it++));

            /* If the entry matches the device code, remove it. */
            AMS_ASSERT(cur->IsConstructed());
            if (cur->Get().GetDeviceCode() == device_code) {
                /* Destroy and deallocate the holder. */
                cur->Destroy();
                cur->~DeviceCodeEntryHolder();
                this->memory_resource->Deallocate(cur, sizeof(*cur));

                erased = true;
            }
        }

        return erased;
    }

    Result DeviceCodeEntryManager::FindDeviceCodeEntry(DeviceCodeEntry **out, DeviceCode device_code) {
        /* Check arguments. */
        AMS_ASSERT(out != nullptr);
        R_UNLESS(out != nullptr, ddsf::ResultInvalidArgument());

        /* Find the device. */
        bool found = false;
        this->ForEachEntry([&](DeviceCodeEntry &entry) -> bool {
            if (entry.GetDeviceCode() == device_code) {
                found = true;
                *out = std::addressof(entry);
                return false;
            }
            return true;
        });

        /* Check that we found the device. */
        R_UNLESS(found, ddsf::ResultDeviceCodeNotFound());

        return ResultSuccess();
    }

    Result DeviceCodeEntryManager::FindDeviceCodeEntry(const DeviceCodeEntry **out, DeviceCode device_code) const {
        /* Check arguments. */
        AMS_ASSERT(out != nullptr);
        R_UNLESS(out != nullptr, ddsf::ResultInvalidArgument());

        /* Find the device. */
        bool found = false;
        this->ForEachEntry([&](const DeviceCodeEntry &entry) -> bool {
            if (entry.GetDeviceCode() == device_code) {
                found = true;
                *out = std::addressof(entry);
                return false;
            }
            return true;
        });

        /* Check that we found the device. */
        R_UNLESS(found, ddsf::ResultDeviceCodeNotFound());

        return ResultSuccess();
    }

    Result DeviceCodeEntryManager::FindDevice(IDevice **out, DeviceCode device_code) {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);

        /* Find the entry. */
        DeviceCodeEntry *entry;
        R_TRY(this->FindDeviceCodeEntry(std::addressof(entry), device_code));

        /* Set the output. */
        if (out != nullptr) {
            *out = std::addressof(entry->GetDevice());
        }

        return ResultSuccess();
    }

    Result DeviceCodeEntryManager::FindDevice(const IDevice **out, DeviceCode device_code) const {
        /* Check pre-conditions. */
        AMS_ASSERT(out != nullptr);

        /* Find the entry. */
        const DeviceCodeEntry *entry;
        R_TRY(this->FindDeviceCodeEntry(std::addressof(entry), device_code));

        /* Set the output. */
        if (out != nullptr) {
            *out = std::addressof(entry->GetDevice());
        }

        return ResultSuccess();
    }

}
