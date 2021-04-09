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
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_service_object.hpp>
#include <stratosphere/tipc/tipc_object_manager.hpp>

namespace ams::tipc {

    template<size_t NumSessions, typename Interface, typename Impl, template<typename, size_t> typename _Allocator>
    struct PortMeta {
        static constexpr inline size_t MaxSessions = NumSessions;

        using ServiceObject = tipc::ServiceObject<Interface, Impl>;

        using Allocator     = _Allocator<ServiceObject, NumSessions>;
    };

    struct DummyDeferralManager{
        struct Key{};
    };

    class PortManagerInterface {
        public:
            virtual Result ProcessRequest(WaitableObject &object) = 0;
    };

    template<typename DeferralManagerType, size_t ThreadStackSize, typename... PortInfos>
    class ServerManagerImpl {
        private:
            static_assert(util::IsAligned(ThreadStackSize, os::ThreadStackAlignment));

            static constexpr inline bool IsDeferralSupported = !std::same_as<DeferralManagerType, DummyDeferralManager>;
            using ResumeKey = typename DeferralManagerType::Key;

            static ALWAYS_INLINE uintptr_t ConvertKeyToMessage(ResumeKey key) {
                static_assert(sizeof(key) <= sizeof(uintptr_t));
                static_assert(std::is_trivial<ResumeKey>::value);

                /* TODO: std::bit_cast */
                uintptr_t converted = 0;
                std::memcpy(std::addressof(converted), std::addressof(key), sizeof(key));
                return converted;
            }

            static ALWAYS_INLINE ResumeKey ConvertMessageToKey(uintptr_t message) {
                static_assert(sizeof(ResumeKey) <= sizeof(uintptr_t));
                static_assert(std::is_trivial<ResumeKey>::value);

                /* TODO: std::bit_cast */
                ResumeKey converted = {};
                std::memcpy(std::addressof(converted), std::addressof(message), sizeof(converted));
                return converted;
            }

            static constexpr inline size_t NumPorts    = sizeof...(PortInfos);
            static constexpr inline size_t MaxSessions = (PortInfos::MaxSessions + ...);

            /* Verify that it's possible to service this many sessions, with our port manager count. */
            static_assert(MaxSessions <= NumPorts * svc::ArgumentHandleCountMax);

            template<size_t Ix> requires (Ix < NumPorts)
            static constexpr inline size_t SessionsPerPortManager = (Ix == NumPorts - 1) ? ((MaxSessions / NumPorts) + MaxSessions % NumPorts)
                                                                                         : ((MaxSessions / NumPorts));

            template<size_t Ix> requires (Ix < NumPorts)
            using PortInfo = typename std::tuple_element<Ix, std::tuple<PortInfos...>>::type;
        public:
            class PortManagerBase : public PortManagerInterface {
                public:
                    enum MessageType {
                        MessageType_AddSession    = 0,
                        MessageType_TriggerResume = 1,
                    };
                protected:
                    s32 m_id;
                    std::atomic<s32> m_num_sessions;
                    s32 m_port_number;
                    os::WaitableManagerType m_waitable_manager;
                    DeferralManagerType m_deferral_manager;
                    os::MessageQueueType m_message_queue;
                    os::WaitableHolderType m_message_queue_holder;
                    uintptr_t m_message_queue_storage[MaxSessions];
                    ObjectManagerBase *m_object_manager;
                public:
                    PortManagerBase() : m_id(), m_num_sessions(), m_port_number(), m_waitable_manager(), m_deferral_manager(), m_message_queue(), m_message_queue_holder(), m_message_queue_storage(), m_object_manager() {
                        /* Setup our message queue. */
                        os::InitializeMessageQueue(std::addressof(m_message_queue), m_message_queue_storage, util::size(m_message_queue_storage));
                        os::InitializeWaitableHolder(std::addressof(m_message_queue_holder), std::addressof(m_message_queue), os::MessageQueueWaitType::ForNotEmpty);
                    }

                    void InitializeBase(s32 id, ObjectManagerBase *manager) {
                        /* Set our id. */
                        m_id = id;

                        /* Reset our session count. */
                        m_num_sessions = 0;

                        /* Initialize our waitable manager. */
                        os::InitializeWaitableManager(std::addressof(m_waitable_manager));
                        os::LinkWaitableHolder(std::addressof(m_waitable_manager), std::addressof(m_message_queue_holder));

                        /* Initialize our object manager. */
                        m_object_manager = manager;
                    }

