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

#define DOCTEST_CONFIG_IMPLEMENT
#include "util_test_framework.hpp"

namespace ams {

    namespace {

        constexpr size_t MallocBufferSize = 16_MB;
        alignas(os::MemoryPageSize) constinit u8 g_malloc_buffer[MallocBufferSize];

    }

    namespace hos {

        bool IsUnitTestProgramForSetVersion() { return true; }

    }

    namespace init {

        void InitializeSystemModuleBeforeConstructors() {
            /* Catch has global-ctors which allocate, so we need to do this earlier than normal. */
            init::InitializeAllocator(g_malloc_buffer, sizeof(g_malloc_buffer));
        }

        void InitializeSystemModule() { /* ... */ }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void NORETURN Exit(int rc) {
        AMS_UNUSED(rc);
        AMS_ABORT("Exit called by immortal process");
    }

    void Main() {
        /* Ensure our thread priority and core mask is correct. */
        {
            auto * const cur_thread = os::GetCurrentThread();
            os::SetThreadCoreMask(cur_thread, 3, (1ul << 3));
            os::ChangeThreadPriority(cur_thread, 0);
        }

        /* Run tests. */
        {
            doctest::Context ctx;

            ctx.applyCommandLine(os::GetHostArgc(), os::GetHostArgv());

            ctx.run();
        }

        AMS_INFINITE_LOOP();

        /* This can never be reached. */
        AMS_ASSUME(false);
    }

}

namespace doctest {

    namespace {

        class OutputDebugStringStream : public std::stringbuf {
            public:
                OutputDebugStringStream() = default;
                ~OutputDebugStringStream() { pubsync(); }

                int sync() override {
                    const auto message = str();
                    return R_SUCCEEDED(ams::svc::OutputDebugString(message.c_str(), message.length())) ? 0 : -1;
                }
        };

    }

    std::ostream& get_cout() {
        static std::ostream ret(new OutputDebugStringStream);
        return ret;
    }

    std::ostream& get_cerr() {
        return get_cout();
    }

}