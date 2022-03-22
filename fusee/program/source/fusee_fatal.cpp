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
#include <exosphere.hpp>
#include "fusee_fatal.hpp"
#include "fusee_external_package.hpp"
#include "fs/fusee_fs_api.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t PMC = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();

        Result SaveFatalErrorContext(const ams::impl::FatalErrorContext *ctx) {
            /* Create and open the file. */
            fs::FileHandle file;
            {
                /* Generate the file path. */
                char path[0x40];
                util::TSNPrintf(path, sizeof(path), "sdmc:/atmosphere/fatal_errors/report_%016" PRIx64 ".bin", ctx->report_identifier);

                /* Create the file. */
                R_TRY(fs::CreateFile(path, sizeof(*ctx)));

                /* Open the file. */
                R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_ReadWrite));
            }

            /* Ensure we close the file when done with it. */
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Write the context to the file. */
            R_TRY(fs::WriteFile(file, 0, ctx, sizeof(*ctx), fs::WriteOption::Flush));

            return ResultSuccess();
        }

    }

    NORETURN void RebootToSelf() {
        /* Patch SDRAM init to perform an SVC immediately after second write. */
        reg::Write(PMC + APBDEV_PMC_SCRATCH45, 0x2E38DFFF);
        reg::Write(PMC + APBDEV_PMC_SCRATCH46, 0x6001DC28);

        /* Set SVC handler to jump to reboot stub in IRAM. */
        reg::Write(PMC + APBDEV_PMC_SCRATCH33, 0x4003F000);
        reg::Write(PMC + APBDEV_PMC_SCRATCH40, 0x6000F208);

        /* Set boot as warmboot. */
        reg::Write(PMC + APBDEV_PMC_SCRATCH0, (1 << 0));

        /* Copy reboot stub into high IRAM. */
        std::memcpy(reinterpret_cast<void *>(0x4003F000), GetExternalPackage().reboot_stub, sizeof(GetExternalPackage().reboot_stub));

        /* Copy our main payload into low IRAM. */
        std::memcpy(reinterpret_cast<void *>(0x40010000), GetExternalPackage().fusee, sizeof(GetExternalPackage().fusee));

        /* Reboot. */
        reg::Write(PMC + APBDEV_PMC_CNTRL, PMC_REG_BITS_ENUM(CNTRL_MAIN_RESET, ENABLE));

        /* Wait for the reboot to take. */
        AMS_INFINITE_LOOP();
    }

    void SaveAndShowFatalError() {
        /* Get the context (at static location in memory). */
        ams::impl::FatalErrorContext *f_ctx = reinterpret_cast<ams::impl::FatalErrorContext *>(0x4003E000);

        /* Check for valid magic. */
        if (f_ctx->magic != ams::impl::FatalErrorContext::Magic) {
            return;
        }

        /* Show the fatal error. */
        ShowFatalError(f_ctx, SaveFatalErrorContext(f_ctx));

        /* Clear the magic. */
        f_ctx->magic = ~f_ctx->magic;

        /* Wait for reboot. */
        WaitForReboot();
    }

    void WaitForReboot() {
        /* Wait for power button to be pressed. */
        while (!pmic::IsPowerButtonPressed()) {
            util::WaitMicroSeconds(100);
        }

        /* If not erista, just do a normal reboot. */
        if (fuse::GetSocType() != fuse::SocType_Erista) {
            /* Reboot. */
            pmic::ShutdownSystem(true);

            /* Wait for our reboot to complete. */
            AMS_INFINITE_LOOP();
        }

        /* Reboot to self, if we can. */
        if (GetExternalPackage().header.magic == ExternalPackageHeader::Magic) {
            RebootToSelf();
        } else {
            /* Just do a normal reboot. */
            pmic::ShutdownSystem(true);

            /* Wait for our reboot to complete. */
            AMS_INFINITE_LOOP();
        }
    }

}