                    void RegisterPort(s32 index, svc::Handle port_handle) {
                        /* Set our port number. */
                        this->m_port_number = index;

                        /* Create a waitable object for the port. */
                        tipc::WaitableObject object;

                        /* Setup the object. */
                        object.InitializeAsPort(port_handle);

                        /* Register the object. */
                        m_object_manager->AddObject(object);
                    }

                    virtual Result ProcessRequest(WaitableObject &object) override {
                        /* Process the request, this must succeed because we succeeded when deferring earlier. */
                        R_ABORT_UNLESS(m_object_manager->ProcessRequest(object));

                        /* NOTE: We support nested deferral, where Nintendo does not. */
                        if constexpr (IsDeferralSupported) {
                            R_UNLESS(!PortManagerBase::IsRequestDeferred(), tipc::ResultRequestDeferred());
                        }

                        /* Reply to the request. */
                        return m_object_manager->Reply(object.GetHandle());
                    }

                    Result ReplyAndReceive(os::WaitableHolderType **out_holder, WaitableObject *out_object, svc::Handle reply_target) {
                        return m_object_manager->ReplyAndReceive(out_holder, out_object, reply_target, std::addressof(m_waitable_manager));
                    }

                    void StartRegisterRetry(ResumeKey key) {
                        /* Begin the retry. */
                        m_deferral_manager.StartRegisterRetry(key);
                    }

                    bool TestResume(ResumeKey key) {
                        /* Check to see if the key corresponds to some deferred message. */
                        return m_deferral_manager.TestResume(key);
                    }

                    void TriggerResume(ResumeKey key) {
                        /* Send the key as a message. */
                        os::SendMessageQueue(std::addressof(m_message_queue), static_cast<uintptr_t>(MessageType_TriggerResume));
                        os::SendMessageQueue(std::addressof(m_message_queue), ConvertKeyToMessage(key));
                    }
                private:
                    static bool IsRequestDeferred() {
                        if constexpr (IsDeferralSupported) {
                            /* Get the message buffer. */
                            const svc::ipc::MessageBuffer message_buffer(svc::ipc::GetMessageBuffer());

                            /* Parse the hipc headers. */
                            const svc::ipc::MessageBuffer::MessageHeader message_header(message_buffer);
                            const svc::ipc::MessageBuffer::SpecialHeader special_header(message_buffer, message_header);

                            /* Determine raw data index and extents. */
                            const auto raw_data_offset = message_buffer.GetRawDataIndex(message_header, special_header);
                            const auto raw_data_count  = message_header.GetRawCount();

                            /* Result is the last raw data word. */
                            const Result method_result = message_buffer.GetRaw<u32>(raw_data_offset + raw_data_count - 1);

                            /* Check that the result is the special deferral result. */
                            return tipc::ResultRequestDeferred::Includes(method_result);
                        } else {
                            /* If deferral isn't supported, requests are never deferred. */
                            return false;
                        }
                    }
            };

            template<typename PortInfo, size_t PortSessions>
            class PortManagerImpl final : public PortManagerBase {
                private:
                    tipc::ObjectManager<PortSessions> m_object_manager_impl;
                public:
                    PortManagerImpl() : PortManagerBase(), m_object_manager_impl() {
                        /* ...  */
                    }

                    void Initialize(s32 id) {
                        /* Initialize our base. */
                        this->InitializeBase(id, std::addressof(m_object_manager_impl));

                        /* Initialize our object manager. */
                        m_object_manager_impl->Initialize(std::addressof(this->m_waitable_manager));
                    }
            };

            template<size_t Ix>
            using PortManager = PortManagerImpl<PortInfo<Ix>, SessionsPerPortManager<Ix>>;

            using PortManagerTuple = decltype([]<size_t... Ix>(std::index_sequence<Ix...>) {
                return std::tuple<PortManager<Ix>...>{};
            }(std::make_index_sequence(NumPorts)));

