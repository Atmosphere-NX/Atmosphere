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
        public:
            enum Type {
                Type_ThreadSwitch   =  1,

                Type_SvcEntry0      =  3,
                Type_SvcEntry1      =  4,
                Type_SvcExit0       =  5,
                Type_SvcExit1       =  6,
                Type_Interrupt      =  7,

                Type_ScheduleUpdate = 11,

                Type_CoreMigration  = 14,
            };
        private:
            static bool s_is_active;
        public:
            static void Initialize(KVirtualAddress address, size_t size);
            static void Start();
            static void Stop();

            static void PushRecord(u8 type, u64 param0 = 0, u64 param1 = 0, u64 param2 = 0, u64 param3 = 0, u64 param4 = 0, u64 param5 = 0);

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

#define MESOSPHERE_KTRACE_PUSH_RECORD(TYPE, ...)                       \
    ({                                                                 \
        if constexpr (::ams::kern::IsKTraceEnabled) {                  \
            if (::ams::kern::KTrace::IsActive()) {                     \
                ::ams::kern::KTrace::PushRecord(TYPE, ## __VA_ARGS__); \
            }                                                          \
        }                                                              \
    })

#define MESOSPHERE_KTRACE_THREAD_SWITCH(NEXT) \
    MESOSPHERE_KTRACE_PUSH_RECORD(::ams::kern::KTrace::Type_ThreadSwitch, (NEXT)->GetId())

#define MESOSPHERE_KTRACE_SVC_ENTRY(SVC_ID, PARAM0, PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7)                           \
    ({                                                                                                                                \
        if constexpr (::ams::kern::IsKTraceEnabled) {                                                                                 \
            if (::ams::kern::KTrace::IsActive()) {                                                                                    \
                ::ams::kern::KTrace::PushRecord(::ams::kern::KTrace::Type_SvcEntry0, SVC_ID, PARAM0, PARAM1, PARAM2, PARAM3, PARAM4); \
                ::ams::kern::KTrace::PushRecord(::ams::kern::KTrace::Type_SvcEntry1, PARAM5, PARAM6, PARAM7);                         \
            }                                                                                                                         \
        }                                                                                                                             \
    })

#define MESOSPHERE_KTRACE_SVC_EXIT(SVC_ID, PARAM0, PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7)                           \
    ({                                                                                                                               \
        if constexpr (::ams::kern::IsKTraceEnabled) {                                                                                \
            if (::ams::kern::KTrace::IsActive()) {                                                                                   \
                ::ams::kern::KTrace::PushRecord(::ams::kern::KTrace::Type_SvcExit0, SVC_ID, PARAM0, PARAM1, PARAM2, PARAM3, PARAM4); \
                ::ams::kern::KTrace::PushRecord(::ams::kern::KTrace::Type_SvcExit1, PARAM5, PARAM6, PARAM7);                         \
            }                                                                                                                        \
        }                                                                                                                            \
    })

#define MESOSPHERE_KTRACE_INTERRUPT(ID) \
    MESOSPHERE_KTRACE_PUSH_RECORD(::ams::kern::KTrace::Type_Interrupt, ID)

#define MESOSPHERE_KTRACE_SCHEDULE_UPDATE(CORE, PREV, NEXT) \
    MESOSPHERE_KTRACE_PUSH_RECORD(::ams::kern::KTrace::Type_ScheduleUpdate, CORE, (PREV)->GetId(), (NEXT)->GetId())

#define MESOSPHERE_KTRACE_CORE_MIGRATION(THREAD_ID, PREV, NEXT, REASON) \
    MESOSPHERE_KTRACE_PUSH_RECORD(::ams::kern::KTrace::Type_CoreMigration,  THREAD_ID, PREV, NEXT, REASON)
