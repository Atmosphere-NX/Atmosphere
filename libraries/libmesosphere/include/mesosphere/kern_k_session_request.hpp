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

                            constexpr KProcessAddress GetClientAddress() const { return this->client_address; }
                            constexpr KProcessAddress GetServerAddress() const { return this->server_address; }
                            constexpr size_t GetSize() const { return this->size; }
                            constexpr KMemoryState GetMemoryState() const { return this->state; }
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

                    size_t GetSendCount() const { return this->num_send; }
                    size_t GetReceiveCount() const { return this->num_recv; }
                    size_t GetExchangeCount() const { return this->num_exch; }

                    /* TODO: More functionality. */
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

            static void PostDestroy(uintptr_t arg) { /* ... */ }

            KThread *GetThread() const { return this->thread; }
            KWritableEvent *GetEvent() const { return this->event; }
            uintptr_t GetAddress() const { return this->address; }
            size_t GetSize() const { return this->size; }
            KProcess *GetServerProcess() const { return this->server; }

            void ClearThread() { this->thread = nullptr; }
            void ClearEvent()  { this->event  = nullptr; }

            size_t GetSendCount() const { return this->mappings.GetSendCount(); }
            size_t GetReceiveCount() const { return this->mappings.GetReceiveCount(); }
            size_t GetExchangeCount() const { return this->mappings.GetExchangeCount(); }

            /* TODO: More functionality. */
    };

}
