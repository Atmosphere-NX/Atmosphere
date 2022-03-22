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
#pragma once
#include <vapours.hpp>
#include <stratosphere/sf.hpp>

namespace ams::scs {

    enum Port {
        Port_HtcTenv,
        Port_Count,
    };

    constexpr inline int SessionCount[Port_Count] = {
        6,
    };

    constexpr inline auto MaxSessions = [] {
        auto total = 0;
        for (const auto sessions : SessionCount) {
            total += sessions;
        }
        return total;
    }();

    struct ServerOptions {
        static constexpr size_t PointerBufferSize = 0;
        static constexpr size_t MaxDomains = 6;
        static constexpr size_t MaxDomainObjects = 16;
        static constexpr bool CanDeferInvokeRequest = false;
        static constexpr bool CanManageMitmServers  = false;
    };

    class ServerManager final : public sf::hipc::ServerManager<Port_Count, ServerOptions, MaxSessions> {
        /* ... */
    };

    ServerManager *GetServerManager();
    void StartServer();

}
