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
#include "fusee_display.hpp"
#include "sein/fusee_secure_initialize.hpp"
#include "sdram/fusee_sdram.hpp"
#include "mtc/fusee_mtc.hpp"
#include "fs/fusee_fs_api.hpp"
#include "fusee_overlay_manager.hpp"
#include "fusee_sd_card.hpp"
#include "fusee_fatal.hpp"
#include "fusee_external_package.hpp"
#include "fusee_setup_horizon.hpp"
#include "fusee_secmon_sync.hpp"

namespace ams::nxboot {

    namespace {

        constexpr const char ExternalPackageFilePath[] = "sdmc:/atmosphere/package3";

        constinit fs::FileHandle g_package_file;

        void OpenExternalPackage() {
            Result result;

            /* Open external package. */
            if (R_FAILED((result = fs::OpenFile(std::addressof(g_package_file), ExternalPackageFilePath, fs::OpenMode_Read)))) {
                ShowFatalError("Failed to open %s!\n", ExternalPackageFilePath);
            }

            /* Get file size. */
            s64 file_size;
            if (R_FAILED((result = fs::GetFileSize(std::addressof(file_size), g_package_file)))) {
                ShowFatalError("Failed to get package3 size: 0x%08" PRIx32 "\n", result.GetValue());
            }

            /* Check file size. */
            if (static_cast<size_t>(file_size) != ExternalPackageSize) {
                ShowFatalError("package3 seems corrupted (size 0x%zx != 0x%zx)", static_cast<size_t>(file_size), ExternalPackageSize);
            }
        }

        void ReadFullExternalPackage() {
            Result result;

            if (R_FAILED((result = fs::ReadFile(g_package_file, 0, const_cast<void *>(static_cast<const void *>(std::addressof(GetExternalPackage()))), ExternalPackageSize)))) {
                ShowFatalError("Failed to read %s!\n", ExternalPackageFilePath);
            }
        }

        void CloseExternalPackage() {
            fs::CloseFile(g_package_file);
        }

    }

    void Main() {
        /* Perform secure hardware initialization. */
        SecureInitialize(true);

        /* Overclock the bpmp. */
        clkrst::SetBpmpClockRate(fuse::GetSocType() == fuse::SocType_Mariko ? clkrst::BpmpClockRate_589MHz : clkrst::BpmpClockRate_576MHz);

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

        /* Open the external package. */
        OpenExternalPackage();

        /* Load the memory training overlay. */
        LoadOverlay(g_package_file, OverlayId_MemoryTraining);

        /* Do memory training. */
        DoMemoryTraining();

        /* Read the rest of the archive file. */
        ReadFullExternalPackage();

        /* Save the memory training overlay. */
        SaveMemoryTrainingOverlay();

        /* Initialize display (splash screen will be visible from this point onwards). */
        InitializeDisplay();
        ShowDisplay();

        /* Close the external package. */
        CloseExternalPackage();

        /* Perform rest of the boot process. */
        SetupAndStartHorizon();

        /* Restore the memory training overlay. */
        RestoreMemoryTrainingOverlay();

        /* Restore memory clock rate. */
        RestoreMemoryClockRate();

        /* Restore secure monitor code. */
        RestoreSecureMonitorOverlay();

        /* Finalize display. */
        FinalizeDisplay();

        /* Finalize sd card. */
        FinalizeSdCard();

        /* Finalize the data cache. */
        hw::FinalizeDataCache();

        /* Downclock the bpmp. */
        clkrst::SetBpmpClockRate(clkrst::BpmpClockRate_408MHz);

        /* Signal to the secure monitor that we're done. */
        SetBootloaderState(pkg1::BootloaderState_Done);

        /* Halt ourselves. */
        while (true) {
            reg::Write(secmon::MemoryRegionPhysicalDeviceFlowController.GetAddress() + FLOW_CTLR_HALT_COP_EVENTS, FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_MODE, FLOW_MODE_STOP),
                                                                                                                  FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_JTAG,        ENABLED));
        }
    }

}
