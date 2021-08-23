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
#include "fusee_display.hpp"
#include "sein/fusee_secure_initialize.hpp"
#include "sdram/fusee_sdram.hpp"
#include "fs/fusee_fs_api.hpp"
#include "fusee_sd_card.hpp"
#include "fusee_fatal.hpp"
#include "fusee_secondary_archive.hpp"

namespace ams::nxboot {

    namespace {

        /* TODO: Change to fusee-secondary.bin when development is done. */
        constexpr const char FuseeSecondaryFilePath[] = "sdmc:/atmosphere/fusee-boogaloo.bin";

        void ReadFuseeSecondary() {
            Result result;

            /* Open fusee-secondary. */
            fs::FileHandle file;
            if (R_FAILED((result = fs::OpenFile(std::addressof(file), FuseeSecondaryFilePath, fs::OpenMode_Read)))) {
                ShowFatalError("Failed to open %s!\n", FuseeSecondaryFilePath);
            }

            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Get file size. */
            s64 file_size;
            if (R_FAILED((result = fs::GetFileSize(std::addressof(file_size), file)))) {
                ShowFatalError("Failed to get fusee-secondary size: 0x%08" PRIx32 "\n", result.GetValue());
            }

            /* Check file size. */
            if (static_cast<size_t>(file_size) != SecondaryArchiveSize) {
                ShowFatalError("fusee-secondary seems corrupted (size 0x%zx != 0x%zx)", static_cast<size_t>(file_size), SecondaryArchiveSize);
            }

            /* Read to fixed address. */
            if (R_FAILED((result = fs::ReadFile(file, 0, const_cast<SecondaryArchive *>(std::addressof(GetSecondaryArchive())), SecondaryArchiveSize)))) {
                ShowFatalError("Failed to read fusee-secondary: 0x%08" PRIx32 "\n", result.GetValue());
            }
        }

    }

    void Main() {
        /* Perform secure hardware initialization. */
        SecureInitialize(true);

        /* Initialize Sdram. */
        InitializeSdram();

        /* Initialize cache. */
        hw::InitializeDataCache();

        /* Initialize SD card. */
        {
            Result result = InitializeSdCard();
            if (R_FAILED(result)) {
                ShowFatalError("Failed to initialize the SD card: 0x%08" PRIx32 "\n", result.GetValue());
            }
        }

        /* Mount SD card. */
        if (!fs::MountSdCard()) {
            ShowFatalError("Failed to mount the SD card.");
        }

        /* If we have a fatal error, save and display it. */
        SaveAndShowFatalError();

        /* Read our overlay file. */
        ReadFuseeSecondary();

        /* TODO */

        /* TODO */
        AMS_INFINITE_LOOP();
    }

}
