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
#include "amsmitm_initialization.hpp"
#include "amsmitm_module_management.hpp"
#include "bpc_mitm/bpc_ams_power_utils.hpp"
#include "sysupdater/sysupdater_fs_utils.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    #define INNER_HEAP_SIZE 0x1000000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::AtmosphereProgramId::Mitm;

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

    /* Override. */
    void ExceptionHandler(FatalErrorContext *ctx) {
        /* We're bpc-mitm (or ams_mitm, anyway), so manually reboot to fatal error. */
        mitm::bpc::RebootForFatalError(ctx);
    }

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(fsInitialize());
        R_ABORT_UNLESS(pmdmntInitialize());
        R_ABORT_UNLESS(pminfoInitialize());
        ncm::Initialize();
        spl::InitializeForFs();
    });

    /* Disable auto-abort in fs operations. */
    fs::SetEnabledAutoAbort(false);

    /* Initialize fssystem library. */
    fssystem::InitializeForFileSystemProxy();

    /* Configure ncm to use fssystem library to mount content from the sd card. */
    ncm::SetMountContentMetaFunction(mitm::sysupdater::MountSdCardContentMeta);

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* Cleanup services. */
    spl::Finalize();
    ncm::Finalize();
    pminfoExit();
    pmdmntExit();
    fsExit();
}

int main(int argc, char **argv) {
    /* Launch all mitm modules in sequence. */
    mitm::LaunchAllModules();

    /* Wait for all mitm modules to end. */
    mitm::WaitAllModules();

    return 0;
}

