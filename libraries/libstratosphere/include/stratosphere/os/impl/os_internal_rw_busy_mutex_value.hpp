/*
 * Copyright (c) Atmosphère-NX
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

namespace ams::os::impl {

    #if defined(ATMOSPHERE_OS_HORIZON) || defined(ATMOSPHERE_OS_WINDOWS) || defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)
        class InternalReaderWriterBusyMutexValue {
            public:
                static constexpr inline u8 WriterCountMax = std::numeric_limits<u8>::max();

                static constexpr ALWAYS_INLINE u16 GetReaderCount(u32 v) {
                    return static_cast<u16>(v >> 0);
                }

                static constexpr ALWAYS_INLINE u8 GetWriterCurrent(u32 v) {
                    return static_cast<u8>(v >> 16);
                }

                static constexpr ALWAYS_INLINE u8 GetWriterNext(u32 v) {
                    return static_cast<u8>(v >> 24);
                }

                static constexpr ALWAYS_INLINE u32 IncrementWriterNext(u32 v) {
                    return v + (1u << 24);
                }

                static constexpr ALWAYS_INLINE bool IsWriteLocked(u32 v) {
                    return GetWriterCurrent(v) != GetWriterNext(v);
                }

                static ALWAYS_INLINE u8 *GetWriterCurrentPointer(u32 *p) {
                    if constexpr (util::IsLittleEndian()) {
                        return reinterpret_cast<u8 *>(reinterpret_cast<uintptr_t>(p)) + 2;
                    } else {
                        return reinterpret_cast<u8 *>(reinterpret_cast<uintptr_t>(p)) + 1;
                    }
                }
        };
    #else
        #error "Unknown OS for ams::os::impl::InternalReaderWriterBusyMutexValue"
    #endif

}
