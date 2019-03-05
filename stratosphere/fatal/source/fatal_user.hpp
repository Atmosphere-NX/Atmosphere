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
 
#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "fatal_types.hpp"

enum UserCmd {
    User_Cmd_ThrowFatal = 0,
    User_Cmd_ThrowFatalWithPolicy = 1,
    User_Cmd_ThrowFatalWithCpuContext = 2,
};

class UserService final : public IServiceObject {
    private:    
        /* Actual commands. */
        Result ThrowFatal(u32 error, PidDescriptor pid_desc);
        Result ThrowFatalWithPolicy(u32 error, PidDescriptor pid_desc, FatalType policy);
        Result ThrowFatalWithCpuContext(u32 error, PidDescriptor pid_desc, FatalType policy, InBuffer<u8> _ctx);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<User_Cmd_ThrowFatal, &UserService::ThrowFatal>(),
            MakeServiceCommandMeta<User_Cmd_ThrowFatalWithPolicy, &UserService::ThrowFatalWithPolicy>(),
            MakeServiceCommandMeta<User_Cmd_ThrowFatalWithCpuContext, &UserService::ThrowFatalWithCpuContext>(),
        };
};
