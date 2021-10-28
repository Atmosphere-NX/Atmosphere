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

extern "C" {

    constinit u32 __nx_fs_num_sessions = 1;
    constinit u32 __nx_applet_type = AppletType_None;

    constinit bool __nx_fsdev_support_cwd = false;

    extern int __system_argc;
    extern char** __system_argv;

    alignas(16) constinit u8 __nx_exception_stack[::ams::os::MemoryPageSize];
    constinit u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

}

namespace ams {

    namespace hos {

        void InitializeForStratosphere();

    }

    namespace init {

        void InitializeSystemModuleBeforeConstructors();

        void InitializeSystemModule();
        void FinalizeSystemModule();

        void Startup();

    }

    void Main();

}

namespace {

    constinit char *g_empty_argv = nullptr;

}

extern "C" void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ::ams::CrashHandler(ctx);
}

extern "C" void __libnx_initheap(void) {
    /* Stratosphere system modules do not support newlib heap. */
}

extern "C" void __appInit(void) {
    /* The very first thing all stratosphere code must do is initialize the os library. */
    ::ams::hos::InitializeForStratosphere();

    /* Perform pre-C++ constructor init. */
    ::ams::init::InitializeSystemModuleBeforeConstructors();
}

extern "C" void __appExit(void) {
    /* ... */
}

extern "C" void argvSetup(void) {
    /* We don't use newlib argc/argv, so we can clear these. */
    __system_argc = 0;
    __system_argv = std::addressof(g_empty_argv);
}

extern "C" int main(int argc, char **argv) {
    /* We don't use newlib argc/argv. */
    AMS_UNUSED(argc, argv);

    /* Perform remainder of logic with system module initialized. */
    {
        ::ams::init::InitializeSystemModule();
        ON_SCOPE_EXIT { ::ams::init::FinalizeSystemModule(); };

        /* Perform miscellaneous startup. */
        ::ams::init::Startup();

        /* Invoke ams main. */
        ::ams::Main();
    }
}

extern "C" WEAK_SYMBOL void *__libnx_alloc(size_t) {
    AMS_ABORT("__libnx_alloc was called");
}

extern "C" WEAK_SYMBOL void *__libnx_aligned_alloc(size_t, size_t) {
    AMS_ABORT("__libnx_aligned_alloc was called");
}

extern "C" WEAK_SYMBOL void __libnx_free(void *) {
    AMS_ABORT("__libnx_free was called");
}