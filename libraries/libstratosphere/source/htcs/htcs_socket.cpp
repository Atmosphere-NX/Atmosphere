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
#include "client/htcs_session.hpp"
#include "client/htcs_virtual_socket_collection.hpp"

namespace ams::htcs {

    namespace {

        constinit bool g_initialized = false;
        constinit bool g_enable_disconnection_emulation = false;

        constinit AllocateFunction g_allocate_function     = nullptr;
        constinit DeallocateFunction g_deallocate_function = nullptr;

        constinit void *g_buffer = nullptr;
        constinit size_t g_buffer_size = 0;

        constinit os::TlsSlot g_tls_slot = {};

        constinit tma::IHtcsManager *g_manager = nullptr;
        constinit tma::IHtcsManager *g_monitor = nullptr;

        constinit client::VirtualSocketCollection *g_sockets = nullptr;

        void SetLastError(uintptr_t error_code) {
            os::SetTlsValue(g_tls_slot, error_code);
        }

        void InitializeImpl(void *buffer, size_t buffer_size, int num_sessions) {
            /* Check the session count. */
            AMS_ASSERT(0 < num_sessions && num_sessions <= SessionCountMax);

            /* Initialize the manager and monitor. */
            client::InitializeSessionManager(std::addressof(g_manager), std::addressof(g_monitor), num_sessions);

            /* Register the process. */
            const sf::ClientProcessId process_id{0};
            R_ABORT_UNLESS(g_manager->RegisterProcessId(process_id));
            R_ABORT_UNLESS(g_monitor->MonitorManager(process_id));

            /* Allocate a tls slot for our last error. */
            R_ABORT_UNLESS(os::SdkAllocateTlsSlot(std::addressof(g_tls_slot), nullptr));

            /* Setup the virtual socket collection. */
            AMS_ASSERT(buffer != nullptr);
            g_sockets = reinterpret_cast<client::VirtualSocketCollection *>(buffer);
            std::construct_at(g_sockets);
            g_sockets->Init(static_cast<u8 *>(buffer) + sizeof(*g_sockets), buffer_size - sizeof(*g_sockets));

            /* Mark initialized. */
            g_initialized = true;
        }

        void InitializeImpl(AllocateFunction allocate, DeallocateFunction deallocate, int num_sessions, int num_sockets) {
            /* Check the session count. */
            AMS_ASSERT(0 < num_sessions && num_sessions <= SessionCountMax);

            /* Set the allocation functions. */
            g_allocate_function   = allocate;
            g_deallocate_function = deallocate;

            /* Allocate a buffer. */
            g_buffer_size = sizeof(client::VirtualSocketCollection) + client::VirtualSocketCollection::GetWorkingMemorySize(num_sockets);
            g_buffer      = g_allocate_function(g_buffer_size);

            /* Initialize. */
            InitializeImpl(g_buffer, g_buffer_size, num_sessions);
        }

    }

    bool IsInitialized() {
        return g_initialized;
    }

    size_t GetWorkingMemorySize(int num_sockets) {
        AMS_ASSERT(num_sockets <= SocketCountMax);
        return sizeof(client::VirtualSocketCollection) + client::VirtualSocketCollection::GetWorkingMemorySize(num_sockets);
    }

    void Initialize(AllocateFunction allocate, DeallocateFunction deallocate, int num_sessions) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = true;

