/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace sts::fatal::srv {

    class UserService final : public IServiceObject {
        private:
            enum class CommandId {
                ThrowFatal               = 0,
                ThrowFatalWithPolicy     = 1,
                ThrowFatalWithCpuContext = 2,
            };
        private:
            /* Actual commands. */
            Result ThrowFatal(u32 error, PidDescriptor pid_desc);
            Result ThrowFatalWithPolicy(u32 error, PidDescriptor pid_desc, FatalType policy);
            Result ThrowFatalWithCpuContext(u32 error, PidDescriptor pid_desc, FatalType policy, InBuffer<u8> _ctx);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(UserService, ThrowFatal),
                MAKE_SERVICE_COMMAND_META(UserService, ThrowFatalWithPolicy),
                MAKE_SERVICE_COMMAND_META(UserService, ThrowFatalWithCpuContext),
            };
    };

    class PrivateService final : public IServiceObject {
        private:
            enum class CommandId {
                GetFatalEvent = 0,
            };
        private:
            /* Actual commands. */
            Result GetFatalEvent(Out<CopiedHandle> out_h);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(PrivateService, GetFatalEvent),
            };
    };

}

