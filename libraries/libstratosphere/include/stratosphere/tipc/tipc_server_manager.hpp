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
                    enum MessageType : u8 {
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
                    ServerManagerImpl *m_server_manager;
                public:
                    PortManagerBase() : m_id(), m_num_sessions(), m_port_number(), m_waitable_manager(), m_deferral_manager(), m_message_queue(), m_message_queue_holder(), m_message_queue_storage(), m_object_manager(), m_server_manager() {
                        /* Setup our message queue. */
                        os::InitializeMessageQueue(std::addressof(m_message_queue), m_message_queue_storage, util::size(m_message_queue_storage));
                        os::InitializeWaitableHolder(std::addressof(m_message_queue_holder), std::addressof(m_message_queue), os::MessageQueueWaitType::ForNotEmpty);
                    }

                    constexpr s32 GetPortIndex() const {
                        return m_port_number;
                    }

                    s32 GetSessionCount() const {
                        return m_num_sessions;
                    }

                    ObjectManagerBase *GetObjectManager() const {
                        return m_object_manager;
                    }

                    void InitializeBase(s32 id, ServerManagerImpl *sm, ObjectManagerBase *manager) {
                        /* Set our id. */
                        m_id = id;

                        /* Set our server manager. */
                        m_server_manager = sm;

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

                    void AddSession(svc::Handle session_handle, tipc::ServiceObjectBase *service_object) {
                        /* Create a waitable object for the session. */
                        tipc::WaitableObject object;

                        /* Setup the object. */
                        object.InitializeAsSession(session_handle, true, service_object);

                        /* Register the object. */
                        m_object_manager->AddObject(object);
                    }

                    void ProcessMessages() {
                        /* While we have messages in our queue, receive and handle them. */
                        uintptr_t message_type, message_data;
                        while (os::TryReceiveMessageQueue(std::addressof(message_type), std::addressof(m_message_queue))) {
                            /* Receive the message's data. */
                            os::ReceiveMessageQueue(std::addressof(message_data), std::addressof(m_message_queue));

                            /* Handle the specific message. */
                            switch (static_cast<MessageType>(static_cast<typename std::underlying_type<MessageType>::type>(message_type))) {
                                case MessageType_AddSession:
                                    {
                                        /* Get the handle from where it's packed into the message type. */
                                        const svc::Handle session_handle = static_cast<svc::Handle>(message_type >> BITSIZEOF(u32));

                                        /* Allocate a service object for the port. */
                                        auto *service_object = m_server_manager->AllocateObject(static_cast<size_t>(message_data));

                                        /* Add the newly-created service object. */
                                        this->AddSession(session_handle, service_object);
                                    }
                                    break;
                                case MessageType_TriggerResume:
                                    if constexpr (IsDeferralSupported) {
                                        /* Acquire exclusive server manager access. */
                                        std::scoped_lock lk(m_server_manager->GetMutex());

                                        /* Perform the resume. */
                                        const auto resume_key = ConvertMessageToKey(message_data);
                                        m_deferral_manager.Resume(resume_key, this);
                                    }
                                    break;
                                AMS_UNREACHABLE_DEFAULT_CASE();
                            }
                        }
                    }

                    void CloseSession(WaitableObject &object) {
                        /* Get the object's handle. */
                        const auto handle = object.GetHandle();

                        /* Close the object with our manager. */
                        m_object_manager->CloseObject(handle);

                        /* Close the handle itself. */
                        R_ABORT_UNLESS(svc::CloseHandle(handle));

                        /* Decrement our session count. */
                        --m_num_sessions;
                    }

                    void CloseSessionIfNecessary(WaitableObject &object, bool necessary) {
                        if (necessary) {
                            /* Get the object's handle. */
                            const auto handle = object.GetHandle();

                            /* Close the object with our manager. */
                            m_object_manager->CloseObject(handle);

                            /* Close the handle itself. */
                            R_ABORT_UNLESS(svc::CloseHandle(handle));
                        }

                        /* Decrement our session count. */
                        --m_num_sessions;
                    }

                    Result StartRegisterRetry(ResumeKey key) {
                        if constexpr (IsDeferralSupported) {
                            /* Acquire exclusive server manager access. */
                            std::scoped_lock lk(m_server_manager->GetMutex());

                            /* Begin the retry. */
                            return m_deferral_manager.StartRegisterRetry(key);
                        } else {
                            return ResultSuccess();
                        }
                    }

                    void ProcessRegisterRetry(WaitableObject &object) {
                        if constexpr (IsDeferralSupported) {
                            /* Acquire exclusive server manager access. */
                            std::scoped_lock lk(m_server_manager->GetMutex());

                            /* Process the retry. */
                            m_deferral_manager.ProcessRegisterRetry(object);
                        }
                    }

                    bool TestResume(ResumeKey key) {
                        if constexpr (IsDeferralSupported) {
                            /* Acquire exclusive server manager access. */
                            std::scoped_lock lk(m_server_manager->GetMutex());

                            /* Check to see if the key corresponds to some deferred message. */
                            return m_deferral_manager.TestResume(key);
                        } else {
                            return false;
                        }
                    }

                    void TriggerResume(ResumeKey key) {
                        /* Acquire exclusive server manager access. */
                        std::scoped_lock lk(m_server_manager->GetMutex());

                        /* Send the key as a message. */
                        os::SendMessageQueue(std::addressof(m_message_queue), static_cast<uintptr_t>(MessageType_TriggerResume));
                        os::SendMessageQueue(std::addressof(m_message_queue), ConvertKeyToMessage(key));
                    }

                    void TriggerAddSession(svc::Handle session_handle, size_t port_index) {
                        /* Acquire exclusive server manager access. */
                        std::scoped_lock lk(m_server_manager->GetMutex());

                        /* Increment our session count. */
                        ++m_num_sessions;

                        /* Send information about the session as a message. */
                        os::SendMessageQueue(std::addressof(m_message_queue), static_cast<uintptr_t>(MessageType_AddSession) | (static_cast<u64>(session_handle) << BITSIZEOF(u32)));
                        os::SendMessageQueue(std::addressof(m_message_queue), static_cast<uintptr_t>(port_index));
                    }
                public:
                    static bool IsRequestDeferred() {
                        if constexpr (IsDeferralSupported) {
                            /* Get the message buffer. */
                            const svc::ipc::MessageBuffer message_buffer(svc::ipc::GetMessageBuffer());

                            /* Parse the hipc headers. */
                            const svc::ipc::MessageBuffer::MessageHeader message_header(message_buffer);
                            const svc::ipc::MessageBuffer::SpecialHeader special_header(message_buffer, message_header);

                            /* Determine raw data index. */
                            const auto raw_data_offset = message_buffer.GetRawDataIndex(message_header, special_header);

                            /* Result is the first raw data word. */
                            const Result method_result = message_buffer.GetRaw<u32>(raw_data_offset);

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

                    void Initialize(s32 id, ServerManagerImpl *sm) {
                        /* Initialize our base. */
                        this->InitializeBase(id, sm, std::addressof(m_object_manager_impl));

                        /* Initialize our object manager. */
                        m_object_manager_impl.Initialize(std::addressof(this->m_waitable_manager));
                    }
            };

            template<size_t Ix>
            using PortManager = PortManagerImpl<PortInfo<Ix>, SessionsPerPortManager<Ix>>;

            using PortManagerTuple = decltype([]<size_t... Ix>(std::index_sequence<Ix...>) {
                return std::tuple<PortManager<Ix>...>{};
            }(std::make_index_sequence<NumPorts>()));

            using PortAllocatorTuple = std::tuple<typename PortInfos::Allocator...>;
        private:
            os::Mutex m_mutex;
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
                R_ABORT_UNLESS(os::CreateThread(m_port_threads + Ix, &LoopAutoForPortThreadFunction<Ix>, this, m_port_stacks + Ix, ThreadStackSize, priority));

                /* Start the thread. */
                os::StartThread(m_port_threads + Ix);
            }
        public:
            ServerManagerImpl() : m_mutex(true), m_tls_slot(), m_port_managers(), m_port_allocators() { /* ... */ }

            os::TlsSlot GetTlsSlot() const { return m_tls_slot; }

            os::Mutex &GetMutex() { return m_mutex; }

            void Initialize() {
                /* Initialize our tls slot. */
                if constexpr (IsDeferralSupported) {
                    R_ABORT_UNLESS(os::SdkAllocateTlsSlot(std::addressof(m_tls_slot), nullptr));
                }

                /* Initialize our port managers. */
                [this]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                    (this->GetPortManager<Ix>().Initialize(static_cast<s32>(Ix), this), ...);
                }(std::make_index_sequence<NumPorts>());
            }

            template<size_t Ix>
            void RegisterPort(svc::Handle port_handle) {
                this->GetPortManager<Ix>().RegisterPort(static_cast<s32>(Ix), port_handle);
            }

            template<size_t Ix>
            void RegisterPort(sm::ServiceName service_name, size_t max_sessions) {
                /* Register service. */
                svc::Handle port_handle = svc::InvalidHandle;
                R_ABORT_UNLESS(sm::RegisterService(std::addressof(port_handle), service_name, max_sessions, false));

                /* Register the port handle. */
                this->RegisterPort<Ix>(port_handle);
            }

            void LoopAuto() {
                /* If we have additional threads, create and start them. */
                if constexpr (NumPorts > 1) {
                    const auto thread_priority = os::GetThreadPriority(os::GetCurrentThread());

                    [thread_priority, this]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                        /* Create all threads. */
                        (this->InitializePortThread<Ix>(thread_priority), ...);
                    }(std::make_index_sequence<NumPorts - 1>());
                }

                /* Process for the last port. */
                this->LoopAutoForPort<NumPorts - 1>();
            }

            tipc::ServiceObjectBase *AllocateObject(size_t port_index) {
                /* Check that the port index is valid. */
                AMS_ABORT_UNLESS(port_index < NumPorts);

                /* Try to allocate from each port, in turn. */
                tipc::ServiceObjectBase *allocated = nullptr;
                [this, port_index, &allocated]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                    (this->TryAllocateObject<Ix>(port_index, allocated), ...);
                }(std::make_index_sequence<NumPorts>());

                /* Return the allocated object. */
                AMS_ABORT_UNLESS(allocated != nullptr);
                return allocated;
            }

            void TriggerResume(ResumeKey resume_key) {
                /* Acquire exclusive access to ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Check/trigger resume on each of our ports. */
                [this, resume_key]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                    (this->TriggerResumeImpl<Ix>(resume_key), ...);
                }(std::make_index_sequence<NumPorts>());
            }

            Result AddSession(svc::Handle *out, tipc::ServiceObjectBase *object) {
                /* Acquire exclusive access to ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Create a handle for the session. */
                svc::Handle session_handle;
                R_TRY(svc::CreateSession(std::addressof(session_handle), out, false, 0));

                /* Select the best port manager. */
                PortManagerBase *best_manager = nullptr;
                s32 best_sessions             = -1;
                [this, &best_manager, &best_sessions]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                    (this->TrySelectBetterPort<Ix>(best_manager, best_sessions), ...);
                }(std::make_index_sequence<NumPorts>());

                /* Add the session to the least burdened manager. */
                best_manager->AddSession(session_handle, object);

                return ResultSuccess();
            }
        private:
            template<size_t Ix> requires (Ix < NumPorts)
            void TryAllocateObject(size_t port_index, tipc::ServiceObjectBase *&allocated) {
                /* Check that the port index matches. */
                if (port_index == Ix) {
                    /* Get the allocator. */
                    auto &allocator = std::get<Ix>(m_port_allocators);

                    /* Allocate the object. */
                    AMS_ABORT_UNLESS(allocated == nullptr);
                    allocated = allocator.Allocate();
                    AMS_ABORT_UNLESS(allocated != nullptr);

                    /* If we should, set the object's deleter. */
                    if constexpr (IsServiceObjectDeleter<typename std::tuple_element<Ix, PortAllocatorTuple>::type>) {
                        allocated->SetDeleter(std::addressof(allocator));
                    }
                }
            }

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
                    /* Reply to our pending request, and receive a new one. */
                    os::WaitableHolderType *signaled_holder = nullptr;
                    tipc::WaitableObject signaled_object{};
                    R_TRY_CATCH(port_manager.ReplyAndReceive(std::addressof(signaled_holder), std::addressof(signaled_object), reply_target)) {
                        R_CATCH(os::ResultSessionClosedForReceive, os::ResultReceiveListBroken) {
                            /* Close the object and continue. */
                            port_manager.CloseSession(signaled_object);

                            /* We have nothing to reply to. */
                            reply_target = svc::InvalidHandle;
                            continue;
                        }
                    } R_END_TRY_CATCH;

                    if (signaled_holder == nullptr) {
                        /* A session was signaled, accessible via signaled_object. */
                        switch (signaled_object.GetType()) {
                            case WaitableObject::ObjectType_Port:
                                {
                                    /* Try to accept a new session */
                                    svc::Handle session_handle;
                                    if (R_SUCCEEDED(svc::AcceptSession(std::addressof(session_handle), signaled_object.GetHandle()))) {
                                        this->TriggerAddSession(session_handle, static_cast<size_t>(port_manager.GetPortIndex()));
                                    }

                                    /* We have nothing to reply to. */
                                    reply_target = svc::InvalidHandle;
                                }
                                break;
                            case WaitableObject::ObjectType_Session:
                                {
                                    /* Process the request */
                                    const Result process_result = port_manager.GetObjectManager()->ProcessRequest(signaled_object);
                                    if (R_SUCCEEDED(process_result)) {
                                        if constexpr (IsDeferralSupported) {
                                            /* Check if the request is deferred. */
                                            if (PortManagerBase::IsRequestDeferred()) {
                                                /* Process the retry that we began. */
                                                port_manager.ProcessRegisterRetry(signaled_object);

                                                /* We have nothing to reply to. */
                                                reply_target = svc::InvalidHandle;
                                            } else {
                                                /* We're done processing, so we should reply. */
                                                reply_target = signaled_object.GetHandle();
                                            }
                                        } else {
                                            /* We're done processing, so we should reply. */
                                            reply_target = signaled_object.GetHandle();
                                        }
                                    } else {
                                        /* We failed to process, so note the session as closed (or close it). */
                                        port_manager.CloseSessionIfNecessary(signaled_object, !tipc::ResultSessionClosed::Includes(process_result));

                                        /* We have nothing to reply to. */
                                        reply_target = svc::InvalidHandle;
                                    }
                                }
                                break;
                            AMS_UNREACHABLE_DEFAULT_CASE();
                        }
                    } else {
                        /* Our message queue was signaled. */
                        port_manager.ProcessMessages();

                        /* We have nothing to reply to. */
                        reply_target = svc::InvalidHandle;
                    }
                }
            }

            void TriggerAddSession(svc::Handle session_handle, size_t port_index) {
                /* Acquire exclusive access to ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Select the best port manager. */
                PortManagerBase *best_manager = nullptr;
                s32 best_sessions             = -1;
                [this, &best_manager, &best_sessions]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                    (this->TrySelectBetterPort<Ix>(best_manager, best_sessions), ...);
                }(std::make_index_sequence<NumPorts>());

                /* Trigger the session add on the least-burdened manager. */
                best_manager->TriggerAddSession(session_handle, port_index);
            }

            template<size_t Ix> requires (Ix < NumPorts)
            void TrySelectBetterPort(PortManagerBase *&best_manager, s32 &best_sessions) {
                auto &cur_manager       = this->GetPortManager<Ix>();
                const auto cur_sessions = cur_manager.GetSessionCount();

                /* NOTE: It's unknown how nintendo handles the case where the last manager has more sessions (to cover the remainder). */
                /* Our algorithm diverges from theirs (it does not do std::min bounds capping), to accommodate remainder ports.  */
                /* If we learn how they handle this edge case, we can change our ways to match theirs. */

                if constexpr (Ix == 0) {
                    best_manager  = std::addressof(cur_manager);
                    best_sessions = cur_sessions;
                } else {
                    static_assert(SessionsPerPortManager<Ix - 1> == SessionsPerPortManager<0>);
                    static_assert(SessionsPerPortManager<Ix - 1> <= SessionsPerPortManager<Ix>);
                    if (cur_sessions < best_sessions || best_sessions >= static_cast<s32>(SessionsPerPortManager<Ix - 1>)) {
                        best_manager  = std::addressof(cur_manager);
                        best_sessions = cur_sessions;
                    }
                }
            }

            template<size_t Ix>
            void TriggerResumeImpl(ResumeKey resume_key) {
                /* Get the port manager. */
                auto &port_manager = this->GetPortManager<Ix>();

                /* If we should, trigger a resume. */
                if (port_manager.TestResume(resume_key)) {
                    port_manager.TriggerResume(resume_key);
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

