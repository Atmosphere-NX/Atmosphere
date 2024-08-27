/*
 * Copyright (C) Switch-OC-Suite
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

#ifdef ATMOSPHERE_IS_STRATOSPHERE
    #include <stratosphere.hpp>
    #include <vapours/results/results_common.hpp>
    #define LOGGING(fmt, ...) ((void)0)
    #define CRASH(msg, ...) { ams::diag::AbortImpl(msg, __PRETTY_FUNCTION__, "", 0); __builtin_unreachable(); }
#else
    #include "oc_test.hpp"
#endif

#include "customize.hpp"

#define PATCH_OFFSET(offset, value) \
    static_assert(sizeof(__typeof__(offset)) <= sizeof(u64)); \
    *(offset) = value;

namespace ams::ldr {
    R_DEFINE_ERROR_RESULT(OutOfRange,               1000);
    R_DEFINE_ERROR_RESULT(InvalidMemPllmEntry,      1001);
    R_DEFINE_ERROR_RESULT(InvalidMtcMagic,          1002);
    R_DEFINE_ERROR_RESULT(InvalidMtcTable,          1003);
    R_DEFINE_ERROR_RESULT(InvalidDvbTable,          1004);
    R_DEFINE_ERROR_RESULT(InvalidCpuFreqVddEntry,   1005);
    R_DEFINE_ERROR_RESULT(InvalidCpuDvfs,           1006);
    R_DEFINE_ERROR_RESULT(InvalidCpuMinVolt,        1007);
    R_DEFINE_ERROR_RESULT(InvalidGpuDvfs,           1008);
    R_DEFINE_ERROR_RESULT(InvalidGpuFreqMaxPattern, 1009);
    R_DEFINE_ERROR_RESULT(InvalidGpuPllEntry,       1010);
    R_DEFINE_ERROR_RESULT(InvalidRegulatorEntry,    1011);
    R_DEFINE_ERROR_RESULT(UninitializedPatcher,     1012);
    R_DEFINE_ERROR_RESULT(UnsuccessfulPatcher,      1013);
    R_DEFINE_ERROR_RESULT(SafetyCheckFailure,       1014);
}

namespace ams::ldr::oc {
    template<typename Pointer>
    struct PatcherEntry {
        using patternFn = bool(*)(Pointer* ptr);
        using patcherFn = Result(*)(Pointer* ptr);

        const char* description;
        patcherFn   patcher_fn = nullptr;
        size_t      maximum_patched_count = 0;
        patternFn   pattern_search_fn = nullptr;
        Pointer     value_search;

        size_t      patched_count = 0;

        Result Apply(Pointer* ptr) {
            Result res = patcher_fn(ptr);
            if (R_SUCCEEDED(res))
                patched_count++;

            return res;
        }

        Result SearchAndApply(Pointer* ptr) {
            bool searchOk = false;
            if (pattern_search_fn) {
                if (pattern_search_fn(ptr)) searchOk = true;
            } else {
                if (value_search == *(ptr)) searchOk = true;
            }

            if (searchOk)
                return Apply(ptr);

            R_THROW(ldr::ResultUnsuccessfulPatcher());
        }

        Result CheckResult() {
            #ifndef ATMOSPHERE_IS_STRATOSPHERE
            R_UNLESS(patched_count > 0, ldr::ResultUnsuccessfulPatcher());
            #endif

            if (maximum_patched_count)
                R_UNLESS(patched_count <= maximum_patched_count, ldr::ResultUnsuccessfulPatcher());

            R_SUCCEED();
        }
    };
}
