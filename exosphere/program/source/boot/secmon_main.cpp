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
#include "secmon_boot.hpp"
#include "secmon_boot_functions.hpp"
#include "../smc/secmon_random_cache.hpp"
#include "../smc/secmon_smc_handler.hpp"
#include "../secmon_cache.hpp"
#include "../secmon_cpu_context.hpp"
#include "../secmon_misc.hpp"
#include "../secmon_setup.hpp"

namespace ams::secmon {

    namespace {

        constexpr inline const uintptr_t Package2LoadAddress = MemoryRegionDramPackage2Payloads.GetAddress();

    }

    void Main() {
        /* Set library register addresses. */
        actmon::SetRegisterAddress(MemoryRegionVirtualDeviceActivityMonitor.GetAddress());
        clkrst::SetRegisterAddress(MemoryRegionVirtualDeviceClkRst.GetAddress());
          flow::SetRegisterAddress(MemoryRegionVirtualDeviceFlowController.GetAddress());
          fuse::SetRegisterAddress(MemoryRegionVirtualDeviceFuses.GetAddress());
           gic::SetRegisterAddress(MemoryRegionVirtualDeviceGicDistributor.GetAddress(), MemoryRegionVirtualDeviceGicCpuInterface.GetAddress());
           i2c::SetRegisterAddress(i2c::Port_1, MemoryRegionVirtualDeviceI2c1.GetAddress());
           i2c::SetRegisterAddress(i2c::Port_5, MemoryRegionVirtualDeviceI2c5.GetAddress());
        pinmux::SetRegisterAddress(MemoryRegionVirtualDeviceApbMisc.GetAddress(), MemoryRegionVirtualDeviceGpio.GetAddress());
           pmc::SetRegisterAddress(MemoryRegionVirtualDevicePmc.GetAddress());
            se::SetRegisterAddress(MemoryRegionVirtualDeviceSecurityEngine.GetAddress(), MemoryRegionVirtualDeviceSecurityEngine2.GetAddress());
          uart::SetRegisterAddress(MemoryRegionVirtualDeviceUart.GetAddress());
           wdt::SetRegisterAddress(MemoryRegionVirtualDeviceTimer.GetAddress());
          util::SetRegisterAddress(MemoryRegionVirtualDeviceTimer.GetAddress());

        /* Get the secure monitor parameters. */
        auto &secmon_params = *reinterpret_cast<pkg1::SecureMonitorParameters *>(MemoryRegionVirtualDeviceBootloaderParams.GetAddress());

        /* Perform initialization. */
        {
            /* Perform initial setup. */
            /* This checks the security engine's validity, and configures common interrupts in the GIC. */
            /* This also initializes the global configuration context. */
            secmon::Setup1();
            AMS_SECMON_LOG("%s\n", "Boot begin.");

            /* Save the boot info. */
            secmon::SaveBootInfo(secmon_params);

            /* Perform cold-boot specific init. */
            secmon::boot::InitializeColdBoot();

            /* Setup the SoC security measures. */
            secmon::SetupSocSecurity();

            /* Setup the Cpu core context. */
            secmon::SetupCpuCoreContext();

            /* Clear the crt0 code that was present in iram. */
            secmon::boot::ClearIramBootCode();

            /* Clear the debug code from iram, if we're not in debug config. */
            #if !defined(AMS_BUILD_FOR_DEBUGGING) && !defined(AMS_BUILD_FOR_AUDITING)
            secmon::boot::ClearIramDebugCode();
            #endif

            /* Alert the bootloader that we're initialized. */
            secmon_params.secmon_state = pkg1::SecureMonitorState_Initialized;
        }

        /* Wait for NX Bootloader to finish loading the BootConfig. */
        secmon::boot::WaitForNxBootloader(secmon_params, pkg1::BootloaderState_LoadedBootConfig);
        hw::DataSynchronizationBarrierInnerShareable();

        /* Load the bootconfig. */
        secmon::boot::LoadBootConfig(MemoryRegionPhysicalIramBootConfig.GetPointer());

        /* Verify or clear the boot config. */
        secmon::boot::VerifyOrClearBootConfig();

        /* Get the boot config. */
        const auto &bc = secmon::GetBootConfig();

        /* Set the tsc value by the boot config. */
        {
            constexpr u64 TscMask = (static_cast<u64>(1) << 55) - 1;

            secmon::boot::EnableTsc(bc.data.GetInitialTscValue() & TscMask);
        }

        /* Wait for NX Bootloader to initialize DRAM. */
        secmon::boot::WaitForNxBootloader(secmon_params, pkg1::BootloaderState_InitializedDram);

        /* Secure the PMC and MC. */
        secmon::SetupPmcAndMcSecure();

        /* Copy warmboot to dram. */
        {
            /* Define warmboot extents. */
            const void * const src = MemoryRegionPhysicalIramWarmbootBin.GetPointer();
                  void * const dst = MemoryRegionVirtualDramSecureDataStoreWarmbootFirmware.GetPointer();
            const size_t size      = MemoryRegionPhysicalIramWarmbootBin.GetSize();

            /* Ensure we copy the correct data. */
            hw::FlushDataCache(src, size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Copy warmboot.bin to its secure dram location. */
            std::memcpy(dst, src, size);
        }

        /* Load the mariko program image. */
        secmon::boot::LoadMarikoProgram();

        /* Setup the GPU carveout's magic numbers. */
        secmon::boot::WriteGpuCarveoutMagicNumbers();

        /* Wait for NX bootloader to load Package2. */
        secmon::boot::WaitForNxBootloader(secmon_params, pkg1::BootloaderState_LoadedPackage2);

        /* Parse and decrypt the package2 header. */
        pkg2::Package2Meta &pkg2_meta = secmon::boot::GetEphemeralPackage2Meta();
        const uintptr_t pkg2_payloads_start = MemoryRegionDramPackage2.GetAddress() + sizeof(pkg2::Package2Header);
        {
            /* Read the encrypred header. */
            pkg2::Package2Header encrypted_header;

            const auto *dram_header = MemoryRegionDramPackage2.GetPointer<pkg2::Package2Header>();
            hw::FlushDataCache(dram_header, sizeof(*dram_header));
            hw::DataSynchronizationBarrierInnerShareable();

            std::memcpy(std::addressof(encrypted_header), dram_header, sizeof(encrypted_header));

            /* Atmosphere extension: support plaintext package2, identified by all-zeroes signature and decrypted header. */
            secmon::boot::UpdateBootConfigForPackage2Header(encrypted_header);

            /* Verify the package2 header's signature. */
            secmon::boot::VerifyPackage2HeaderSignature(encrypted_header, !bc.signed_data.IsPackage2SignatureVerificationDisabled());

            /* Decrypt the package2 header. */
            secmon::boot::DecryptPackage2Header(std::addressof(pkg2_meta), encrypted_header.meta, !bc.signed_data.IsPackage2EncryptionDisabled());
        }

        /* Verify the package2 header. */
        secmon::boot::VerifyPackage2Header(pkg2_meta);

        /* Save the package2 hash if in recovery boot. */
        if (secmon::IsRecoveryBoot()) {
            se::Sha256Hash hash;
            secmon::boot::CalculatePackage2Hash(std::addressof(hash), pkg2_meta, MemoryRegionDramPackage2.GetAddress());
            secmon::SetPackage2Hash(hash);
        }

        /* Verify the package2 payloads. */
        secmon::boot::CheckVerifyResult(secmon::boot::VerifyPackage2Payloads(pkg2_meta, pkg2_payloads_start), pkg1::ErrorInfo_InvalidPackage2Payload, "pkg2 payload FAIL");

        /* Decrypt/Move the package2 payloads to the right places. */
        secmon::boot::DecryptAndLoadPackage2Payloads(Package2LoadAddress, pkg2_meta, pkg2_payloads_start, !bc.signed_data.IsPackage2EncryptionDisabled());

        /* Ensure that the CPU sees correct package2 data. */
        secmon::FlushEntireDataCache();
        secmon::EnsureInstructionConsistency();

        /* Set the core's entrypoint and argument. */
        secmon::SetEntryContext(0, Package2LoadAddress + pkg2_meta.entrypoint, 0);

        /* Clear the boot keys from iram. */
        secmon::boot::ClearIramBootKeys();

        /* Unmap the identity mapping. */
        secmon::boot::UnmapPhysicalIdentityMapping();

        /* Unmap DRAM. */
        secmon::boot::UnmapDram();

        /* Wait for NX bootloader to be done. */
        secmon::boot::WaitForNxBootloader(secmon_params, pkg1::BootloaderState_Done);

        /* Perform final initialization. */
        secmon::SetupSocProtections();
        secmon::SetupCpuSErrorDebug();

        /* Configure the smc handler tables to reflect the current target firmware. */
        secmon::smc::ConfigureSmcHandlersForTargetFirmware();

        AMS_SECMON_LOG("%s\n", "Boot end.");
    }

}
