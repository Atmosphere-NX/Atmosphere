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
#include <vapours.hpp>
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_service_object.hpp>
#include <stratosphere/tipc/tipc_object_manager.hpp>
#include <stratosphere/tipc/tipc_deferral_manager.hpp>

namespace ams::tipc {

    template<size_t NumSessions, typename Interface, typename Impl, template<typename, size_t> typename _Allocator>
    struct PortMeta {
        static constexpr inline size_t MaxSessions = NumSessions;

        static constexpr bool CanDeferInvokeRequest = IsDeferrable<Impl>;

        using ServiceObject = tipc::ServiceObject<Interface, Impl>;

        using Allocator     = _Allocator<ServiceObject, NumSessions>;
    };

    struct DummyDeferralManagerBase{};

    template<size_t N>
    struct DummyDeferralManager : public DummyDeferralManagerBase {};

    namespace impl {

        template<size_t ThreadStackSize, bool IsDeferralSupported, size_t NumPorts, typename... PortInfos>
        class ServerManagerImpl {
            private:
                static_assert(NumPorts == sizeof...(PortInfos));
                static constexpr inline size_t MaxSessions = (PortInfos::MaxSessions + ...);

                /* Verify that we have at least one port. */
                static_assert(NumPorts > 0);

                /* Verify that it's possible to service this many sessions, with our port manager count. */
                static_assert(MaxSessions <= NumPorts * svc::ArgumentHandleCountMax);

                static_assert(util::IsAligned(ThreadStackSize, os::ThreadStackAlignment));
                alignas(os::ThreadStackAlignment) static constinit inline u8 s_port_stacks[ThreadStackSize * (NumPorts - 1)];

                template<size_t Ix> requires (Ix < NumPorts)
                static constexpr inline size_t SessionsPerPortManager = (Ix == NumPorts - 1) ? ((MaxSessions / NumPorts) + MaxSessions % NumPorts)
                                                                                             : ((MaxSessions / NumPorts));

                template<size_t Ix> requires (Ix < NumPorts)
                using PortInfo = typename std::tuple_element<Ix, std::tuple<PortInfos...>>::type;

                static_assert(IsDeferralSupported == (PortInfos::CanDeferInvokeRequest || ...));

                template<size_t Sessions>
                using DeferralManagerImplType = typename std::conditional<IsDeferralSupported, DeferralManager<Sessions>, DummyDeferralManager<Sessions>>::type;

                using DeferralManagerBaseType = typename std::conditional<IsDeferralSupported, DeferralManagerBase, DummyDeferralManagerBase>::type;

                template<size_t Ix> requires (Ix < NumPorts)
                static constexpr inline bool IsPortDeferrable = PortInfo<Ix>::CanDeferInvokeRequest;
            public:
                class PortManagerBase {
                    public:
                        enum MessageType : u8 {
                            MessageType_AddSession    = 0,
                            MessageType_TriggerResume = 1,
                        };
                    protected:
                        s32 m_id;
                        std::atomic<s32> m_num_sessions;
                        s32 m_port_number;
                        os::MultiWaitType m_multi_wait;
                        os::MessageQueueType m_message_queue;
                        os::MultiWaitHolderType m_message_queue_holder;
                        uintptr_t m_message_queue_storage[MaxSessions];
                        ServerManagerImpl *m_server_manager;
                        ObjectManagerBase *m_object_manager;
                        DeferralManagerBaseType *m_deferral_manager;
                    public:
                        PortManagerBase() : m_id(), m_num_sessions(), m_port_number(), m_multi_wait(), m_message_queue(), m_message_queue_holder(), m_message_queue_storage(), m_server_manager(), m_object_manager(), m_deferral_manager() {
                            /* Setup our message queue. */
                            os::InitializeMessageQueue(std::addressof(m_message_queue), m_message_queue_storage, util::size(m_message_queue_storage));
                            os::InitializeMultiWaitHolder(std::addressof(m_message_queue_holder), std::addressof(m_message_queue), os::MessageQueueWaitType::ForNotEmpty);
                        }

