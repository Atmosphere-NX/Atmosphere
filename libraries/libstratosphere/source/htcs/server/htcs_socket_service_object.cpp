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
#include "htcs_socket_service_object.hpp"
#include "htcs_service_object_allocator.hpp"
#include "../impl/htcs_manager.hpp"

namespace ams::htcs::server {

    SocketServiceObject::SocketServiceObject(ManagerServiceObject *manager, s32 desc) : m_manager(manager, true), m_desc(desc) {
        /* ... */
    }

    SocketServiceObject::~SocketServiceObject() {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Close the underlying socket. */
        s32 dummy_err, dummy_res;
        manager->Close(std::addressof(dummy_err), std::addressof(dummy_res), m_desc);
    }

    Result SocketServiceObject::Close(sf::Out<s32> out_err, sf::Out<s32> out_res) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Close the underlying socket. */
        manager->Close(out_err.GetPointer(), out_res.GetPointer(), m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::Connect(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Perform the connect. */
        manager->Connect(out_err.GetPointer(), out_res.GetPointer(), address, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::Bind(sf::Out<s32> out_err, sf::Out<s32> out_res, const htcs::SockAddrHtcs &address) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Perform the bind. */
        manager->Bind(out_err.GetPointer(), out_res.GetPointer(), address, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::Listen(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 backlog_count) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Perform the listen. */
        manager->Listen(out_err.GetPointer(), out_res.GetPointer(), backlog_count, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::Recv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, s32 flags) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Perform the recv. */
        manager->Recv(out_err.GetPointer(), out_size.GetPointer(), reinterpret_cast<char *>(buffer.GetPointer()), buffer.GetSize(), flags, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::Send(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::InAutoSelectBuffer &buffer, s32 flags) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Perform the send. */
        manager->Send(out_err.GetPointer(), out_size.GetPointer(), reinterpret_cast<const char *>(buffer.GetPointer()), buffer.GetSize(), flags, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::Shutdown(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 how) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Perform the shutdown. */
        manager->Shutdown(out_err.GetPointer(), out_res.GetPointer(), how, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::Fcntl(sf::Out<s32> out_err, sf::Out<s32> out_res, s32 command, s32 value) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Perform the fcntl. */
        manager->Fcntl(out_err.GetPointer(), out_res.GetPointer(), command, value, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::AcceptStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Start the accept. */
        os::NativeHandle event_handle;
        R_TRY(manager->AcceptStart(out_task_id.GetPointer(), std::addressof(event_handle), m_desc));

        /* Set the output event handle. */
        out_event.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result SocketServiceObject::AcceptResults(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, sf::Out<htcs::SockAddrHtcs> out_address, u32 task_id) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Get the accept results. */
        s32 desc = -1;
        manager->AcceptResults(out_err.GetPointer(), std::addressof(desc), out_address.GetPointer(), task_id, m_desc);

        /* If an error occurred, we're done. */
        R_SUCCEED_IF(*out_err != 0);

        /* Create a new socket object. */
        *out = ServiceObjectFactory::CreateSharedEmplaced<tma::ISocket, SocketServiceObject>(m_manager.Get(), desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::RecvStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 mem_size, s32 flags) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Start the recv. */
        os::NativeHandle event_handle;
        R_TRY(manager->RecvStart(out_task_id.GetPointer(), std::addressof(event_handle), mem_size, m_desc, flags));

        /* Set the output event handle. */
        out_event.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result SocketServiceObject::RecvResults(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Get the recv results. */
        manager->RecvResults(out_err.GetPointer(), out_size.GetPointer(), reinterpret_cast<char *>(buffer.GetPointer()), buffer.GetSize(), task_id, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::RecvLargeStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s32 unaligned_size_start, s32 unaligned_size_end, s64 aligned_size, sf::CopyHandle &&mem_handle, s32 flags) {
        /* Check that the transfer memory size is okay. */
        R_UNLESS(util::IsIntValueRepresentable<size_t>(aligned_size), htcs::ResultInvalidSize());

        /* Attach the transfer memory. */
        os::TransferMemoryType tmem;
        os::AttachTransferMemory(std::addressof(tmem), static_cast<size_t>(aligned_size), mem_handle.GetOsHandle(), mem_handle.IsManaged());
        mem_handle.Detach();
        ON_SCOPE_EXIT { os::DestroyTransferMemory(std::addressof(tmem)); };

        /* Map the transfer memory. */
        void *address;
        R_TRY(os::MapTransferMemory(std::addressof(address), std::addressof(tmem), os::MemoryPermission_None));
        ON_SCOPE_EXIT { os::UnmapTransferMemory(std::addressof(tmem)); };

        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Start the large receive. */
        os::NativeHandle event_handle;
        R_TRY(manager->RecvStart(out_task_id.GetPointer(), std::addressof(event_handle), unaligned_size_start + aligned_size + unaligned_size_end, m_desc, flags));

        /* Set the output event handle. */
        out_event.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result SocketServiceObject::SendStartOld(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InAutoSelectBuffer &buffer, s32 flags) {
        return this->SendStart(out_task_id, out_event, sf::InNonSecureAutoSelectBuffer(buffer.GetPointer(), buffer.GetSize()), flags);
    }

    Result SocketServiceObject::SendLargeStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InAutoSelectBuffer &start_buffer, const sf::InAutoSelectBuffer &end_buffer, sf::CopyHandle &&mem_handle, s64 aligned_size, s32 flags) {
        /* Check that the sizes are okay. */
        R_UNLESS(util::IsIntValueRepresentable<s64>(start_buffer.GetSize()), htcs::ResultInvalidSize());
        R_UNLESS(util::IsIntValueRepresentable<s64>(end_buffer.GetSize()),   htcs::ResultInvalidSize());
        R_UNLESS(util::IsIntValueRepresentable<size_t>(aligned_size),        htcs::ResultInvalidSize());

        /* Attach the transfer memory. */
        os::TransferMemoryType tmem;
        os::AttachTransferMemory(std::addressof(tmem), static_cast<size_t>(aligned_size), mem_handle.GetOsHandle(), mem_handle.IsManaged());
        mem_handle.Detach();
        ON_SCOPE_EXIT { os::DestroyTransferMemory(std::addressof(tmem)); };

        /* Map the transfer memory. */
        void *address;
        R_TRY(os::MapTransferMemory(std::addressof(address), std::addressof(tmem), os::MemoryPermission_None));
        ON_SCOPE_EXIT { os::UnmapTransferMemory(std::addressof(tmem)); };

        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Start the large send. */
        constexpr auto NumBuffers = 3;
        const char *pointers[NumBuffers] = { reinterpret_cast<const char *>(start_buffer.GetPointer()), static_cast<const char *>(address), reinterpret_cast<const char *>(end_buffer.GetPointer()) };
        s64 sizes[NumBuffers] = { static_cast<s64>(start_buffer.GetSize()), aligned_size, static_cast<s64>(end_buffer.GetSize()) };

        os::NativeHandle event_handle;
        R_TRY(manager->SendLargeStart(out_task_id.GetPointer(), std::addressof(event_handle), pointers, sizes, NumBuffers, m_desc, flags));

        /* Set the output event handle. */
        out_event.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result SocketServiceObject::SendResults(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Get the send results. */
        manager->SendResults(out_err.GetPointer(), out_size.GetPointer(), task_id, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::StartSend(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, sf::Out<s64> out_max_size, s64 size, s32 flags) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Start the send. */
        os::NativeHandle event_handle;
        R_TRY(manager->StartSend(out_task_id.GetPointer(), std::addressof(event_handle), m_desc, size, flags));

        /* Set the output max size to the size. */
        *out_max_size = size;

        /* Set the output event handle. */
        out_event.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result SocketServiceObject::ContinueSendOld(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InAutoSelectBuffer &buffer, u32 task_id) {
        return this->ContinueSend(out_size, out_wait, sf::InNonSecureAutoSelectBuffer(buffer.GetPointer(), buffer.GetSize()), task_id);
    }

    Result SocketServiceObject::EndSend(sf::Out<s32> out_err, sf::Out<s64> out_size, u32 task_id) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* End the send. */
        manager->EndSend(out_err.GetPointer(), out_size.GetPointer(), task_id, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::StartRecv(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, s64 size, s32 flags) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Start the recv. */
        os::NativeHandle event_handle;
        R_TRY(manager->StartRecv(out_task_id.GetPointer(), std::addressof(event_handle), size, m_desc, flags));

        /* Set the output event handle. */
        out_event.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result SocketServiceObject::EndRecv(sf::Out<s32> out_err, sf::Out<s64> out_size, const sf::OutAutoSelectBuffer &buffer, u32 task_id) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* End the recv. */
        manager->EndRecv(out_err.GetPointer(), out_size.GetPointer(), reinterpret_cast<char *>(buffer.GetPointer()), buffer.GetSize(), task_id, m_desc);

        return ResultSuccess();
    }

    Result SocketServiceObject::SendStart(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InNonSecureAutoSelectBuffer &buffer, s32 flags) {
        /* Check that the sizes are okay. */
        R_UNLESS(util::IsIntValueRepresentable<s64>(buffer.GetSize()), htcs::ResultInvalidSize());

        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Start the send. */
        os::NativeHandle event_handle;
        R_TRY(manager->SendStart(out_task_id.GetPointer(), std::addressof(event_handle), reinterpret_cast<const char *>(buffer.GetPointer()), buffer.GetSize(), m_desc, flags));

        /* Set the output event handle. */
        out_event.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result SocketServiceObject::ContinueSend(sf::Out<s64> out_size, sf::Out<bool> out_wait, const sf::InNonSecureAutoSelectBuffer &buffer, u32 task_id) {
        /* Check that the sizes are okay. */
        R_UNLESS(util::IsIntValueRepresentable<s64>(buffer.GetSize()), htcs::ResultInvalidSize());

        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Continue the send. */
        R_TRY(manager->ContinueSend(out_size.GetPointer(), reinterpret_cast<const char *>(buffer.GetPointer()), buffer.GetSize(), task_id, m_desc));

        /* We aren't doing a waiting send. */
        *out_wait = false;
        return ResultSuccess();
    }

    Result SocketServiceObject::GetPrimitive(sf::Out<s32> out) {
        /* Get our descriptor. */
        *out = m_desc;
        return ResultSuccess();
    }

}
