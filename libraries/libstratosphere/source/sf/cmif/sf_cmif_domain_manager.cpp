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
#include <stratosphere.hpp>

namespace ams::sf::cmif {

    ServerDomainManager::Domain::~Domain() {
        while (!this->entries.empty()) {
            Entry *entry = &this->entries.front();
            {
                std::scoped_lock lk(this->manager->entry_owner_lock);
                AMS_ABORT_UNLESS(entry->owner == this);
                entry->owner = nullptr;
            }
            entry->object.Reset();
            this->entries.pop_front();
            this->manager->entry_manager.FreeEntry(entry);
        }
    }

    void ServerDomainManager::Domain::DestroySelf() {
        ServerDomainManager *manager = this->manager;
        this->~Domain();
        manager->FreeDomain(this);
    }

    Result ServerDomainManager::Domain::ReserveIds(DomainObjectId *out_ids, size_t count) {
        for (size_t i = 0; i < count; i++) {
            Entry *entry = this->manager->entry_manager.AllocateEntry();
            R_UNLESS(entry != nullptr, sf::cmif::ResultOutOfDomainEntries());
            AMS_ABORT_UNLESS(entry->owner == nullptr);
            out_ids[i] = this->manager->entry_manager.GetId(entry);
        }
        return ResultSuccess();
    }

    void ServerDomainManager::Domain::ReserveSpecificIds(const DomainObjectId *ids, size_t count) {
        this->manager->entry_manager.AllocateSpecificEntries(ids, count);
    }

    void ServerDomainManager::Domain::UnreserveIds(const DomainObjectId *ids, size_t count) {
        for (size_t i = 0; i < count; i++) {
            Entry *entry = this->manager->entry_manager.GetEntry(ids[i]);
            AMS_ABORT_UNLESS(entry != nullptr);
            AMS_ABORT_UNLESS(entry->owner == nullptr);
            this->manager->entry_manager.FreeEntry(entry);
        }
    }

    void ServerDomainManager::Domain::RegisterObject(DomainObjectId id, ServiceObjectHolder &&obj) {
        Entry *entry = this->manager->entry_manager.GetEntry(id);
        AMS_ABORT_UNLESS(entry != nullptr);
        {
            std::scoped_lock lk(this->manager->entry_owner_lock);
            AMS_ABORT_UNLESS(entry->owner == nullptr);
            entry->owner = this;
            this->entries.push_back(*entry);
        }
        entry->object = std::move(obj);
    }

    ServiceObjectHolder ServerDomainManager::Domain::UnregisterObject(DomainObjectId id) {
        ServiceObjectHolder obj;
        Entry *entry = this->manager->entry_manager.GetEntry(id);
        if (entry == nullptr) {
            return ServiceObjectHolder();
        }
        {
            std::scoped_lock lk(this->manager->entry_owner_lock);
            if (entry->owner != this) {
                return ServiceObjectHolder();
            }
            entry->owner = nullptr;
            obj = std::move(entry->object);
            this->entries.erase(this->entries.iterator_to(*entry));
        }
        this->manager->entry_manager.FreeEntry(entry);
        return obj;
    }

    ServiceObjectHolder ServerDomainManager::Domain::GetObject(DomainObjectId id) {
        Entry *entry = this->manager->entry_manager.GetEntry(id);
        if (entry == nullptr) {
            return ServiceObjectHolder();
        }

        {
            std::scoped_lock lk(this->manager->entry_owner_lock);
            if (entry->owner != this) {
                return ServiceObjectHolder();
            }
        }
        return entry->object.Clone();
    }

    ServerDomainManager::EntryManager::EntryManager(DomainEntryStorage *entry_storage, size_t entry_count) : lock(false) {
        this->entries = reinterpret_cast<Entry *>(entry_storage);
        this->num_entries = entry_count;
        for (size_t i = 0; i < this->num_entries; i++) {
            Entry *entry = new (this->entries + i) Entry();
            this->free_list.push_back(*entry);
        }
    }

    ServerDomainManager::EntryManager::~EntryManager() {
        for (size_t i = 0; i < this->num_entries; i++) {
            this->entries[i].~Entry();
        }
    }

    ServerDomainManager::Entry *ServerDomainManager::EntryManager::AllocateEntry() {
        std::scoped_lock lk(this->lock);

        if (this->free_list.empty()) {
            return nullptr;
        }

        Entry *e = &this->free_list.front();
        this->free_list.pop_front();
        return e;
    }

    void ServerDomainManager::EntryManager::FreeEntry(Entry *entry) {
        std::scoped_lock lk(this->lock);
        AMS_ABORT_UNLESS(entry->owner == nullptr);
        AMS_ABORT_UNLESS(!entry->object);
        this->free_list.push_front(*entry);
    }

    void ServerDomainManager::EntryManager::AllocateSpecificEntries(const DomainObjectId *ids, size_t count) {
        std::scoped_lock lk(this->lock);

        /* Allocate new IDs. */
        for (size_t i = 0; i < count; i++) {
            const auto id = ids[i];
            Entry *entry = this->GetEntry(id);
            if (id != InvalidDomainObjectId) {
                AMS_ABORT_UNLESS(entry != nullptr);
                AMS_ABORT_UNLESS(entry->owner == nullptr);
                this->free_list.erase(this->free_list.iterator_to(*entry));
            }
        }
    }

}