                        constexpr s32 GetPortIndex() const {
                            return m_port_number;
                        }

                        s32 GetSessionCount() const {
                            return m_num_sessions;
                        }

                        void InitializeBase(s32 id, ServerManagerImpl *sm, DeferralManagerBaseType *dm, ObjectManagerBase *om) {
                            /* Set our id. */
                            m_id = id;

                            /* Set our server manager. */
                            m_server_manager = sm;

                            /* Reset our session count. */
                            m_num_sessions = 0;

                            /* Initialize our multi wait. */
                            os::InitializeMultiWait(std::addressof(m_multi_wait));
                            os::LinkMultiWaitHolder(std::addressof(m_multi_wait), std::addressof(m_message_queue_holder));

                            /* Initialize our object manager. */
                            m_object_manager = om;

                            /* Initialize our deferral manager. */
                            m_deferral_manager = dm;
                        }

                        void RegisterPort(s32 index, os::NativeHandle port_handle) {
                            /* Set our port number. */
                            m_port_number = index;

                            /* Create an object holder for the port. */
                            tipc::ObjectHolder object;

                            /* Setup the object. */
                            object.InitializeAsPort(port_handle);

                            /* Register the object. */
                            m_object_manager->AddObject(object);
                        }

                        os::NativeHandle ProcessRequest(ObjectHolder &object) {
                            /* Acquire exclusive server manager access. */
                            std::scoped_lock lk(m_server_manager->GetMutex());

                            /* Process the request. */
                            const Result result = m_object_manager->ProcessRequest(object);
                            if (R_SUCCEEDED(result)) {
                                /* We should reply only if the request isn't deferred. */
                                return !IsRequestDeferred() ? object.GetHandle() : os::InvalidNativeHandle;
                            } else {
                                /* Processing failed, so note the session as closed (or close it). */
                                this->CloseSessionIfNecessary(object, !tipc::ResultSessionClosed::Includes(result));

                                /* We shouldn't reply on failure. */
                                return os::InvalidNativeHandle;
                            }
                        }

                        template<bool Enable = IsDeferralSupported, typename = typename std::enable_if<Enable>::type>
                        void ProcessDeferredRequest(ObjectHolder &object) {
                            static_assert(Enable == IsDeferralSupported);

                            if (const auto reply_target = this->ProcessRequest(object); reply_target != os::InvalidNativeHandle) {
                                m_object_manager->Reply(reply_target);
                            }
                        }

                        bool ReplyAndReceive(os::MultiWaitHolderType **out_holder, ObjectHolder *out_object, os::NativeHandle reply_target) {
                            /* If we don't have a reply target, clear our message buffer. */
                            if (reply_target == os::InvalidNativeHandle) {
                                svc::ipc::MessageBuffer(svc::ipc::GetMessageBuffer()).SetNull();
                            }

                            /* Try to reply/receive. */
                            const Result result = m_object_manager->ReplyAndReceive(out_holder, out_object, reply_target, std::addressof(m_multi_wait));

                            /* Acquire exclusive access to the server manager. */
                            std::scoped_lock lk(m_server_manager->GetMutex());

                            /* Handle the result. */
                            R_TRY_CATCH(result) {
                                R_CATCH(os::ResultSessionClosedForReceive, os::ResultReceiveListBroken) {
                                    /* Close the object. */
                                    this->CloseSession(*out_object);

                                    /* We don't have anything to process. */
                                    return false;
                                }
                            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                            return true;
                        }

                        void AddSession(os::NativeHandle session_handle, tipc::ServiceObjectBase *service_object) {
                            /* Create an object holder for the session. */
                            tipc::ObjectHolder object;

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
                                            const os::NativeHandle session_handle = static_cast<os::NativeHandle>(message_type >> BITSIZEOF(u32));

                                            /* Allocate a service object for the port. */
                                            auto *service_object = m_server_manager->AllocateObject(static_cast<size_t>(message_data), session_handle, *m_deferral_manager);

                                            /* Add the newly-created service object. */
                                            this->AddSession(session_handle, service_object);
                                        }
                                        break;
                                    case MessageType_TriggerResume:
                                        if constexpr (IsDeferralSupported) {
                                            /* Perform the resume. */
                                            this->OnTriggerResume(message_data);
                                        }
                                        break;
                                    AMS_UNREACHABLE_DEFAULT_CASE();
                                }
                            }
                        }

