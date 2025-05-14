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
#include "amsmitm_initialization.hpp"
#include "amsmitm_module_management.hpp"
#include "bpc_mitm/bpc_ams_power_utils.hpp"
#include "sysupdater/sysupdater_fs_utils.hpp"

namespace ams {

    namespace {

        /* TODO: we really shouldn't be using malloc just to avoid dealing with real allocator separation. */
        constexpr size_t MallocBufferSize = 12_MB;
        alignas(os::MemoryPageSize) constinit u8 g_malloc_buffer[MallocBufferSize];

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Initialize fs. */
            fs::InitializeForSystem();
            fs::SetEnabledAutoAbort(false);

            /* Initialize other services. */
            R_ABORT_UNLESS(pmdmntInitialize());
            R_ABORT_UNLESS(pminfoInitialize());
            ncm::Initialize();

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() {
            /* Initialize the global malloc allocator. */
            init::InitializeAllocator(g_malloc_buffer, sizeof(g_malloc_buffer));
        }

    }

    void ExceptionHandler(FatalErrorContext *ctx) {
        /* We're bpc-mitm (or ams_mitm, anyway), so manually reboot to fatal error. */
        mitm::bpc::RebootForFatalError(ctx);
    }

    void NORETURN Exit(int rc) {
        AMS_UNUSED(rc);
        AMS_ABORT("Exit called by immortal process");
    }

    void Main() {
        /* Register "ams" port, use up its session. */
        {
            svc::Handle ams_port;
            R_ABORT_UNLESS(svc::ManageNamedPort(std::addressof(ams_port), "ams", 1));

            svc::Handle ams_session;
            R_ABORT_UNLESS(svc::ConnectToNamedPort(std::addressof(ams_session), "ams"));
        }

        /* Initialize fssystem library. */
        fssystem::InitializeForAtmosphereMitm();

        /* Configure ncm to use fssystem library to mount content from the sd card. */
        ncm::SetMountContentMetaFunction(mitm::sysupdater::MountSdCardContentMeta);

        /* Launch all mitm modules in sequence. */
        mitm::LaunchAllModules();

        /* Wait for all mitm modules to end. */
        mitm::WaitAllModules();
    }

}
