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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/results.hpp>

namespace ams::svc {

    /* TODO: C++ style handle? */
#ifdef ATMOSPHERE_IS_STRATOSPHERE
    using Handle = ::Handle;
#else
    using Handle = u32;
#endif

    enum {
        HandleWaitMask = (1u << 30),
    };

    constexpr inline s32 ArgumentHandleCountMax = 0x40;

    constexpr inline s64 WaitInfinite = -1;

    enum PseudoHandle : Handle {
        CurrentThread  = 0xFFFF8000,
        CurrentProcess = 0xFFFF8001,
    };

    constexpr inline Handle InvalidHandle = Handle(0);

    constexpr ALWAYS_INLINE bool operator==(const Handle &lhs, const PseudoHandle &rhs) {
        return static_cast<Handle>(lhs) == static_cast<Handle>(rhs);
    }

    constexpr ALWAYS_INLINE bool operator==(const PseudoHandle &lhs, const Handle &rhs) {
        return static_cast<Handle>(lhs) == static_cast<Handle>(rhs);
    }

    constexpr ALWAYS_INLINE bool operator!=(const Handle &lhs, const PseudoHandle &rhs) {
        return !(lhs == rhs);
    }

    constexpr ALWAYS_INLINE bool operator!=(const PseudoHandle &lhs, const Handle &rhs) {
        return !(lhs == rhs);
    }

    constexpr ALWAYS_INLINE bool IsPseudoHandle(const Handle &handle) {
        return handle == PseudoHandle::CurrentProcess || handle == PseudoHandle::CurrentThread;
    }

#if   defined(ATMOSPHERE_ARCH_ARM64)


    namespace lp64    { /* ... */ }
    namespace aarch64 { /* ... */ }
    using namespace ::ams::svc::lp64;
    using namespace ::ams::svc::aarch64;

    /* TODO: ifdef ATMOSPHERE_ABI_LP64 */
    #if 1
        namespace aarch64::lp64  { /* ... */ }
        using namespace ::ams::svc::aarch64::lp64;
    #else
        namespace aarch64::ilp32 { /* ... */ }
        using namespace ::ams::svc::aarch64::ilp32;
    #endif

#elif defined(ATMOSPHERE_ARCH_ARM)

    namespace ilp32   { /* ... */ }
    namespace aarch32 { /* ... */ }
    using namespace ::ams::svc::ilp32;
    using namespace ::ams::svc::aarch32;

#else

    #error "Unknown Architecture"

#endif

}
