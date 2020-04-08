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
#include "../sf_common.hpp"
#include "sf_cmif_domain_api.hpp"
#include "sf_cmif_domain_service_object.hpp"

namespace ams::sf::cmif {

    class ServerDomainManager {
        NON_COPYABLE(ServerDomainManager);
        NON_MOVEABLE(ServerDomainManager);
        private:
            class Domain;

            struct Entry {
                NON_COPYABLE(Entry);
                NON_MOVEABLE(Entry);

                util::IntrusiveListNode free_list_node;
                util::IntrusiveListNode domain_list_node;
                Domain *owner;
                ServiceObjectHolder object;

                explicit Entry() : owner(nullptr) { /* ... */ }
            };

            class Domain final : public DomainServiceObject {
                NON_COPYABLE(Domain);
                NON_MOVEABLE(Domain);
                private:
                    using EntryList = typename util::IntrusiveListMemberTraits<&Entry::domain_list_node>::ListType;
                private:
                    ServerDomainManager *manager;
                    EntryList entries;
                public:
                    explicit Domain(ServerDomainManager *m) : manager(m) { /* ... */ }
                    ~Domain();

                    void DestroySelf();

                    virtual ServerDomainBase *GetServerDomain() override final {
                        return static_cast<ServerDomainBase *>(this);
                    }

                    virtual Result ReserveIds(DomainObjectId *out_ids, size_t count) override final;
                    virtual void   ReserveSpecificIds(const DomainObjectId *ids, size_t count) override final;
                    virtual void   UnreserveIds(const DomainObjectId *ids, size_t count) override final;
                    virtual void   RegisterObject(DomainObjectId id, ServiceObjectHolder &&obj) override final;

                    virtual ServiceObjectHolder UnregisterObject(DomainObjectId id) override final;
                    virtual ServiceObjectHolder GetObject(DomainObjectId id) override final;
            };
        public:
            using DomainEntryStorage  = TYPED_STORAGE(Entry);
            using DomainStorage = TYPED_STORAGE(Domain);
        private:
            class EntryManager {
                private:
                    using EntryList = typename util::IntrusiveListMemberTraits<&Entry::free_list_node>::ListType;
                private:
                    os::Mutex lock;
                    EntryList free_list;
                    Entry *entries;
                    size_t num_entries;
                public:
                    EntryManager(DomainEntryStorage *entry_storage, size_t entry_count);
                    ~EntryManager();
                    Entry *AllocateEntry();
                    void   FreeEntry(Entry *);

                    void AllocateSpecificEntries(const DomainObjectId *ids, size_t count);

                    inline DomainObjectId GetId(Entry *e) {
                        const size_t index = e - this->entries;
                        AMS_ABORT_UNLESS(index < this->num_entries);
                        return DomainObjectId{ u32(index + 1) };
                    }

                    inline Entry *GetEntry(DomainObjectId id) {
                        if (id == InvalidDomainObjectId) {
                            return nullptr;
                        }
                        const size_t index = id.value - 1;
                        if (!(index < this->num_entries)) {
                            return nullptr;
                        }
                        return this->entries + index;
                    }
            };
        private:
            os::Mutex entry_owner_lock;
            EntryManager entry_manager;
        private:
            virtual void *AllocateDomain()   = 0;
            virtual void  FreeDomain(void *) = 0;
        protected:
            ServerDomainManager(DomainEntryStorage *entry_storage, size_t entry_count) : entry_owner_lock(false), entry_manager(entry_storage, entry_count) { /* ... */ }

            inline DomainServiceObject *AllocateDomainServiceObject() {
                void *storage = this->AllocateDomain();
                if (storage == nullptr) {
                    return nullptr;
                }
                return new (storage) Domain(this);
            }
        public:
            static void DestroyDomainServiceObject(DomainServiceObject *obj) {
                static_cast<Domain *>(obj)->DestroySelf();
            }
    };


}
