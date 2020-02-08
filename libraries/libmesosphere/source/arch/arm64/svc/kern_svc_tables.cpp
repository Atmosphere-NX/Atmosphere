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
#ifdef MESOSPHERE_USE_STUBBED_SVC_TABLES
#include <mesosphere/kern_debug_log.hpp>
#endif

#include <mesosphere/svc/kern_svc_tables.hpp>
#include <vapours/svc/svc_codegen.hpp>

namespace ams::kern::svc {

    namespace {

        #ifndef MESOSPHERE_USE_STUBBED_SVC_TABLES
            #define DECLARE_SVC_STRUCT(ID, RETURN_TYPE, NAME, ...)                                                                        \
                class NAME {                                                                                                              \
                    private:                                                                                                              \
                        using Impl = ::ams::svc::codegen::KernelSvcWrapper<::ams::kern::svc::NAME##64, ::ams::kern::svc::NAME##64From32>; \
                    public:                                                                                                               \
                        static NOINLINE void Call64() { return Impl::Call64(); }                                                          \
                        static NOINLINE void Call64From32() { return Impl::Call64From32(); }                                              \
                };
        #else
            #define DECLARE_SVC_STRUCT(ID, RETURN_TYPE, NAME, ...)                                                                        \
                class NAME {                                                                                                              \
                    public:                                                                                                               \
                        static NOINLINE void Call64()       { MESOSPHERE_PANIC("Stubbed Svc"#NAME"64 was called"); }                      \
                        static NOINLINE void Call64From32() { MESOSPHERE_PANIC("Stubbed Svc"#NAME"64From32 was called"); }                \
                };
        #endif


        /* Set omit-frame-pointer to prevent GCC from emitting MOV X29, SP instructions. */
        #pragma GCC push_options
        #pragma GCC optimize ("omit-frame-pointer")

            AMS_SVC_FOREACH_KERN_DEFINITION(DECLARE_SVC_STRUCT, _)

        #pragma GCC pop_options

    }

    const std::array<SvcTableEntry, NumSupervisorCalls> SvcTable64From32 = [] {
        std::array<SvcTableEntry, NumSupervisorCalls> table = {};

        #define AMS_KERN_SVC_SET_TABLE_ENTRY(ID, RETURN_TYPE, NAME, ...) \
            table[ID] = NAME::Call64From32;
        AMS_SVC_FOREACH_KERN_DEFINITION(AMS_KERN_SVC_SET_TABLE_ENTRY, _)
        #undef AMS_KERN_SVC_SET_TABLE_ENTRY

        return table;
    }();

    const std::array<SvcTableEntry, NumSupervisorCalls> SvcTable64 = [] {
        std::array<SvcTableEntry, NumSupervisorCalls> table = {};

        #define AMS_KERN_SVC_SET_TABLE_ENTRY(ID, RETURN_TYPE, NAME, ...) \
            table[ID] = NAME::Call64;
        AMS_SVC_FOREACH_KERN_DEFINITION(AMS_KERN_SVC_SET_TABLE_ENTRY, _)
        #undef AMS_KERN_SVC_SET_TABLE_ENTRY

        return table;
    }();

}
