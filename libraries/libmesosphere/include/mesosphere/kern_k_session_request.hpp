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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_writable_event.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_process.hpp>
#include <mesosphere/kern_k_memory_block.hpp>

namespace ams::kern {

    class KSessionRequest final : public KSlabAllocated<KSessionRequest>, public KAutoObject, public util::IntrusiveListBaseNode<KSessionRequest> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KSessionRequest, KAutoObject);
        public:
            class SessionMappings {
                private:
                    static constexpr size_t NumStaticMappings = 8;

                    class Mapping {
                        private:
                            KProcessAddress client_address;
                            KProcessAddress server_address;
                            size_t size;
                            KMemoryState state;
                        public:
                            constexpr void Set(KProcessAddress c, KProcessAddress s, size_t sz, KMemoryState st) {
                                this->client_address = c;
                                this->server_address = s;
                                this->size           = sz;
                                this->state          = st;
                            }

                            constexpr ALWAYS_INLINE KProcessAddress GetClientAddress() const { return this->client_address; }
                            constexpr ALWAYS_INLINE KProcessAddress GetServerAddress() const { return this->server_address; }
                            constexpr ALWAYS_INLINE size_t GetSize() const { return this->size; }
                            constexpr ALWAYS_INLINE KMemoryState GetMemoryState() const { return this->state; }
                    };
                private:
                    Mapping static_mappings[NumStaticMappings];
                    Mapping *mappings;
                    u8 num_send;
                    u8 num_recv;
                    u8 num_exch;
                public:
                    constexpr explicit SessionMappings() : static_mappings(), mappings(), num_send(), num_recv(), num_exch() { /* ... */ }

                    void Initialize() { /* ... */ }
                    void Finalize();

                    constexpr ALWAYS_INLINE size_t GetSendCount() const { return this->num_send; }
                    constexpr ALWAYS_INLINE size_t GetReceiveCount() const { return this->num_recv; }
                    constexpr ALWAYS_INLINE size_t GetExchangeCount() const { return this->num_exch; }

                    Result PushSend(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state);
                    Result PushReceive(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state);
                    Result PushExchange(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state);

                    constexpr ALWAYS_INLINE KProcessAddress GetSendClientAddress(size_t i) const { return GetSendMapping(i).GetClientAddress(); }
                    constexpr ALWAYS_INLINE KProcessAddress GetSendServerAddress(size_t i) const { return GetSendMapping(i).GetServerAddress(); }
                    constexpr ALWAYS_INLINE size_t          GetSendSize(size_t i)          const { return GetSendMapping(i).GetSize(); }
                    constexpr ALWAYS_INLINE KMemoryState    GetSendMemoryState(size_t i)   const { return GetSendMapping(i).GetMemoryState(); }

                    constexpr ALWAYS_INLINE KProcessAddress GetReceiveClientAddress(size_t i) const { return GetReceiveMapping(i).GetClientAddress(); }
                    constexpr ALWAYS_INLINE KProcessAddress GetReceiveServerAddress(size_t i) const { return GetReceiveMapping(i).GetServerAddress(); }
                    constexpr ALWAYS_INLINE size_t          GetReceiveSize(size_t i)          const { return GetReceiveMapping(i).GetSize(); }
                    constexpr ALWAYS_INLINE KMemoryState    GetReceiveMemoryState(size_t i)   const { return GetReceiveMapping(i).GetMemoryState(); }

                    constexpr ALWAYS_INLINE KProcessAddress GetExchangeClientAddress(size_t i) const { return GetExchangeMapping(i).GetClientAddress(); }
                    constexpr ALWAYS_INLINE KProcessAddress GetExchangeServerAddress(size_t i) const { return GetExchangeMapping(i).GetServerAddress(); }
                    constexpr ALWAYS_INLINE size_t          GetExchangeSize(size_t i)          const { return GetExchangeMapping(i).GetSize(); }
                    constexpr ALWAYS_INLINE KMemoryState    GetExchangeMemoryState(size_t i)   const { return GetExchangeMapping(i).GetMemoryState(); }
                private:
                    Result PushMap(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state, size_t index);

                    constexpr ALWAYS_INLINE const Mapping &GetSendMapping(size_t i) const {
                        MESOSPHERE_ASSERT(i < this->num_send);

                        const size_t index = i;
                        if (index < NumStaticMappings) {
                            return this->static_mappings[index];
                        } else {
                            return this->mappings[index - NumStaticMappings];
                        }
                    }

                    constexpr ALWAYS_INLINE const Mapping &GetReceiveMapping(size_t i) const {
                        MESOSPHERE_ASSERT(i < this->num_recv);

                        const size_t index = this->num_send + i;
                        if (index < NumStaticMappings) {
                            return this->static_mappings[index];
                        } else {
                            return this->mappings[index - NumStaticMappings];
                        }
                    }

                    constexpr ALWAYS_INLINE const Mapping &GetExchangeMapping(size_t i) const {
                        MESOSPHERE_ASSERT(i < this->num_exch);

                        const size_t index = this->num_send + this->num_recv + i;
                        if (index < NumStaticMappings) {
                            return this->static_mappings[index];
                        } else {
                            return this->mappings[index - NumStaticMappings];
                        }
                    }


            };
        private:
            SessionMappings mappings;
            KThread *thread;
            KProcess *server;
            KWritableEvent *event;
            uintptr_t address;
            size_t size;
        public:
            constexpr KSessionRequest() : mappings(), thread(), server(), event(), address(), size() { /* ... */ }
            virtual ~KSessionRequest() { /* ... */ }

            static KSessionRequest *Create() {
                KSessionRequest *req = KSessionRequest::Allocate();
                if (req != nullptr) {
                    KAutoObject::Create(req);
                }
                return req;
            }

            virtual void Destroy() override {
                this->Finalize();
                KSessionRequest::Free(this);
            }

            void Initialize(KWritableEvent *event, uintptr_t address, size_t size) {
                this->mappings.Initialize();

                this->thread  = std::addressof(GetCurrentThread());
                this->event   = event;
                this->address = address;
                this->size    = size;

                this->thread->Open();
                if (this->event != nullptr) {
                    this->event->Open();
                }
            }

            virtual void Finalize() override {
                this->mappings.Finalize();

                if (this->thread) {
                    this->thread->Close();
                }
                if (this->event) {
                    this->event->Close();
                }
                if (this->server) {
                    this->server->Close();
                }
            }

            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            constexpr ALWAYS_INLINE KThread *GetThread() const { return this->thread; }
            constexpr ALWAYS_INLINE KWritableEvent *GetEvent() const { return this->event; }
            constexpr ALWAYS_INLINE uintptr_t GetAddress() const { return this->address; }
            constexpr ALWAYS_INLINE size_t GetSize() const { return this->size; }
            constexpr ALWAYS_INLINE KProcess *GetServerProcess() const { return this->server; }

            void ALWAYS_INLINE SetServerProcess(KProcess *process) {
                this->server = process;
                this->server->Open();
            }

            constexpr ALWAYS_INLINE void ClearThread() { this->thread = nullptr; }
            constexpr ALWAYS_INLINE void ClearEvent()  { this->event  = nullptr; }

            constexpr ALWAYS_INLINE size_t GetSendCount() const { return this->mappings.GetSendCount(); }
            constexpr ALWAYS_INLINE size_t GetReceiveCount() const { return this->mappings.GetReceiveCount(); }
            constexpr ALWAYS_INLINE size_t GetExchangeCount() const { return this->mappings.GetExchangeCount(); }

            ALWAYS_INLINE Result PushSend(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
                return this->mappings.PushSend(client, server, size, state);
            }

            ALWAYS_INLINE Result PushReceive(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
                return this->mappings.PushReceive(client, server, size, state);
            }

            ALWAYS_INLINE Result PushExchange(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
                return this->mappings.PushExchange(client, server, size, state);
            }

            constexpr ALWAYS_INLINE KProcessAddress GetSendClientAddress(size_t i) const { return this->mappings.GetSendClientAddress(i); }
            constexpr ALWAYS_INLINE KProcessAddress GetSendServerAddress(size_t i) const { return this->mappings.GetSendServerAddress(i); }
            constexpr ALWAYS_INLINE size_t          GetSendSize(size_t i)          const { return this->mappings.GetSendSize(i);          }
            constexpr ALWAYS_INLINE KMemoryState    GetSendMemoryState(size_t i)   const { return this->mappings.GetSendMemoryState(i);   }

            constexpr ALWAYS_INLINE KProcessAddress GetReceiveClientAddress(size_t i) const { return this->mappings.GetReceiveClientAddress(i); }
            constexpr ALWAYS_INLINE KProcessAddress GetReceiveServerAddress(size_t i) const { return this->mappings.GetReceiveServerAddress(i); }
            constexpr ALWAYS_INLINE size_t          GetReceiveSize(size_t i)          const { return this->mappings.GetReceiveSize(i);          }
            constexpr ALWAYS_INLINE KMemoryState    GetReceiveMemoryState(size_t i)   const { return this->mappings.GetReceiveMemoryState(i);   }

            constexpr ALWAYS_INLINE KProcessAddress GetExchangeClientAddress(size_t i) const { return this->mappings.GetExchangeClientAddress(i);  }
            constexpr ALWAYS_INLINE KProcessAddress GetExchangeServerAddress(size_t i) const { return this->mappings.GetExchangeServerAddress(i);  }
            constexpr ALWAYS_INLINE size_t          GetExchangeSize(size_t i)          const { return this->mappings.GetExchangeSize(i);           }
            constexpr ALWAYS_INLINE KMemoryState    GetExchangeMemoryState(size_t i)   const { return this->mappings.GetExchangeMemoryState(i);    }
    };

}
