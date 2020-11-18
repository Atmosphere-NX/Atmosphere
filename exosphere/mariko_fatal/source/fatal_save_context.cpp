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
#include "fatal_save_context.hpp"
#include "fatal_sdmmc.hpp"
#include "fs/fatal_fs_api.hpp"

namespace ams::secmon::fatal {

    Result SaveFatalErrorContext(const ams::impl::FatalErrorContext *ctx) {
        /* Initialize the sdmmc driver. */
        R_TRY(InitializeSdCard());

        /* Get the connection status. */
        #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
        {
            sdmmc::SpeedMode speed_mode;
            sdmmc::BusWidth bus_width;
            R_TRY(CheckSdCardConnection(std::addressof(speed_mode), std::addressof(bus_width)));
            AMS_SECMON_LOG("Sd Card Connection:\n");
            AMS_SECMON_LOG("    Speed Mode: %u\n", static_cast<u32>(speed_mode));
            AMS_SECMON_LOG("    Bus Width:  %u\n", static_cast<u32>(bus_width));
        }
        #endif

        /* Mount the SD card. */
        R_UNLESS(fs::MountSdCard(), fs::ResultPartitionNotFound());

        /* Unmount the SD card once we're done. */
        ON_SCOPE_EXIT { fs::UnmountSdCard(); };

        /* Create and open the file. */
        fs::FileHandle file;
        {
            /* Generate the file path. */
            char path[0x40];
            util::TSNPrintf(path, sizeof(path), "/atmosphere/fatal_errors/report_%016" PRIx64 ".bin", ctx->report_identifier);

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
