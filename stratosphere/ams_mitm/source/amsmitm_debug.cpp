/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

        os::Mutex g_throw_lock;
        bool g_threw;
        Result g_throw_result;


        void DebugThrowThreadFunc(void *arg);

        constexpr size_t DebugThrowThreadStackSize = 0x4000;
        constexpr int    DebugThrowThreadPriority  = 49;
        os::StaticThread<DebugThrowThreadStackSize> g_debug_throw_thread(&DebugThrowThreadFunc, nullptr, DebugThrowThreadPriority);

        void DebugThrowThreadFunc(void *arg) {
            /* TODO: Better heuristic for fatal startup than sleep. */
            svcSleepThread(10'000'000'000ul);
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
        R_ASSERT(g_debug_throw_thread.Start());
    }

}
