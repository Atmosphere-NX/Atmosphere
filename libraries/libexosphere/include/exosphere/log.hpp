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

namespace ams::log {

    #if defined(AMS_BUILD_FOR_AUDITING) || defined(AMS_BUILD_FOR_DEBUGGING)
        #define AMS_IMPL_ENABLE_LOG
    #endif

    #if defined(AMS_IMPL_ENABLE_LOG)
        #define AMS_LOG(...)    ::ams::log::Printf(__VA_ARGS__)
        #define AMS_VLOG(...)   ::ams::log::VPrintf(__VA_ARGS__)
        #define AMS_DUMP(...)   ::ams::log::Dump(__VA_ARGS__)
        #define AMS_LOG_FLUSH() ::ams::log::Flush()
    #else
        #define AMS_LOG(...)    static_cast<void>(0)
        #define AMS_VLOG(...)   static_cast<void>(0)
        #define AMS_DUMP(...)   static_cast<void>(0)
        #define AMS_LOG_FLUSH() static_cast<void>(0)
    #endif

    void Initialize();
    void Initialize(uart::Port port, u32 baud_rate, u32 flags);
    void Finalize();

    void Printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
    void VPrintf(const char *fmt, ::std::va_list vl);
    void Dump(const void *src, size_t size);

    void SendText(const void *text, size_t size);
    void Flush();

}
