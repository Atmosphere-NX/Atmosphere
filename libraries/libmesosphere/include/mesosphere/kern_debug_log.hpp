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
#include <mesosphere/svc/kern_svc_k_user_pointer.hpp>

namespace ams::kern {

    class KDebugLog {
        private:
            static NOINLINE void VSNPrintf(char *dst, const size_t dst_size, const char *format, ::std::va_list vl);
        public:
            static NOINLINE void Initialize();

            static NOINLINE void Printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
            static NOINLINE void VPrintf(const char *format, ::std::va_list vl);

            static NOINLINE Result PrintUserString(ams::kern::svc::KUserPointer<const char *> user_str, size_t len);

            /* Functionality for preserving across sleep. */
            static NOINLINE void Save();
            static NOINLINE void Restore();
    };

}

#ifndef MESOSPHERE_DEBUG_LOG_SELECTED

    #ifdef ATMOSPHERE_BOARD_NINTENDO_NX
        #define MESOSPHERE_DEBUG_LOG_USE_UART
    #else
        #error "Unknown board for Default Debug Log Source"
    #endif

    #define MESOSPHERE_DEBUG_LOG_SELECTED

#endif

#define MESOSPHERE_RELEASE_LOG(fmt, ...) ::ams::kern::KDebugLog::Printf((fmt), ## __VA_ARGS__)
#define MESOSPHERE_RELEASE_VLOG(fmt, vl) ::ams::kern::KDebugLog::VPrintf((fmt), (vl))

#ifdef MESOSPHERE_ENABLE_DEBUG_PRINT
#define MESOSPHERE_LOG(fmt, ...) MESOSPHERE_RELEASE_LOG((fmt), ## __VA_ARGS__)
#define MESOSPHERE_VLOG(fmt, vl) MESOSPHERE_RELEASE_VLOG((fmt), (vl))
#else
#define MESOSPHERE_LOG(fmt, ...) do { MESOSPHERE_UNUSED(fmt); MESOSPHERE_UNUSED(__VA_ARGS__); } while (0)
#define MESOSPHERE_VLOG(fmt, vl) do { MESOSPHERE_UNUSED(fmt); MESOSPHERE_UNUSED(vl); } while (0)
#endif