            using PortAllocatorTuple = std::tuple<typename PortInfos::Allocator...>;
        private:
            os::SdkMutex m_mutex;
            os::TlsSlot m_tls_slot;
            PortManagerTuple m_port_managers;
            PortAllocatorTuple m_port_allocators;
            os::ThreadType m_port_threads[NumPorts - 1];
            alignas(os::ThreadStackAlignment) u8 m_port_stacks[ThreadStackSize * (NumPorts - 1)];
        private:
            template<size_t Ix>
            ALWAYS_INLINE auto &GetPortManager() {
                return std::get<Ix>(m_port_managers);
            }

            template<size_t Ix>
            ALWAYS_INLINE const auto &GetPortManager() const {
                return std::get<Ix>(m_port_managers);
            }

            template<size_t Ix>
            void LoopAutoForPort() {
                R_ABORT_UNLESS(this->LoopProcess(this->GetPortManager<Ix>()));
            }

            template<size_t Ix>
            static void LoopAutoForPortThreadFunction(void *_this) {
                static_cast<ServerManagerImpl *>(_this)->LoopAutoForPort<Ix>();
            }

            template<size_t Ix>
            void InitializePortThread(s32 priority) {
                /* Create the thread. */
                R_ABORT_UNLESS(os::CreateThread(m_port_threads + Ix, LoopAutoForPortThreadFunction, this, m_port_stacks + Ix, ThreadStackSize, priority));

                /* Start the thread. */
                os::StartThread(m_port_threads + Ix);
            }
        public:
            ServerManagerImpl() : m_mutex(), m_tls_slot(), m_port_managers(), m_port_allocators() { /* ... */ }

            os::TlsSlot GetTlsSlot() const { return m_tls_slot; }

            void Initialize() {
                /* Initialize our tls slot. */
                if constexpr (IsDeferralSupported) {
                    R_ABORT_UNLESS(os::SdkAllocateTlsSlot(std::addressof(m_tls_slot), nullptr));
                }

                /* Initialize our port managers. */
                [this]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                    (this->GetPortManager<Ix>().Initialize(static_cast<s32>(Ix)), ...);
                }(std::make_index_sequence(NumPorts));
            }

            template<size_t Ix>
            void RegisterPort(svc::Handle port_handle) {
                this->GetPortManager<Ix>().RegisterPort(static_cast<s32>(Ix), port_handle);
            }

            void LoopAuto() {
                /* If we have additional threads, create and start them. */
                if constexpr (NumPorts > 1) {
                    const auto thread_priority = os::GetThreadPriority(os::GetCurrentThread());

                    [thread_priority, this]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                        /* Create all threads. */
                        (this->InitializePortThread<Ix>(thread_priority), ...);
                    }(std::make_index_sequence(NumPorts - 1));
                }

                /* Process for the last port. */
                this->LoopAutoForPort<NumPorts - 1>();
            }
        private:
            Result LoopProcess(PortManagerBase &port_manager) {
                /* Set our tls slot's value to be the port manager we're processing for. */
                if constexpr (IsDeferralSupported) {
                    os::SetTlsValue(this->GetTlsSlot(), reinterpret_cast<uintptr_t>(std::addressof(port_manager)));
                }

                /* Clear the message buffer. */
                /* NOTE: Nintendo only clears the hipc header. */
                std::memset(svc::ipc::GetMessageBuffer(), 0, svc::ipc::MessageBufferSize);

                /* Process requests forever. */
                svc::Handle reply_target = svc::InvalidHandle;
                while (true) {
                    /* TODO */
                }
            }
    };

    template<typename DeferralManagerType, typename... PortInfos>
    using ServerManagerWithDeferral = ServerManagerImpl<DeferralManagerType, os::MemoryPageSize, PortInfos...>;

    template<typename DeferralManagerType, size_t ThreadStackSize, typename... PortInfos>
    using ServerManagerWithDeferralAndThreadStack = ServerManagerImpl<DeferralManagerType, ThreadStackSize, PortInfos...>;

    template<typename... PortInfos>
    using ServerManager = ServerManagerImpl<DummyDeferralManager, os::MemoryPageSize, PortInfos...>;

    template<size_t ThreadStackSize, typename... PortInfos>
    using ServerManagerWithThreadStack = ServerManagerImpl<DummyDeferralManager, ThreadStackSize, PortInfos...>;

}

