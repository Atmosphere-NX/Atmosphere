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
#include <stratosphere.hpp>
#include "amsmitm_debug.hpp"

namespace ams::mitm {

    namespace {

        os::Mutex g_throw_lock(false);
        bool g_threw;
        Result g_throw_result;


        void DebugThrowThreadFunc(void *arg);

        constexpr size_t DebugThrowThreadStackSize = 0x4000;
        os::ThreadType g_debug_throw_thread;

        alignas(os::ThreadStackAlignment) u8 g_debug_throw_thread_stack[DebugThrowThreadStackSize];

        void DebugThrowThreadFunc(void *arg) {
            /* TODO: Better heuristic for fatal startup than sleep. */
            os::SleepThread(TimeSpan::FromSeconds(10));
            fatalThrow(g_throw_result.GetValue());
        }

    }

    void ThrowResultForDebug(Result res) {
        std::scoped_lock lk(g_throw_lock);

        if (g_threw) {
            return;
        }

        g_throw_result = res;
        g_threw = true;
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_debug_throw_thread), DebugThrowThreadFunc, nullptr, g_debug_throw_thread_stack, sizeof(g_debug_throw_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(mitm, DebugThrowThread)));
        os::SetThreadNamePointer(std::addressof(g_debug_throw_thread), AMS_GET_SYSTEM_THREAD_NAME(mitm, DebugThrowThread));
        os::StartThread(std::addressof(g_debug_throw_thread));
    }

}
