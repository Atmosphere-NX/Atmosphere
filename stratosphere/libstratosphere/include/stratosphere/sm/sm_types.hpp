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
#include <algorithm>
#include <cstring>
#include <switch.h>
#include "../defines.hpp"
#include "../results.hpp"
#include "../os.hpp"

namespace sts::sm {

    struct ServiceName {
        static constexpr size_t MaxLength = 8;

        char name[MaxLength];

        static constexpr ServiceName Encode(const char *name, size_t name_size) {
            ServiceName out{};

            for (size_t i = 0; i < MaxLength; i++) {
                if (i < name_size) {
                    out.name[i] = name[i];
                } else {
                    out.name[i] = 0;
                }
            }

            return out;
        }

        static constexpr ServiceName Encode(const char *name) {
            return Encode(name, std::strlen(name));
        }
    };
    static constexpr ServiceName InvalidServiceName = ServiceName::Encode("");
    static_assert(alignof(ServiceName) == 1, "ServiceName definition!");

    inline bool operator==(const ServiceName &lhs, const ServiceName &rhs) {
        return std::memcmp(&lhs, &rhs, sizeof(ServiceName)) == 0;
    }

    inline bool operator!=(const ServiceName &lhs, const ServiceName &rhs) {
        return !(lhs == rhs);
    }

    /* For Debug Monitor extensions. */
    struct ServiceRecord {
        ServiceName service;
        os::ProcessId owner_pid;
        u64 max_sessions;
        os::ProcessId mitm_pid;
        os::ProcessId mitm_waiting_ack_pid;
        bool is_light;
        bool mitm_waiting_ack;
    };
    static_assert(sizeof(ServiceRecord) == 0x30, "ServiceRecord definition!");

}