                        void CloseSession(ObjectHolder &object) {
                            /* Get the object's handle. */
                            const auto handle = object.GetHandle();

                            /* Close the object with our manager. */
                            m_object_manager->CloseObject(handle);

                            /* Close the handle itself. */
                            R_ABORT_UNLESS(svc::CloseHandle(handle));

                            /* Decrement our session count. */
                            --m_num_sessions;
                        }

                        void CloseSessionIfNecessary(ObjectHolder &object, bool necessary) {
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

                        bool TestResume(uintptr_t key) {
                            if constexpr (IsDeferralSupported) {
                                /* Acquire exclusive server manager access. */
                                std::scoped_lock lk(m_server_manager->GetMutex());

                                /* Check to see if the key corresponds to some deferred message. */
                                return m_deferral_manager->TestResume(key);
                            } else {
                                return false;
                            }
                        }

                        void TriggerResume(uintptr_t key) {
                            /* Acquire exclusive server manager access. */
                            std::scoped_lock lk(m_server_manager->GetMutex());

                            /* Send the key as a message. */
                            os::SendMessageQueue(std::addressof(m_message_queue), static_cast<uintptr_t>(MessageType_TriggerResume));
                            os::SendMessageQueue(std::addressof(m_message_queue), key);
                        }

                        void TriggerAddSession(os::NativeHandle session_handle, size_t port_index) {
                            /* Increment our session count. */
                            ++m_num_sessions;

                            /* Send information about the session as a message. */
                            os::SendMessageQueue(std::addressof(m_message_queue), static_cast<uintptr_t>(MessageType_AddSession) | (static_cast<u64>(session_handle) << BITSIZEOF(u32)));
                            os::SendMessageQueue(std::addressof(m_message_queue), static_cast<uintptr_t>(port_index));
                        }
                    private:
                        void OnTriggerResume(uintptr_t key) {
                            /* Acquire exclusive server manager access. */
                            std::scoped_lock lk(m_server_manager->GetMutex());

                            /* Trigger the resume. */
                            m_deferral_manager->TriggerResume(this, key);
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
                        DeferralManagerImplType<PortSessions> m_deferral_manager_impl;
                        tipc::ObjectManager<1 + PortSessions> m_object_manager_impl;
                    public:
                        PortManagerImpl() : PortManagerBase(), m_deferral_manager_impl(), m_object_manager_impl() {
                            /* ...  */
                        }

                        void Initialize(s32 id, ServerManagerImpl *sm) {
                            /* Initialize our base. */
                            this->InitializeBase(id, sm, std::addressof(m_deferral_manager_impl), std::addressof(m_object_manager_impl));

                            /* Initialize our object manager. */
                            m_object_manager_impl.Initialize(std::addressof(this->m_multi_wait));
                        }
                };

                template<size_t Ix>
                using PortManager = PortManagerImpl<PortInfo<Ix>, SessionsPerPortManager<Ix>>;

                using PortManagerTuple = decltype([]<size_t... Ix>(std::index_sequence<Ix...>) {
                    return std::tuple<PortManager<Ix>...>{};
                }(std::make_index_sequence<NumPorts>()));

                using PortAllocatorTuple = std::tuple<typename PortInfos::Allocator...>;
            private:
                os::SdkRecursiveMutex m_mutex;
                PortManagerTuple m_port_managers;
                PortAllocatorTuple m_port_allocators;
                os::ThreadType m_port_threads[NumPorts - 1];
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
                void InitializePortThread(s32 priority, const char *name) {
                    /* Create the thread. */
                    R_ABORT_UNLESS(os::CreateThread(m_port_threads + Ix, &LoopAutoForPortThreadFunction<Ix>, this, s_port_stacks + Ix, ThreadStackSize, priority));

                    /* Set the thread name pointer. */
                    if (name != nullptr) {
                        os::SetThreadNamePointer(m_port_threads + Ix, name);
                    }

                    /* Start the thread. */
                    os::StartThread(m_port_threads + Ix);
                }
            public:
                ServerManagerImpl() : m_mutex(), m_port_managers(), m_port_allocators() { /* ... */ }

                os::SdkRecursiveMutex &GetMutex() { return m_mutex; }

                void Initialize() {
                    /* Initialize our port managers. */
                    [this]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                        (this->GetPortManager<Ix>().Initialize(static_cast<s32>(Ix), this), ...);
                    }(std::make_index_sequence<NumPorts>());
                }

