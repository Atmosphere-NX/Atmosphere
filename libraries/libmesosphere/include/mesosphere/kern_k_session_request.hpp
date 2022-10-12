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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_event.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_process.hpp>
#include <mesosphere/kern_k_memory_block.hpp>

namespace ams::kern {

    class KSessionRequest final : public KSlabAllocated<KSessionRequest, true>, public KAutoObject, public util::IntrusiveListBaseNode<KSessionRequest> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KSessionRequest, KAutoObject);
        public:
            class SessionMappings {
                private:
                    /* At most 15 buffers of each type (4-bit descriptor counts), for 45 total. */
                    static constexpr size_t NumMappings        = ((1ul << 4) - 1) * 3;
                    static constexpr size_t NumStaticMappings  = 8;
                    static constexpr size_t NumDynamicMappings = NumMappings - NumStaticMappings;

                    class Mapping {
                        private:
                            uintptr_t m_client_address;
                            uintptr_t m_server_address;
                            size_t m_size;
                            KMemoryState m_state;
                        public:
                            constexpr void Set(KProcessAddress c, KProcessAddress s, size_t sz, KMemoryState st) {
                                m_client_address = GetInteger(c);
                                m_server_address = GetInteger(s);
                                m_size           = sz;
                                m_state          = st;
                            }

                            constexpr ALWAYS_INLINE KProcessAddress GetClientAddress() const { return m_client_address; }
                            constexpr ALWAYS_INLINE KProcessAddress GetServerAddress() const { return m_server_address; }
                            constexpr ALWAYS_INLINE size_t GetSize() const { return m_size; }
                            constexpr ALWAYS_INLINE KMemoryState GetMemoryState() const { return m_state; }
                    };
                public:
                    class DynamicMappings : public KSlabAllocated<DynamicMappings, true> {
                        private:
                            Mapping m_mappings[NumDynamicMappings];
                        public:
                            constexpr explicit DynamicMappings() : m_mappings() { /* ... */ }

                            constexpr ALWAYS_INLINE       Mapping &Get(size_t idx)       { return m_mappings[idx]; }
                            constexpr ALWAYS_INLINE const Mapping &Get(size_t idx) const { return m_mappings[idx]; }
                    };
                    static_assert(sizeof(DynamicMappings) == sizeof(Mapping) * NumDynamicMappings);
                private:
                    Mapping m_static_mappings[NumStaticMappings];
                    DynamicMappings *m_dynamic_mappings;
                    u8 m_num_send;
                    u8 m_num_recv;
                    u8 m_num_exch;
                public:
                    constexpr explicit SessionMappings(util::ConstantInitializeTag) : m_static_mappings(), m_dynamic_mappings(), m_num_send(), m_num_recv(), m_num_exch() { /* ... */ }

                    explicit SessionMappings() : m_dynamic_mappings(nullptr), m_num_send(), m_num_recv(), m_num_exch() { /* ... */ }

                    void Initialize() { /* ... */ }
                    void Finalize();

                    constexpr ALWAYS_INLINE size_t GetSendCount() const { return m_num_send; }
                    constexpr ALWAYS_INLINE size_t GetReceiveCount() const { return m_num_recv; }
                    constexpr ALWAYS_INLINE size_t GetExchangeCount() const { return m_num_exch; }

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
                        MESOSPHERE_ASSERT(i < m_num_send);

                        const size_t index = i;
                        if (index < NumStaticMappings) {
                            return m_static_mappings[index];
                        } else {
                            return m_dynamic_mappings->Get(index - NumStaticMappings);
                        }
                    }

                    constexpr ALWAYS_INLINE const Mapping &GetReceiveMapping(size_t i) const {
                        MESOSPHERE_ASSERT(i < m_num_recv);

                        const size_t index = m_num_send + i;
                        if (index < NumStaticMappings) {
                            return m_static_mappings[index];
                        } else {
                            return m_dynamic_mappings->Get(index - NumStaticMappings);
                        }
                    }

                    constexpr ALWAYS_INLINE const Mapping &GetExchangeMapping(size_t i) const {
                        MESOSPHERE_ASSERT(i < m_num_exch);

                        const size_t index = m_num_send + m_num_recv + i;
                        if (index < NumStaticMappings) {
                            return m_static_mappings[index];
                        } else {
                            return m_dynamic_mappings->Get(index - NumStaticMappings);
                        }
                    }
            };
        private:
            SessionMappings m_mappings;
            KThread *m_thread;
            KProcess *m_server;
            KEvent *m_event;
            uintptr_t m_address;
            size_t m_size;
        public:
            constexpr explicit KSessionRequest(util::ConstantInitializeTag) : KAutoObject(util::ConstantInitialize), m_mappings(util::ConstantInitialize), m_thread(), m_server(), m_event(), m_address(), m_size() { /* ... */ }

            explicit KSessionRequest() : m_thread(nullptr), m_server(nullptr), m_event(nullptr) { /* ... */ }

            static KSessionRequest *Create() {
                KSessionRequest *req = KSessionRequest::Allocate();
                if (AMS_LIKELY(req != nullptr)) {
                    KAutoObject::Create<KSessionRequest>(req);
                }
                return req;
            }

            static KSessionRequest *CreateFromUnusedSlabMemory() {
                KSessionRequest *req = KSessionRequest::AllocateFromUnusedSlabMemory();
                if (AMS_LIKELY(req != nullptr)) {
                    KAutoObject::Create<KSessionRequest>(req);
                }
                return req;
            }

            virtual void Destroy() override {
                this->Finalize();
                KSessionRequest::Free(this);
            }

            void Initialize(KEvent *event, uintptr_t address, size_t size) {
                m_mappings.Initialize();

                m_thread  = std::addressof(GetCurrentThread());
                m_event   = event;
                m_address = address;
                m_size    = size;

                m_thread->Open();
                if (m_event != nullptr) {
                    m_event->Open();
                }
            }

            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            constexpr ALWAYS_INLINE KThread *GetThread() const { return m_thread; }
            constexpr ALWAYS_INLINE KEvent *GetEvent() const { return m_event; }
            constexpr ALWAYS_INLINE uintptr_t GetAddress() const { return m_address; }
            constexpr ALWAYS_INLINE size_t GetSize() const { return m_size; }
            constexpr ALWAYS_INLINE KProcess *GetServerProcess() const { return m_server; }

            void ALWAYS_INLINE SetServerProcess(KProcess *process) {
                m_server = process;
                m_server->Open();
            }

            constexpr ALWAYS_INLINE void ClearThread() { m_thread = nullptr; }
            constexpr ALWAYS_INLINE void ClearEvent()  { m_event  = nullptr; }

            constexpr ALWAYS_INLINE size_t GetSendCount() const { return m_mappings.GetSendCount(); }
            constexpr ALWAYS_INLINE size_t GetReceiveCount() const { return m_mappings.GetReceiveCount(); }
            constexpr ALWAYS_INLINE size_t GetExchangeCount() const { return m_mappings.GetExchangeCount(); }

            ALWAYS_INLINE Result PushSend(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
                R_RETURN(m_mappings.PushSend(client, server, size, state));
            }

            ALWAYS_INLINE Result PushReceive(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
                R_RETURN(m_mappings.PushReceive(client, server, size, state));
            }

            ALWAYS_INLINE Result PushExchange(KProcessAddress client, KProcessAddress server, size_t size, KMemoryState state) {
                R_RETURN(m_mappings.PushExchange(client, server, size, state));
            }

            constexpr ALWAYS_INLINE KProcessAddress GetSendClientAddress(size_t i) const { return m_mappings.GetSendClientAddress(i); }
            constexpr ALWAYS_INLINE KProcessAddress GetSendServerAddress(size_t i) const { return m_mappings.GetSendServerAddress(i); }
            constexpr ALWAYS_INLINE size_t          GetSendSize(size_t i)          const { return m_mappings.GetSendSize(i);          }
            constexpr ALWAYS_INLINE KMemoryState    GetSendMemoryState(size_t i)   const { return m_mappings.GetSendMemoryState(i);   }

            constexpr ALWAYS_INLINE KProcessAddress GetReceiveClientAddress(size_t i) const { return m_mappings.GetReceiveClientAddress(i); }
            constexpr ALWAYS_INLINE KProcessAddress GetReceiveServerAddress(size_t i) const { return m_mappings.GetReceiveServerAddress(i); }
            constexpr ALWAYS_INLINE size_t          GetReceiveSize(size_t i)          const { return m_mappings.GetReceiveSize(i);          }
            constexpr ALWAYS_INLINE KMemoryState    GetReceiveMemoryState(size_t i)   const { return m_mappings.GetReceiveMemoryState(i);   }

            constexpr ALWAYS_INLINE KProcessAddress GetExchangeClientAddress(size_t i) const { return m_mappings.GetExchangeClientAddress(i);  }
            constexpr ALWAYS_INLINE KProcessAddress GetExchangeServerAddress(size_t i) const { return m_mappings.GetExchangeServerAddress(i);  }
            constexpr ALWAYS_INLINE size_t          GetExchangeSize(size_t i)          const { return m_mappings.GetExchangeSize(i);           }
            constexpr ALWAYS_INLINE KMemoryState    GetExchangeMemoryState(size_t i)   const { return m_mappings.GetExchangeMemoryState(i);    }
        private:
            /* NOTE: This is public and virtual in Nintendo's kernel. */
            void Finalize() {
                m_mappings.Finalize();

                if (m_thread) {
                    m_thread->Close();
                }
                if (m_event) {
                    m_event->Close();
                }
                if (m_server) {
                    m_server->Close();
                }
            }
    };

}
