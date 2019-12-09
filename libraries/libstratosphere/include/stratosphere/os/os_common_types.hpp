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
#include <vapours.hpp>

namespace ams::os {

    enum class TriBool {
        False     = 0,
        True      = 1,
        Undefined = 2,
    };

    enum class MessageQueueWaitKind {
        ForNotEmpty,
        ForNotFull,
    };

    struct ProcessId {
        u64 value;

        inline constexpr explicit operator u64() const {
            return this->value;
        }

        /* Invalid Process ID. */
        static const ProcessId Invalid;
    };

    inline constexpr const ProcessId ProcessId::Invalid = {static_cast<u64>(-1ull)};

    inline constexpr const ProcessId InvalidProcessId = ProcessId::Invalid;

    NX_INLINE Result TryGetProcessId(os::ProcessId *out, ::Handle process_handle) {
        return svcGetProcessId(&out->value, process_handle);
    }

    NX_INLINE os::ProcessId GetProcessId(::Handle process_handle) {
        os::ProcessId process_id;
        R_ASSERT(TryGetProcessId(&process_id, process_handle));
        return process_id;
    }

    inline constexpr bool operator==(const ProcessId &lhs, const ProcessId &rhs) {
        return lhs.value == rhs.value;
    }

    inline constexpr bool operator!=(const ProcessId &lhs, const ProcessId &rhs) {
        return lhs.value != rhs.value;
    }

    inline constexpr bool operator<(const ProcessId &lhs, const ProcessId &rhs) {
        return lhs.value < rhs.value;
    }

    inline constexpr bool operator<=(const ProcessId &lhs, const ProcessId &rhs) {
        return lhs.value <= rhs.value;
    }

    inline constexpr bool operator>(const ProcessId &lhs, const ProcessId &rhs) {
        return lhs.value > rhs.value;
    }

    inline constexpr bool operator>=(const ProcessId &lhs, const ProcessId &rhs) {
        return lhs.value >= rhs.value;
    }

}
