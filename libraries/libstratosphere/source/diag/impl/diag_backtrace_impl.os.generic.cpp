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
#include <stratosphere.hpp>

#if defined(ATMOSPHERE_OS_WINDOWS)
#include <stratosphere/windows.hpp>
#elif defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ams::diag::impl {

    namespace {

        #if defined(ATMOSPHERE_ARCH_X64)
        struct StackFrame {
            u64 fp; /* rbp */
            u64 lr; /* rip */
        };
        #elif defined(ATMOSPHERE_ARCH_X86)
        struct StackFrame {
            u32 fp; /* ebp */
            u32 lr; /* eip */
        }
        #elif defined(ATMOSPHERE_ARCH_ARM64)
        struct StackFrame {
            u64 fp;
            u64 lr;
        };
        #elif defined(ATMOSPHERE_ARCH_ARM)
        struct StackFrame {
            u32 fp;
            u32 lr;
        }
        #else
            #error "Unknown architecture for generic backtrace."
        #endif

        bool TryRead(os::NativeHandle native_handle, void *dst, size_t size, const void *address) {
            #if defined(ATMOSPHERE_OS_WINDOWS)
            return ::ReadProcessMemory(native_handle, address, dst, size, nullptr);
            #elif defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)
            s32 ret;
            do {
                ret = ::write(native_handle, address, size);
            } while (ret < 0 && errno == EINTR);

            if (ret < 0) {
                return false;
            }

            std::memcpy(dst, address, size);
            return true;
            #else
                #error "Unknown OS for Backtrace native handle"
            #endif
        }

    }

    NOINLINE void Backtrace::Initialize() {
        /* Clear our size. */
        m_index = 0;
        m_size  = 0;

        /* Get the base frame pointer. */
        const void *cur_fp = __builtin_frame_address(0);

        /* Try to read stack frames, until we run out. */
        #if defined(ATMOSPHERE_OS_WINDOWS)
        const os::NativeHandle native_handle = ::GetCurrentProcess();
        #elif defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)
        os::NativeHandle pipe_handles[2];
        s32 nret;
        do { nret = ::pipe(pipe_handles); } while (nret < 0 && errno == EINTR);
        if (nret < 0) { return; }
        do { nret = ::fcntl(pipe_handles[0], F_SETFL, O_NONBLOCK); } while (nret < 0 && errno == EINTR);
        if (nret < 0) { return; }
        do { nret = ::fcntl(pipe_handles[1], F_SETFL, O_NONBLOCK); } while (nret < 0 && errno == EINTR);
        if (nret < 0) { return; }
        ON_SCOPE_EXIT {
            do { nret = ::close(pipe_handles[0]); } while (nret < 0 && errno == EINTR);
            do { nret = ::close(pipe_handles[1]); } while (nret < 0 && errno == EINTR);
        };
        const os::NativeHandle native_handle = pipe_handles[1];
        if (native_handle < 0) { return; }
        #else
            #error "Unknown OS for Backtrace native handle"
        #endif

        StackFrame frame;
        while (m_size < BacktraceEntryCountMax) {
            /* Clear the frame. */
            frame = {};

            /* Read the next frame. */
            if (!TryRead(native_handle, std::addressof(frame), sizeof(frame), cur_fp)) {
                break;
            }

            /* Add the return address. */
            m_backtrace_addresses[m_size++] = reinterpret_cast<void *>(frame.lr);

            /* Set the next fp. */
            cur_fp = reinterpret_cast<const void *>(frame.fp);
        }
    }

    bool Backtrace::Step() {
        return (++m_index) < m_size;
    }

    uintptr_t Backtrace::GetStackPointer() const {
        return 0;
    }

    uintptr_t Backtrace::GetReturnAddress() const {
        return reinterpret_cast<uintptr_t>(m_backtrace_addresses[m_index]);
    }

}