        /* Initialize. */
        InitializeImpl(allocate, deallocate, num_sessions, htcs::SocketCountMax);
    }

    void Initialize(void *buffer, size_t buffer_size) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = true;

        /* Initialize. */
        InitializeImpl(buffer, buffer_size, htcs::SessionCountMax);
    }

    void InitializeForDisableDisconnectionEmulation(AllocateFunction allocate, DeallocateFunction deallocate, int num_sessions) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = false;

        /* Initialize. */
        InitializeImpl(allocate, deallocate, num_sessions, htcs::SocketCountMax);
    }

    void InitializeForDisableDisconnectionEmulation(void *buffer, size_t buffer_size) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = false;

        /* Initialize. */
        InitializeImpl(buffer, buffer_size, htcs::SessionCountMax);
    }

    void InitializeForSystem(void *buffer, size_t buffer_size, int num_sessions) {
        /* Check that we're not already initialized. */
        AMS_ASSERT(!IsInitialized());

        /* Configure disconnection emulation. */
        g_enable_disconnection_emulation = true;

        /* Initialize. */
        InitializeImpl(buffer, buffer_size, num_sessions);
    }

    void Finalize() {
        /* Check that we're initialized. */
        AMS_ASSERT(IsInitialized());

        /* Set not initialized. */
        g_initialized = false;

        /* Destroy the virtual socket collection. */
        std::destroy_at(g_sockets);
        g_sockets = nullptr;

        /* Free the buffer, if we have one. */
        if (g_buffer != nullptr) {
            g_deallocate_function(g_buffer, g_buffer_size);
            g_buffer      = nullptr;
            g_buffer_size = 0;
        }

        /* Free the tls slot. */
        os::FreeTlsSlot(g_tls_slot);

        /* Release the manager objects. */
        sf::ReleaseSharedObject(g_manager);
        sf::ReleaseSharedObject(g_monitor);
        g_manager = nullptr;
        g_monitor = nullptr;

        /* Finalize the htcs client sessions. */
        client::FinalizeSessionManager();
    }

    const HtcsPeerName GetPeerNameAny() {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Get name. */
        HtcsPeerName name;
        static_cast<void>(g_manager->GetPeerNameAny(std::addressof(name)));

        return name;
    }

    const HtcsPeerName GetDefaultHostName() {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Get name. */
        HtcsPeerName name;
        static_cast<void>(g_manager->GetDefaultHostName(std::addressof(name)));

        return name;
    }

    s32 GetLastError() {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        return static_cast<s32>(os::GetTlsValue(g_tls_slot));
    }

    s32 Socket() {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Socket(error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    s32 Close(s32 desc) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Close(desc, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    s32 Connect(s32 desc, const SockAddrHtcs *address) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Check that the address family is correct. */
        AMS_ASSERT(address->family == HTCS_AF_HTCS);

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Connect(desc, address, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    s32 Bind(s32 desc, const SockAddrHtcs *address) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Check that the address family is correct. */
        AMS_ASSERT(address->family == HTCS_AF_HTCS);

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Bind(desc, address, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    s32 Listen(s32 desc, s32 backlog_count) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Listen(desc, backlog_count, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    s32 Accept(s32 desc, SockAddrHtcs *address) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Ensure we have an address. */
        SockAddrHtcs tmp;
        if (address == nullptr) {
            address = std::addressof(tmp);
        }

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Accept(desc, address, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    s32 Shutdown(s32 desc, s32 how) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Shutdown(desc, how, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    s32 Fcntl(s32 desc, s32 command, s32 value) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Fcntl(desc, command, value, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    s32 Select(s32 count, FdSet *read, FdSet *write, FdSet *exception, TimeVal *timeout) {
        AMS_UNUSED(count);

        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Check that we have some form of input. */
        if (read == nullptr && write == nullptr && exception == nullptr) {
            SetLastError(static_cast<uintptr_t>(HTCS_EINVAL));
            return -1;
        }

        /* Check that the timeout is valid. */
        if (timeout != nullptr && (timeout->tv_sec < 0 || timeout->tv_usec < 0)) {
            SetLastError(static_cast<uintptr_t>(HTCS_EINVAL));
            return -1;
        }

        /* Perform the operation. */
        s32 error_code = 0;
        const s32 ret = g_sockets->Select(read, write, exception, timeout, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    ssize_t Recv(s32 desc, void *buffer, size_t buffer_size, s32 flags) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Perform the operation. */
        s32 error_code = 0;
        const ssize_t ret = g_sockets->Recv(desc, buffer, buffer_size, flags, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    ssize_t Send(s32 desc, const void *buffer, size_t buffer_size, s32 flags) {
        /* Check that we have a manager. */
        AMS_ASSERT(g_manager != nullptr);

        /* Check that we have a socket collection. */
        AMS_ASSERT(g_sockets != nullptr);

        /* Perform the operation. */
        s32 error_code = 0;
        const ssize_t ret = g_sockets->Send(desc, buffer, buffer_size, flags, error_code);
        if (ret < 0) {
            SetLastError(static_cast<uintptr_t>(error_code));
        }

        return ret;
    }

    void FdSetZero(FdSet *set) {
        AMS_ASSERT(set != nullptr);

        std::memset(set, 0, sizeof(*set));
    }

    void FdSetSet(s32 fd, FdSet *set) {
        AMS_ASSERT(set != nullptr);

        for (auto i = 0; i < FdSetSize; ++i) {
            if (set->fds[i] == 0) {
                set->fds[i] = fd;
                break;
            }
        }
    }

    void FdSetClr(s32 fd, FdSet *set) {
        AMS_ASSERT(set != nullptr);

        for (auto i = 0; i < FdSetSize; ++i) {
            if (set->fds[i] == fd) {
                std::memcpy(set->fds + i, set->fds + i + 1, (FdSetSize - (i + 1)) * sizeof(fd));
                set->fds[FdSetSize - 1] = 0;
                break;
            }
        }
    }

    bool FdSetIsSet(s32 fd, const FdSet *set) {
        AMS_ASSERT(set != nullptr);

        for (auto i = 0; i < FdSetSize; ++i) {
            if (set->fds[i] == fd) {
                return true;
            }
        }

        return false;
    }

    namespace client {

        sf::SharedPointer<tma::ISocket> socket(s32 &last_error) {
            sf::SharedPointer<tma::ISocket> socket = nullptr;
            R_ABORT_UNLESS(g_manager->CreateSocket(std::addressof(last_error), std::addressof(socket), g_enable_disconnection_emulation));
            return socket;
        }

        s32 close(sf::SharedPointer<tma::ISocket> socket, s32 &last_error) {
            s32 res;
            static_cast<void>(socket->Close(std::addressof(last_error), std::addressof(res)));
            return res;
        }

        s32 bind(sf::SharedPointer<tma::ISocket> socket, const htcs::SockAddrHtcs *address, s32 &last_error) {
            /* Create null-terminated address. */
            htcs::SockAddrHtcs null_terminated_address;
            null_terminated_address.family = address->family;
            util::Strlcpy(null_terminated_address.peer_name.name, address->peer_name.name, PeerNameBufferLength);
            util::Strlcpy(null_terminated_address.port_name.name, address->port_name.name, PortNameBufferLength);

            s32 res;
            static_cast<void>(socket->Bind(std::addressof(last_error), std::addressof(res), null_terminated_address));
            return res;
        }

        s32 listen(sf::SharedPointer<tma::ISocket> socket, s32 backlog_count, s32 &last_error) {
            s32 res;
            static_cast<void>(socket->Listen(std::addressof(last_error), std::addressof(res), backlog_count));
            return res;
        }

        sf::SharedPointer<tma::ISocket> accept(sf::SharedPointer<tma::ISocket> socket, htcs::SockAddrHtcs *address, s32 &last_error) {
            /* Begin the accept. */
            sf::SharedPointer<tma::ISocket> res = nullptr;
            u32 task_id = 0;
            sf::NativeHandle event_handle;
            if (R_SUCCEEDED(socket->AcceptStart(std::addressof(task_id), std::addressof(event_handle)))) {
                /* Create system event. */
                os::SystemEventType event;
                os::AttachReadableHandleToSystemEvent(std::addressof(event), event_handle.GetOsHandle(), event_handle.IsManaged(), os::EventClearMode_ManualClear);
                event_handle.Detach();

                /* When we're done, clean up the event. */
                ON_SCOPE_EXIT { os::DestroySystemEvent(std::addressof(event)); };

                /* Wait for the accept to finish. */
                os::WaitSystemEvent(std::addressof(event));

                /* End the accept. */
                static_cast<void>(socket->AcceptResults(std::addressof(last_error), std::addressof(res), address, task_id));
            } else {
                /* Set error. */
                last_error = HTCS_EINTR;
            }

            /* Sleep, if an error occurred. */
            if (last_error != HTCS_ENONE) {
                os::SleepThread(TimeSpan::FromMilliSeconds(1));
            }

            return res;
        }

        s32 fcntl(sf::SharedPointer<tma::ISocket> socket, s32 command, s32 value, s32 &last_error) {
            s32 res;
            static_cast<void>(socket->Fcntl(std::addressof(last_error), std::addressof(res), command, value));
            return res;
        }

        s32 shutdown(sf::SharedPointer<tma::ISocket> socket, s32 how, s32 &last_error) {
            s32 res;
            static_cast<void>(socket->Shutdown(std::addressof(last_error), std::addressof(res), how));
            return res;
        }

        s32 connect(sf::SharedPointer<tma::ISocket> socket, const htcs::SockAddrHtcs *address, s32 &last_error) {
            /* Create null-terminated address. */
            htcs::SockAddrHtcs null_terminated_address;
            null_terminated_address.family = address->family;
            util::Strlcpy(null_terminated_address.peer_name.name, address->peer_name.name, PeerNameBufferLength);
            util::Strlcpy(null_terminated_address.port_name.name, address->port_name.name, PortNameBufferLength);

            s32 res;
            static_cast<void>(socket->Connect(std::addressof(last_error), std::addressof(res), null_terminated_address));
            return res;
        }

        s32 select(s32 * const read, s32 &num_read, s32 * const write, s32 &num_write, s32 * const except, s32 &num_except, htcs::TimeVal *timeout, s32 &last_error) {
            /* Determine the timeout values. */
            s64 tv_sec  = -1;
            s64 tv_usec = -1;
            if (timeout != nullptr) {
                tv_sec  = timeout->tv_sec;
                tv_usec = timeout->tv_usec;
            }

            using InArray  = sf::InMapAliasArray<s32>;
            using OutArray = sf::OutMapAliasArray<s32>;

            /* Begin the select. */
            s32 res = -1;
            u32 task_id = 0;
            sf::NativeHandle event_handle;
            if (R_SUCCEEDED(g_manager->StartSelect(std::addressof(task_id), std::addressof(event_handle), InArray(read, num_read), InArray(write, num_write), InArray(except, num_except), tv_sec, tv_usec))) {
                /* Create system event. */
                os::SystemEventType event;
                os::AttachReadableHandleToSystemEvent(std::addressof(event), event_handle.GetOsHandle(), event_handle.IsManaged(), os::EventClearMode_ManualClear);
                event_handle.Detach();

                /* When we're done, clean up the event. */
                ON_SCOPE_EXIT { os::DestroySystemEvent(std::addressof(event)); };

                /* Wait for the select to finish. */
                os::WaitSystemEvent(std::addressof(event));

                /* End the select. */
                static_cast<void>(g_manager->EndSelect(std::addressof(last_error), std::addressof(res), OutArray(read, num_read), OutArray(write, num_write), OutArray(except, num_except), task_id));
            } else {
                /* Set error. */
                last_error = HTCS_EINTR;
                os::SleepThread(TimeSpan::FromMilliSeconds(1));
            }

            return res;
        }

        namespace {

            constexpr size_t MaximumBufferSizeForSmallTransfer = 0xDFE0;

            ssize_t recvLarge(sf::SharedPointer<tma::ISocket> socket, void *buffer, size_t buffer_size, s32 flags, s32 &last_error) {
                /* Setup. */
                s64 res = -1;
                last_error = HTCS_EINTR;

                /* Start the receive. */
                u32 task_id = 0;
                sf::NativeHandle event_handle;
                if (R_SUCCEEDED(socket->StartRecv(std::addressof(task_id), std::addressof(event_handle), static_cast<s64>(buffer_size), flags))) {
                    /* Create system event. */
                    os::SystemEventType event;
                    os::AttachReadableHandleToSystemEvent(std::addressof(event), event_handle.GetOsHandle(), event_handle.IsManaged(), os::EventClearMode_ManualClear);
                    event_handle.Detach();

                    /* When we're done, clean up the event. */
                    ON_SCOPE_EXIT { os::DestroySystemEvent(std::addressof(event)); };

                    /* Wait for the receive to finish. */
                    os::WaitSystemEvent(std::addressof(event));

                    /* End the receive. */
                    static_cast<void>(socket->EndRecv(std::addressof(last_error), std::addressof(res), sf::OutAutoSelectBuffer(buffer, buffer_size), task_id));
                } else {
                    /* Set error. */
                    last_error = HTCS_EINTR;
                    os::SleepThread(TimeSpan::FromMilliSeconds(1));
                }

                return static_cast<ssize_t>(res);
            }

            ssize_t sendLarge(sf::SharedPointer<tma::ISocket> socket, const void *buffer, size_t buffer_size, s32 flags, s32 &last_error) {
                /* Setup. */
                s64 res = -1;
                last_error = HTCS_EINTR;

                /* Start the send. */
                u32 task_id = 0;
                s64 max_size = 0;
                sf::NativeHandle event_handle;
                if (R_SUCCEEDED(socket->StartSend(std::addressof(task_id), std::addressof(event_handle), std::addressof(max_size), static_cast<s64>(buffer_size), flags))) {
                    /* Create system event. */
                    os::SystemEventType event;
                    os::AttachReadableHandleToSystemEvent(std::addressof(event), event_handle.GetOsHandle(), event_handle.IsManaged(), os::EventClearMode_ManualClear);
                    event_handle.Detach();

                    /* When we're done, clean up the event. */
                    ON_SCOPE_EXIT { os::DestroySystemEvent(std::addressof(event)); };

                    /* Send all the data. */
                    bool done = false;
                    size_t sent = 0;
                    while (sent < buffer_size) {
                        /* Determine how much to send, this iteration. */
                        const u8 *cur = static_cast<const u8 *>(buffer) + sent;
                        const s64 cur_size = std::min(max_size, static_cast<s64>(buffer_size - sent));

                        /* Continue sending data. */
                        s64 cur_sent = 0;
                        bool wait = false;
                        const Result result = socket->ContinueSend(std::addressof(cur_sent), std::addressof(wait), sf::InNonSecureAutoSelectBuffer(cur, cur_size), task_id);
                        if (cur_sent <= 0 || R_FAILED(result)) {
                            done = true;
                            break;
                        }

                        /* Wait if we should. */
                        if (wait) {
                            os::WaitSystemEvent(std::addressof(event));
                            os::ClearSystemEvent(std::addressof(event));
                        }

                        /* Advance. */
                        sent += cur_sent;
                    }

                    /* Wait for the send to finish. */
                    if (!done) {
                        os::WaitSystemEvent(std::addressof(event));
                    }

                    /* End the send. */
                    static_cast<void>(socket->EndSend(std::addressof(last_error), std::addressof(res), task_id));
                } else {
                    /* Set error. */
                    last_error = HTCS_EINTR;
                    os::SleepThread(TimeSpan::FromMilliSeconds(1));
                }

                return static_cast<ssize_t>(res);
            }

        }

        ssize_t recv(sf::SharedPointer<tma::ISocket> socket, void *buffer, size_t buffer_size, s32 flags, s32 &last_error) {
            /* Determine how much to receive. */
            size_t recv_size = buffer_size;

            if ((flags & HTCS_MSG_WAITALL) == 0) {
                recv_size = std::min(MaximumBufferSizeForSmallTransfer, buffer_size);
            }

            /* Perform a large receive, if we have to. */
            if (recv_size > MaximumBufferSizeForSmallTransfer) {
                return recvLarge(socket, buffer, recv_size, flags, last_error);
            }

            /* Start the receive. */
            s64 res = -1;
            u32 task_id = 0;
            sf::NativeHandle event_handle;
            if (R_SUCCEEDED(socket->RecvStart(std::addressof(task_id), std::addressof(event_handle), static_cast<s32>(recv_size), flags))) {
                /* Create system event. */
                os::SystemEventType event;
                os::AttachReadableHandleToSystemEvent(std::addressof(event), event_handle.GetOsHandle(), event_handle.IsManaged(), os::EventClearMode_ManualClear);
                event_handle.Detach();

                /* When we're done, clean up the event. */
                ON_SCOPE_EXIT { os::DestroySystemEvent(std::addressof(event)); };

                /* Wait for the receive to finish. */
                os::WaitSystemEvent(std::addressof(event));

                /* End the receive. */
                static_cast<void>(socket->RecvResults(std::addressof(last_error), std::addressof(res), sf::OutAutoSelectBuffer(buffer, recv_size), task_id));
            } else {
                /* Set error. */
                last_error = HTCS_EINTR;
                os::SleepThread(TimeSpan::FromMilliSeconds(1));
            }

            return static_cast<ssize_t>(res);
        }

        ssize_t send(sf::SharedPointer<tma::ISocket> socket, const void *buffer, size_t buffer_size, s32 flags, s32 &last_error) {
            /* Perform a large send, if we have to. */
            if (buffer_size > MaximumBufferSizeForSmallTransfer) {
                return sendLarge(socket, buffer, buffer_size, flags, last_error);
            }

            /* Start the send. */
            s64 res = -1;
            u32 task_id = 0;
            sf::NativeHandle event_handle;
            if (R_SUCCEEDED(socket->SendStart(std::addressof(task_id), std::addressof(event_handle), sf::InNonSecureAutoSelectBuffer(buffer, buffer_size), flags))) {
                /* Create system event. */
                os::SystemEventType event;
                os::AttachReadableHandleToSystemEvent(std::addressof(event), event_handle.GetOsHandle(), event_handle.IsManaged(), os::EventClearMode_ManualClear);
                event_handle.Detach();

                /* When we're done, clean up the event. */
                ON_SCOPE_EXIT { os::DestroySystemEvent(std::addressof(event)); };

                /* Wait for the send to finish. */
                os::WaitSystemEvent(std::addressof(event));

                /* End the send. */
                static_cast<void>(socket->SendResults(std::addressof(last_error), std::addressof(res), task_id));
            } else {
                /* Set error. */
                last_error = HTCS_EINTR;
                os::SleepThread(TimeSpan::FromMilliSeconds(1));
            }

            return static_cast<ssize_t>(res);
        }

    }

}
