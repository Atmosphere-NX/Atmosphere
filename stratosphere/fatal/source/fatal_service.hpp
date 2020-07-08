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

    class Service final {
        public:
            Result ThrowFatal(Result error, const sf::ClientProcessId &client_pid);
            Result ThrowFatalWithPolicy(Result error, const sf::ClientProcessId &client_pid, FatalPolicy policy);
            Result ThrowFatalWithCpuContext(Result error, const sf::ClientProcessId &client_pid, FatalPolicy policy, const CpuContext &cpu_ctx);
    };
    static_assert(fatal::impl::IsIService<Service>);

    class PrivateService final {
        public:
            Result GetFatalEvent(sf::OutCopyHandle out_h);
    };
    static_assert(fatal::impl::IsIPrivateService<PrivateService>);

}

