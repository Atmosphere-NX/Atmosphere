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

#define CATCH_CONFIG_RUNNER
#include "util_catch.hpp"

namespace ams {

    namespace {

        constexpr size_t MallocBufferSize = 16_MB;
        alignas(os::MemoryPageSize) constinit u8 g_malloc_buffer[MallocBufferSize];

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
        /* Run tests. */
        Catch::Session().run(os::GetHostArgc(), os::GetHostArgv());

        AMS_INFINITE_LOOP();

        /* This can never be reached. */
        AMS_ASSUME(false);
    }

}

namespace Catch {

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

    std::ostream& cout() {
        static std::ostream ret(new OutputDebugStringStream);
        return ret;
    }
    std::ostream& clog() {
        return cout();
    }
    std::ostream& cerr() {
        return clog();
    }
}