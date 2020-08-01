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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_spin_lock.hpp>

namespace ams::kern {

    #if defined(MESOSPHERE_BUILD_FOR_TRACING)
        constexpr inline bool IsKTraceEnabled = true;
    #else
        constexpr inline bool IsKTraceEnabled = false;
    #endif

    constexpr inline size_t KTraceBufferSize = IsKTraceEnabled ? 16_MB : 0;

    static_assert(IsKTraceEnabled || !IsKTraceEnabled);

    class KTrace {
        private:
            static bool s_is_active;
        public:
            static void Initialize(KVirtualAddress address, size_t size);
            static void Start();
            static void Stop();

            static ALWAYS_INLINE bool IsActive() { return s_is_active; }
    };

}

#define MESOSPHERE_KTRACE_RESUME()                    \
    ({                                                \
        if constexpr (::ams::kern::IsKTraceEnabled) { \
            ::ams::kern::KTrace::Start();             \
        }                                             \
    })

#define MESOSPHERE_KTRACE_PAUSE()                     \
    ({                                                \
        if constexpr (::ams::kern::IsKTraceEnabled) { \
            ::ams::kern::KTrace::Stop();              \
        }                                             \
    })
