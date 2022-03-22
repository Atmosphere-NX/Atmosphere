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
#include <stratosphere.hpp>

namespace ams::sf::cmif {

    ServerDomainManager::Domain::~Domain() {
        while (!m_entries.empty()) {
            Entry *entry = std::addressof(m_entries.front());
            {
                std::scoped_lock lk(m_manager->m_entry_owner_lock);
                AMS_ABORT_UNLESS(entry->owner == this);
                entry->owner = nullptr;
            }
            entry->object.Reset();
            m_entries.pop_front();
            m_manager->m_entry_manager.FreeEntry(entry);
        }
    }

    void ServerDomainManager::Domain::DisposeImpl() {
        ServerDomainManager *manager = m_manager;
        std::destroy_at(this);
        manager->FreeDomain(this);
    }

    Result ServerDomainManager::Domain::ReserveIds(DomainObjectId *out_ids, size_t count) {
        for (size_t i = 0; i < count; i++) {
            Entry *entry = m_manager->m_entry_manager.AllocateEntry();
            R_UNLESS(entry != nullptr, sf::cmif::ResultOutOfDomainEntries());
            AMS_ABORT_UNLESS(entry->owner == nullptr);
            out_ids[i] = m_manager->m_entry_manager.GetId(entry);
        }
        return ResultSuccess();
    }

    void ServerDomainManager::Domain::ReserveSpecificIds(const DomainObjectId *ids, size_t count) {
        m_manager->m_entry_manager.AllocateSpecificEntries(ids, count);
    }

    void ServerDomainManager::Domain::UnreserveIds(const DomainObjectId *ids, size_t count) {
        for (size_t i = 0; i < count; i++) {
            Entry *entry = m_manager->m_entry_manager.GetEntry(ids[i]);
            AMS_ABORT_UNLESS(entry != nullptr);
            AMS_ABORT_UNLESS(entry->owner == nullptr);
            m_manager->m_entry_manager.FreeEntry(entry);
        }
    }

    void ServerDomainManager::Domain::RegisterObject(DomainObjectId id, ServiceObjectHolder &&obj) {
        Entry *entry = m_manager->m_entry_manager.GetEntry(id);
        AMS_ABORT_UNLESS(entry != nullptr);
        {
            std::scoped_lock lk(m_manager->m_entry_owner_lock);
            AMS_ABORT_UNLESS(entry->owner == nullptr);
            entry->owner = this;
            m_entries.push_back(*entry);
        }
        entry->object = std::move(obj);
    }

    ServiceObjectHolder ServerDomainManager::Domain::UnregisterObject(DomainObjectId id) {
        ServiceObjectHolder obj;
        Entry *entry = m_manager->m_entry_manager.GetEntry(id);
        if (entry == nullptr) {
            return ServiceObjectHolder();
        }
        {
            std::scoped_lock lk(m_manager->m_entry_owner_lock);
            if (entry->owner != this) {
                return ServiceObjectHolder();
            }
            entry->owner = nullptr;
            obj = std::move(entry->object);
            m_entries.erase(m_entries.iterator_to(*entry));
        }
        m_manager->m_entry_manager.FreeEntry(entry);
        return obj;
    }

    ServiceObjectHolder ServerDomainManager::Domain::GetObject(DomainObjectId id) {
        Entry *entry = m_manager->m_entry_manager.GetEntry(id);
        if (entry == nullptr) {
            return ServiceObjectHolder();
        }

        {
            std::scoped_lock lk(m_manager->m_entry_owner_lock);
            if (entry->owner != this) {
                return ServiceObjectHolder();
            }
        }
        return entry->object.Clone();
    }

    ServerDomainManager::EntryManager::EntryManager(DomainEntryStorage *entry_storage, size_t entry_count) : m_lock() {
        m_entries = reinterpret_cast<Entry *>(entry_storage);
        m_num_entries = entry_count;
        for (size_t i = 0; i < m_num_entries; i++) {
            m_free_list.push_back(*std::construct_at(m_entries + i));
        }
    }

    ServerDomainManager::EntryManager::~EntryManager() {
        for (size_t i = 0; i < m_num_entries; i++) {
            std::destroy_at(m_entries + i);
        }
    }

    ServerDomainManager::Entry *ServerDomainManager::EntryManager::AllocateEntry() {
        std::scoped_lock lk(m_lock);

        if (m_free_list.empty()) {
            return nullptr;
        }

        Entry *e = std::addressof(m_free_list.front());
        m_free_list.pop_front();
        return e;
    }

    void ServerDomainManager::EntryManager::FreeEntry(Entry *entry) {
        std::scoped_lock lk(m_lock);
        AMS_ABORT_UNLESS(entry->owner == nullptr);
        AMS_ABORT_UNLESS(!entry->object);
        m_free_list.push_front(*entry);
    }

    void ServerDomainManager::EntryManager::AllocateSpecificEntries(const DomainObjectId *ids, size_t count) {
        std::scoped_lock lk(m_lock);

        /* Allocate new IDs. */
        for (size_t i = 0; i < count; i++) {
            const auto id = ids[i];
            Entry *entry = this->GetEntry(id);
            if (id != InvalidDomainObjectId) {
                AMS_ABORT_UNLESS(entry != nullptr);
                AMS_ABORT_UNLESS(entry->owner == nullptr);
                m_free_list.erase(m_free_list.iterator_to(*entry));
            }
        }
    }

}
