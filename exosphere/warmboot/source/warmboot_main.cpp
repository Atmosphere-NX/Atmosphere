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
#include "warmboot_bootrom_workaround.hpp"
#include "warmboot_clkrst.hpp"
#include "warmboot_cpu_cluster.hpp"
#include "warmboot_dram.hpp"
#include "warmboot_main.hpp"
#include "warmboot_misc.hpp"
#include "warmboot_secure_monitor.hpp"

namespace ams::warmboot {

    namespace {

        constexpr inline const uintptr_t PMC       = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t FLOW_CTLR = secmon::MemoryRegionPhysicalDeviceFlowController.GetAddress();

        constexpr inline const uintptr_t ExpectedMetadataAddress = 0x40010244;

    }

    void Main(const Metadata *metadata) {
        /* Ensure that we're running under vaguely sane conditions. */
        AMS_ABORT_UNLESS(metadata->magic == Metadata::Magic);
        AMS_ABORT_UNLESS(metadata->target_firmware <= ams::TargetFirmware_Max);

        /* Restrict the bpmp's access to dram. */
        if (metadata->target_firmware >= TargetFirmware_4_0_0) {
            RestrictBpmpAccessToMainMemory();
        }

        /* Configure rtck-daisychaining/jtag. */
        ConfigureMiscSystemDebug();

        /* NOTE: Here, Nintendo checks that the number of burnt anti-downgrade fuses is valid. */

        /* NOTE: Here, Nintendo validates that APBDEV_PMC_SECURE_SCRATCH32 contains the correct magic number for the current warmboot firmware revision. */

        /* Validate that we're executing at the correct address. */
        AMS_ABORT_UNLESS(reinterpret_cast<uintptr_t>(metadata) == ExpectedMetadataAddress);

        /* Validate that we're executing on the bpmp. */
        AMS_ABORT_UNLESS(reg::Read(PG_UP(PG_UP_TAG)) == PG_UP_TAG_PID_COP);

        /* Configure fuse bypass. */
        fuse::ConfigureFuseBypass();

        /* Configure system oscillators. */
        ConfigureOscillators();

        /* Restore DRAM configuration. */
        RestoreRamSvop();
        ConfigureEmcPmacroTraining();

        /* If on Erista, work around the bootrom mbist issue. */
        if (fuse::GetSocType() == fuse::SocType_Erista) {
            ApplyMbistWorkaround();
        }

        /* Initialize the cpu cluster. */
        InitializeCpuCluster();

        /* Restore the secure monitor to tzram. */
        RestoreSecureMonitorToTzram(metadata->target_firmware);

        /* Power on the cpu. */
        PowerOnCpu();

        /* Halt ourselves. */
        while (true) {
            reg::Write(secmon::MemoryRegionPhysicalDeviceFlowController.GetAddress() + FLOW_CTLR_HALT_COP_EVENTS, FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_MODE, FLOW_MODE_STOP),
                                                                                                                  FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_JTAG,        ENABLED));
        }
    }

    NORETURN void ExceptionHandler() {
        /* Write enable to MAIN_RESET. */
        reg::Write(PMC + APBDEV_PMC_CNTRL, PMC_REG_BITS_ENUM(CNTRL_MAIN_RESET, ENABLE));

        /* Wait forever until we're reset. */
        AMS_INFINITE_LOOP();
    }

}

namespace ams::diag {

    void AbortImpl() {
        warmboot::ExceptionHandler();
    }

    #include <exosphere/diag/diag_detailed_assertion_impl.inc>

}
