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
#include <vapours.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/cfg/cfg_types.hpp>

namespace ams::sm {

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
        os::ProcessId owner_process_id;
        u64 max_sessions;
        os::ProcessId mitm_process_id;
        os::ProcessId mitm_waiting_ack_process_id;
        bool is_light;
        bool mitm_waiting_ack;
    };
    static_assert(sizeof(ServiceRecord) == 0x30, "ServiceRecord definition!");

    /* For Mitm extensions. */
    struct MitmProcessInfo {
        os::ProcessId process_id;
        ncm::ProgramId program_id;
        cfg::OverrideStatus override_status;
    };
    static_assert(std::is_trivial<MitmProcessInfo>::value && sizeof(MitmProcessInfo) == 0x20, "MitmProcessInfo definition!");

}
