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
#include <stratosphere.hpp>

namespace ams::fatal::srv {

    class UserService final : public sf::IServiceObject {
        private:
            enum class CommandId {
                ThrowFatal               = 0,
                ThrowFatalWithPolicy     = 1,
                ThrowFatalWithCpuContext = 2,
            };
        private:
            /* Actual commands. */
            Result ThrowFatal(Result error, const sf::ClientProcessId &client_pid);
            Result ThrowFatalWithPolicy(Result error, const sf::ClientProcessId &client_pid, FatalPolicy policy);
            Result ThrowFatalWithCpuContext(Result error, const sf::ClientProcessId &client_pid, FatalPolicy policy, const CpuContext &cpu_ctx);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ThrowFatal),
                MAKE_SERVICE_COMMAND_META(ThrowFatalWithPolicy),
                MAKE_SERVICE_COMMAND_META(ThrowFatalWithCpuContext),
            };
    };

    class PrivateService final : public sf::IServiceObject {
        private:
            enum class CommandId {
                GetFatalEvent = 0,
            };
        private:
            /* Actual commands. */
            Result GetFatalEvent(sf::OutCopyHandle out_h);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetFatalEvent),
            };
    };

}

