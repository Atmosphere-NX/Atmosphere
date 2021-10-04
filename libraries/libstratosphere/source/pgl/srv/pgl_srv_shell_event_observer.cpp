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
#include "pgl_srv_shell_event_observer.hpp"
#include "pgl_srv_shell.hpp"

namespace ams::pgl::srv {

    ShellEventObserverImpl::ShellEventObserverImpl() : message_queue(queue_buffer, QueueCapacity), event(os::EventClearMode_AutoClear, true) {
        this->heap_handle = lmem::CreateUnitHeap(this->event_info_data, sizeof(this->event_info_data), sizeof(this->event_info_data[0]), lmem::CreateOption_ThreadSafe, 8, GetPointer(this->heap_head));

        RegisterShellEventObserver(util::ConstructAt(this->holder, this));
    }

    ShellEventObserverImpl::~ShellEventObserverImpl() {
        UnregisterShellEventObserver(GetPointer(this->holder));
        util::DestroyAt(this->holder);
    }

    Result ShellEventObserverImpl::PopEventInfo(pm::ProcessEventInfo *out) {
        /* Receive an info from the queue. */
        uintptr_t info_address;
        R_UNLESS(this->message_queue.TryReceive(std::addressof(info_address)), pgl::ResultNotAvailable());
        pm::ProcessEventInfo *info = reinterpret_cast<pm::ProcessEventInfo *>(info_address);

        /* Set the output. */
        *out = *info;

        /* Free the received info. */
        lmem::FreeToUnitHeap(this->heap_handle, info);

        return ResultSuccess();
    }

    void ShellEventObserverImpl::Notify(const pm::ProcessEventInfo &info) {
        /* Allocate a new info. */
        auto allocated = reinterpret_cast<pm::ProcessEventInfo *>(lmem::AllocateFromUnitHeap(this->heap_handle));
        if (!allocated) {
            return;
        }

        /* Set it to the notification. */
        *allocated = info;

        /* Try to send it. */
        if (!this->message_queue.TrySend(reinterpret_cast<uintptr_t>(allocated))) {
            lmem::FreeToUnitHeap(this->heap_handle, allocated);
            return;
        }

        /* Notify that we have a new info available. */
        this->event.Signal();
    }

    Result ShellEventObserverCmif::GetProcessEventHandle(ams::sf::OutCopyHandle out) {
        out.SetValue(this->GetEvent().GetReadableHandle());
        return ResultSuccess();
    }

    Result ShellEventObserverCmif::GetProcessEventInfo(ams::sf::Out<pm::ProcessEventInfo> out) {
        return this->PopEventInfo(out.GetPointer());
    }

    Result ShellEventObserverTipc::GetProcessEventHandle(ams::tipc::OutCopyHandle out) {
        out.SetValue(this->GetEvent().GetReadableHandle());
        return ResultSuccess();
    }

    Result ShellEventObserverTipc::GetProcessEventInfo(ams::tipc::Out<pm::ProcessEventInfo> out) {
        return this->PopEventInfo(out.GetPointer());
    }

}
