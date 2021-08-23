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
#include <exosphere.hpp>
#include "fusee_fatal.hpp"
#include "fs/fusee_fs_api.hpp"

namespace ams::nxboot {

    namespace {

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

        /* TODO: Reboot to payload. */
        pmic::ShutdownSystem(true);

        /* Wait for our reboot to complete. */
        AMS_INFINITE_LOOP();
    }

}
