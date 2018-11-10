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

Result UserService::ThrowFatalImpl(u32 error, u64 pid, FatalType policy, FatalCpuContext *context) {
    /* TODO */
    return 0;
}


Result UserService::ThrowFatal(u32 error, PidDescriptor pid_desc) {
    FatalCpuContext ctx = {0};
    return ThrowFatalImpl(error, pid_desc.pid, FatalType_ErrorReportAndErrorScreen, &ctx);
}

Result UserService::ThrowFatalWithPolicy(u32 error, PidDescriptor pid_desc, FatalType policy) {
    FatalCpuContext ctx = {0};
    return ThrowFatalImpl(error, pid_desc.pid, policy, &ctx);
}

Result UserService::ThrowFatalWithCpuContext(u32 error, PidDescriptor pid_desc, FatalType policy, InBuffer<FatalCpuContext> _ctx) {
    /* Require exactly one context passed in. */
    if (_ctx.num_elements != 1) {
        return 0xF601;
    }
    
    return ThrowFatalImpl(error, pid_desc.pid, policy, _ctx.buffer);
}