                template<size_t Ix>
                void RegisterPort(os::NativeHandle port_handle) {
                    this->GetPortManager<Ix>().RegisterPort(static_cast<s32>(Ix), port_handle);
                }

                template<size_t Ix>
                void RegisterPort(sm::ServiceName service_name, size_t max_sessions) {
                    /* Register service. */
                    os::NativeHandle port_handle;
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
                            (this->InitializePortThread<Ix>(thread_priority, nullptr), ...);
                        }(std::make_index_sequence<NumPorts - 1>());
                    }

                    /* Process for the last port. */
                    this->LoopAutoForPort<NumPorts - 1>();
                }

                void LoopAuto(int priority, const char *name) {
                    /* If we have additional threads, create and start them. */
                    if constexpr (NumPorts > 1) {
                        [priority, name, this]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                            /* Create all threads. */
                            (this->InitializePortThread<Ix>(priority, name), ...);
                        }(std::make_index_sequence<NumPorts - 1>());
                    }

                    /* Check current thread. */
                    {
                        AMS_ASSERT(priority == os::GetThreadPriority(os::GetCurrentThread()));
                        /* N does not do: os::SetThreadNamePointer(os::GetCurrentThread(), name); */
                    }

                    /* Process for the last port. */
                    this->LoopAutoForPort<NumPorts - 1>();
                }

                tipc::ServiceObjectBase *AllocateObject(size_t port_index, os::NativeHandle handle, DeferralManagerBaseType &deferral_manager) {
                    /* Check that the port index is valid. */
                    AMS_ABORT_UNLESS(port_index < NumPorts);

                    /* Try to allocate from each port, in turn. */
                    tipc::ServiceObjectBase *allocated = nullptr;
                    [this, port_index, handle, &deferral_manager, &allocated]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                        (this->TryAllocateObject<Ix>(port_index, handle, deferral_manager, allocated), ...);
                    }(std::make_index_sequence<NumPorts>());

                    /* Return the allocated object. */
                    AMS_ABORT_UNLESS(allocated != nullptr);
                    return allocated;
                }

                template<IsResumeKey ResumeKey>
                void TriggerResume(const ResumeKey &resume_key) {
                    /* Acquire exclusive access to ourselves. */
                    std::scoped_lock lk(m_mutex);

                    /* Convert to internal resume key. */
                    const auto internal_resume_key = ConvertToInternalResumeKey(resume_key);

                    /* Check/trigger resume on each of our ports. */
                    [this, internal_resume_key]<size_t... Ix>(std::index_sequence<Ix...>) ALWAYS_INLINE_LAMBDA {
                        (this->TriggerResumeImpl<Ix>(internal_resume_key), ...);
                    }(std::make_index_sequence<NumPorts>());
                }

                Result AddSession(os::NativeHandle *out, tipc::ServiceObjectBase *object) {
                    /* Acquire exclusive access to ourselves. */
                    std::scoped_lock lk(m_mutex);

                    /* Create a handle for the session. */
                    svc::Handle session_handle;
                    R_TRY(svc::CreateSession(std::addressof(session_handle), static_cast<svc::Handle *>(out), false, 0));

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
                void TryAllocateObject(size_t port_index, os::NativeHandle handle, DeferralManagerBaseType &deferral_manager, tipc::ServiceObjectBase *&allocated) {
                    /* Check that the port index matches. */
                    if (port_index == Ix) {
                        /* Check that we haven't already allocated. */
                        AMS_ABORT_UNLESS(allocated == nullptr);

                        /* Get the allocator. */
                        auto &allocator = std::get<Ix>(m_port_allocators);

                        /* Allocate the object. */
                        auto * const new_object = allocator.Allocate();
                        AMS_ABORT_UNLESS(new_object != nullptr);

                        /* If we should, set the object's deleter. */
                        if constexpr (IsServiceObjectDeleter<typename std::tuple_element<Ix, PortAllocatorTuple>::type>) {
                            new_object->SetDeleter(std::addressof(allocator));
                        }

                        /* If we should, set the object's deferral manager. */
                        if constexpr (IsPortDeferrable<Ix>) {
                            deferral_manager.AddObject(new_object->GetImpl(), handle, new_object);
                        }

                        /* Set the allocated object. */
                        allocated = new_object;
                    }
                }

                Result LoopProcess(PortManagerBase &port_manager) {
                    /* Process requests forever. */
                    os::NativeHandle reply_target = os::InvalidNativeHandle;
                    while (true) {
                        /* Reply to our pending request, and wait to receive a new one. */
                        os::MultiWaitHolderType *signaled_holder = nullptr;
                        tipc::ObjectHolder signaled_object{};
                        while (!port_manager.ReplyAndReceive(std::addressof(signaled_holder), std::addressof(signaled_object), reply_target)) {
                            reply_target    = os::InvalidNativeHandle;
                            signaled_object = {};
                        }

                        if (signaled_holder == nullptr) {
                            /* A session was signaled, accessible via signaled_object. */
                            switch (signaled_object.GetType()) {
                                case ObjectHolder::ObjectType_Port:
                                    {
                                        /* Try to accept a new session */
                                        svc::Handle session_handle;
                                        if (R_SUCCEEDED(svc::AcceptSession(std::addressof(session_handle), signaled_object.GetHandle()))) {
                                            this->TriggerAddSession(session_handle, static_cast<size_t>(port_manager.GetPortIndex()));
                                        }

                                        /* We have nothing to reply to. */
                                        reply_target = os::InvalidNativeHandle;
                                    }
                                    break;
                                case ObjectHolder::ObjectType_Session:
                                    {
                                        /* Process the request */
                                        reply_target = port_manager.ProcessRequest(signaled_object);
                                    }
                                    break;
                                AMS_UNREACHABLE_DEFAULT_CASE();
                            }
                        } else {
                            /* Our message queue was signaled. */
                            port_manager.ProcessMessages();

                            /* We have nothing to reply to. */
                            reply_target = os::InvalidNativeHandle;
                        }
                    }
                }

                void TriggerAddSession(os::NativeHandle session_handle, size_t port_index) {
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
                void TriggerResumeImpl(uintptr_t resume_key) {
                    /* Get the port manager. */
                    auto &port_manager = this->GetPortManager<Ix>();

                    /* If we should, trigger a resume. */
                    if (port_manager.TestResume(resume_key)) {
                        port_manager.TriggerResume(resume_key);
                    }
                }
        };

        template<size_t ThreadStackSize, typename PortInfo>
        class ServerManagerImpl<ThreadStackSize, false, 1, PortInfo> {
            private:
                static constexpr inline size_t NumPorts = 1;
                static constexpr inline size_t MaxSessions = PortInfo::MaxSessions;

                /* Verify that it's possible to service this many sessions, with our port manager count. */
                static_assert(MaxSessions <= svc::ArgumentHandleCountMax);
            public:
                class PortManagerBase {
                    protected:
                        os::MultiWaitType m_multi_wait;
                        ObjectManagerBase *m_object_manager;
                    public:
                        constexpr PortManagerBase() : m_multi_wait(), m_object_manager() { /* ... */ }

                        void InitializeBase(ObjectManagerBase *om) {
                            /* Initialize our multi wait. */
                            os::InitializeMultiWait(std::addressof(m_multi_wait));

                            /* Initialize our object manager. */
                            m_object_manager = om;
                        }

                        void RegisterPort(os::NativeHandle port_handle) {
                            /* Create an object holder for the port. */
                            tipc::ObjectHolder object;

                            /* Setup the object. */
                            object.InitializeAsPort(port_handle);

                            /* Register the object. */
                            m_object_manager->AddObject(object);
                        }

                        os::NativeHandle ProcessRequest(ObjectHolder &object) {
                            /* Process the request. */
                            const Result result = m_object_manager->ProcessRequest(object);
                            if (R_SUCCEEDED(result)) {
                                /* We should reply only if the request isn't deferred. */
                                return object.GetHandle();
                            } else {
                                /* Processing failed, so close the session if we need to. */
                                if (!tipc::ResultSessionClosed::Includes(result)) {
                                    this->CloseSession(object);
                                }

                                /* We shouldn't reply on failure. */
                                return os::InvalidNativeHandle;
                            }
                        }

                        bool ReplyAndReceive(ObjectHolder *out_object, os::NativeHandle reply_target) {
                            /* If we don't have a reply target, clear our message buffer. */
                            if (reply_target == os::InvalidNativeHandle) {
                                svc::ipc::MessageBuffer(svc::ipc::GetMessageBuffer()).SetNull();
                            }

                            /* Try to reply/receive. */
                            const Result result = [&]() ALWAYS_INLINE_LAMBDA -> Result {
                                os::MultiWaitHolderType *signaled_holder = nullptr;
                                ON_SCOPE_EXIT { AMS_ABORT_UNLESS(signaled_holder == nullptr); };
                                return m_object_manager->ReplyAndReceive(std::addressof(signaled_holder), out_object, reply_target, std::addressof(m_multi_wait));
                            }();

                            /* Handle the result. */
                            if (R_FAILED(result)) {
                                /* Close the object. */
                                this->CloseSession(*out_object);

                                /* We don't have anything to process. */
                                return false;
                            }

                            return true;
                        }

                        void AddSession(os::NativeHandle session_handle, tipc::ServiceObjectBase *service_object) {
                            /* Create an object holder for the session. */
                            tipc::ObjectHolder object;

                            /* Setup the object. */
                            object.InitializeAsSession(session_handle, true, service_object);

                            /* Register the object. */
                            m_object_manager->AddObject(object);
                        }

                        void CloseSession(ObjectHolder &object) {
                            /* Get the object's handle. */
                            const auto handle = object.GetHandle();

                            /* Close the object with our manager. */
                            m_object_manager->CloseObject(handle);

                            /* Close the handle itself. */
                            R_ABORT_UNLESS(svc::CloseHandle(handle));
                        }
                };

                class PortManagerImpl final : public PortManagerBase {
                    private:
                        tipc::ObjectManager<1 + MaxSessions> m_object_manager_impl;
                    public:
                        constexpr PortManagerImpl() : PortManagerBase(), m_object_manager_impl() {
                            /* ...  */
                        }

                        void Initialize() {
                            /* Initialize our base. */
                            this->InitializeBase(std::addressof(m_object_manager_impl));

                            /* Initialize our object manager. */
                            m_object_manager_impl.Initialize(std::addressof(this->m_multi_wait));
                        }
                };

                using PortManager = PortManagerImpl;
            private:
                PortManager m_port_manager;
                typename PortInfo::Allocator m_port_allocator;
            public:
                constexpr ServerManagerImpl() : m_port_manager(), m_port_allocator() { /* ... */ }

                void Initialize() {
                    /* Initialize our port manager. */
                    m_port_manager.Initialize();
                }

                void RegisterPort(os::NativeHandle port_handle) {
                    m_port_manager.RegisterPort(port_handle);
                }

                void RegisterPort(sm::ServiceName service_name, size_t max_sessions) {
                    /* Register service. */
                    os::NativeHandle port_handle;
                    R_ABORT_UNLESS(sm::RegisterService(std::addressof(port_handle), service_name, max_sessions, false));

                    /* Register the port handle. */
                    this->RegisterPort(port_handle);
                }

                void LoopAuto() {
                    /* Process for the only port. */
                    this->LoopProcess(m_port_manager);
                }

                tipc::ServiceObjectBase *AllocateObject() {
                    /* Allocate the object. */
                    auto * const new_object = m_port_allocator.Allocate();
                    AMS_ABORT_UNLESS(new_object != nullptr);

                    /* If we should, set the object's deleter. */
                    if constexpr (IsServiceObjectDeleter<typename PortInfo::Allocator>) {
                        new_object->SetDeleter(std::addressof(m_port_allocator));
                    }

                    return new_object;
                }

                Result AddSession(os::NativeHandle *out, tipc::ServiceObjectBase *object) {
                    /* Create a handle for the session. */
                    svc::Handle session_handle;
                    R_TRY(svc::CreateSession(std::addressof(session_handle), static_cast<svc::Handle *>(out), false, 0));

                    /* Add the session to our manager. */
                    m_port_manager.AddSession(session_handle, object);

                    return ResultSuccess();
                }
            private:
                void LoopProcess(PortManagerBase &port_manager) {
                    /* Process requests forever. */
                    os::NativeHandle reply_target = os::InvalidNativeHandle;
                    while (true) {
                        /* Reply to our pending request, and wait to receive a new one. */
                        tipc::ObjectHolder signaled_object{};
                        while (!port_manager.ReplyAndReceive(std::addressof(signaled_object), reply_target)) {
                            reply_target = os::InvalidNativeHandle;
                        }

                        /* A session was signaled, accessible via signaled_object. */
                        switch (signaled_object.GetType()) {
                            case ObjectHolder::ObjectType_Port:
                                {
                                    /* Try to accept a new session */
                                    svc::Handle session_handle;
                                    if (R_SUCCEEDED(svc::AcceptSession(std::addressof(session_handle), signaled_object.GetHandle()))) {
                                        port_manager.AddSession(session_handle, this->AllocateObject());
                                    }

                                    /* We have nothing to reply to. */
                                    reply_target = os::InvalidNativeHandle;
                                }
                                break;
                            case ObjectHolder::ObjectType_Session:
                                {
                                    /* Process the request */
                                    reply_target = port_manager.ProcessRequest(signaled_object);
                                }
                                break;
                            AMS_UNREACHABLE_DEFAULT_CASE();
                        }
                    }
                }
        };

    }

    template<size_t ThreadStackSize, typename... PortInfos>
    using ServerManagerImpl = impl::ServerManagerImpl<ThreadStackSize, (PortInfos::CanDeferInvokeRequest || ...), sizeof...(PortInfos), PortInfos...>;


    template<typename... PortInfos>
    using ServerManager = ServerManagerImpl<os::MemoryPageSize, PortInfos...>;

    template<size_t ThreadStackSize, typename... PortInfos>
    using ServerManagerWithThreadStack = ServerManagerImpl<ThreadStackSize, PortInfos...>;

}

