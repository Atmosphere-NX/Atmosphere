/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include "fatal_user.hpp"
#include "fatal_throw.hpp"
#include "fatal_event_manager.hpp"
#include "fatal_task.hpp"

Result UserService::ThrowFatal(u32 error, PidDescriptor pid_desc) {
    return ThrowFatalImpl(error, pid_desc.pid, FatalType_ErrorReportAndErrorScreen, nullptr);
}

Result UserService::ThrowFatalWithPolicy(u32 error, PidDescriptor pid_desc, FatalType policy) {
    return ThrowFatalImpl(error, pid_desc.pid, policy, nullptr);
}

Result UserService::ThrowFatalWithCpuContext(u32 error, PidDescriptor pid_desc, FatalType policy, InBuffer<u8> _ctx) {
    if (_ctx.num_elements < sizeof(FatalCpuContext)) {
        return ThrowFatalImpl(error, pid_desc.pid, policy, nullptr);
    } else {
        return ThrowFatalImpl(error, pid_desc.pid, policy, reinterpret_cast<FatalCpuContext *>(_ctx.buffer));
    }
}