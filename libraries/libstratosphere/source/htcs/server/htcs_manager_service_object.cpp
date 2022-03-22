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
#include "htcs_manager_service_object.hpp"
#include "htcs_socket_service_object.hpp"
#include "htcs_service_object_allocator.hpp"
#include "../impl/htcs_manager.hpp"
#include "../impl/htcs_impl.hpp"

namespace ams::htcs::server {

    namespace {

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    ServiceObjectAllocator::Initialize(lmem::CreateOption_ThreadSafe);
                }
        } g_static_allocator_initializer;

    }

    Result ManagerServiceObject::GetPeerNameAny(sf::Out<htcs::HtcsPeerName> out) {
        *out = impl::GetPeerNameAny();
        return ResultSuccess();
    }

    Result ManagerServiceObject::GetDefaultHostName(sf::Out<htcs::HtcsPeerName> out) {
        *out = impl::GetDefaultHostName();
        return ResultSuccess();
    }

    Result ManagerServiceObject::CreateSocketOld(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out) {
        return this->CreateSocket(out_err, out, false);
    }

    Result ManagerServiceObject::CreateSocket(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, bool enable_disconnection_emulation) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Create a new socket. */
        s32 desc;
        manager->Socket(out_err.GetPointer(), std::addressof(desc), enable_disconnection_emulation);

        /* If an error occurred, we're done. */
        R_SUCCEED_IF(*out_err != 0);

        /* Create a new socket object. */
        *out = ServiceObjectFactory::CreateSharedEmplaced<tma::ISocket, SocketServiceObject>(this, desc);

        return ResultSuccess();
    }

    Result ManagerServiceObject::RegisterProcessId(const sf::ClientProcessId &client_pid) {
        /* NOTE: Nintendo does nothing here. */
        AMS_UNUSED(client_pid);
        return ResultSuccess();
    }

    Result ManagerServiceObject::MonitorManager(const sf::ClientProcessId &client_pid) {
        /* NOTE: Nintendo does nothing here. */
        AMS_UNUSED(client_pid);
        return ResultSuccess();
    }

    Result ManagerServiceObject::StartSelect(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InMapAliasArray<s32> &read_handles, const sf::InMapAliasArray<s32> &write_handles, const sf::InMapAliasArray<s32> &exception_handles, s64 tv_sec, s64 tv_usec) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* Start the select. */
        os::NativeHandle event_handle;
        R_TRY(manager->StartSelect(out_task_id.GetPointer(), std::addressof(event_handle), read_handles.ToSpan(), write_handles.ToSpan(), exception_handles.ToSpan(), tv_sec, tv_usec));

        /* Set the output event handle. */
        out_event.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result ManagerServiceObject::EndSelect(sf::Out<s32> out_err, sf::Out<s32> out_count, const sf::OutMapAliasArray<s32> &read_handles, const sf::OutMapAliasArray<s32> &write_handles, const sf::OutMapAliasArray<s32> &exception_handles, u32 task_id) {
        /* Get the htcs manager. */
        auto *manager = impl::HtcsManagerHolder::GetHtcsManager();

        /* End the select. */
        return manager->EndSelect(out_err.GetPointer(), out_count.GetPointer(), read_handles.ToSpan(), write_handles.ToSpan(), exception_handles.ToSpan(), task_id);
    }

}
