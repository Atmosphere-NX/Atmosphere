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
#include "../secmon_cache.hpp"
#include "../secmon_cpu_context.hpp"
#include "../secmon_error.hpp"
#include "../secmon_misc.hpp"
#include "secmon_smc_power_management.hpp"
#include "secmon_smc_se_lock.hpp"

#include "sc7fw_bin.h"

namespace ams::secmon {

    /* Declare assembly functionality. */
    void *GetCoreExceptionStackVirtual();

}

namespace ams::secmon::smc {

    /* Declare assembly power-management functionality. */
    void PivotStackAndInvoke(void *stack, void (*function)());
    void FinalizePowerOff();

    namespace {

        constexpr inline const uintptr_t PMC       = MemoryRegionVirtualDevicePmc.GetAddress();
        constexpr inline const uintptr_t APB_MISC  = MemoryRegionVirtualDeviceApbMisc.GetAddress();
        constexpr inline const uintptr_t GPIO      = MemoryRegionVirtualDeviceGpio.GetAddress();
        constexpr inline const uintptr_t CLK_RST   = MemoryRegionVirtualDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t EVP       = secmon::MemoryRegionVirtualDeviceExceptionVectors.GetAddress();
        constexpr inline const uintptr_t FLOW_CTLR = MemoryRegionVirtualDeviceFlowController.GetAddress();
        constexpr inline const uintptr_t AHB_ARBC  = MemoryRegionVirtualDeviceSystem.GetAddress();

        constexpr inline uintptr_t CommonSmcStackTop = MemoryRegionVirtualTzramVolatileData.GetEndAddress() - (0x80 * (NumCores - 1));

        enum PowerStateType {
            PowerStateType_StandBy   = 0,
            PowerStateType_PowerDown = 1,
        };

        enum PowerStateId {
            PowerStateId_Sc7 = 27,
        };

        /* http://infocenter.arm.com/help/topic/com.arm.doc.den0022d/Power_State_Coordination_Interface_PDD_v1_1_DEN0022D.pdf Page 46 */
        struct SuspendCpuPowerState {
            using StateId    = util::BitPack32::Field< 0, 16, PowerStateId>;
            using StateType  = util::BitPack32::Field<16,  1, PowerStateType>;
            using PowerLevel = util::BitPack32::Field<24,  2, u32>;
        };

        constinit bool g_charger_hi_z_mode_enabled = false;

        constinit const reg::BitsMask CpuPowerGateStatusMasks[NumCores] = {
            PMC_REG_BITS_MASK(PWRGATE_STATUS_CE0),
            PMC_REG_BITS_MASK(PWRGATE_STATUS_CE1),
            PMC_REG_BITS_MASK(PWRGATE_STATUS_CE2),
            PMC_REG_BITS_MASK(PWRGATE_STATUS_CE3),
        };

        constinit const APBDEV_PMC_PWRGATE_TOGGLE_PARTID CpuPowerGateTogglePartitionIds[NumCores] = {
            APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE0,
            APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE1,
            APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE2,
            APBDEV_PMC_PWRGATE_TOGGLE_PARTID_CE3,
        };

        bool IsCpuPoweredOn(const reg::BitsMask mask) {
            return reg::HasValue(PMC + APBDEV_PMC_PWRGATE_STATUS, REG_BITS_VALUE_FROM_MASK(mask, APBDEV_PMC_PWRGATE_STATUS_STATUS_ON));
        }

        void PowerOnCpu(const reg::BitsMask mask, u32 toggle_partid) {
            /* If the cpu is already on, we have nothing to do. */
            if (IsCpuPoweredOn(mask)) {
                return;
            }

            /* Wait until nothing is being powergated. */
            int timeout = 5000;
            while (true) {
                if (reg::HasValue(PMC + APBDEV_PMC_PWRGATE_TOGGLE, PMC_REG_BITS_ENUM(PWRGATE_TOGGLE_START, DISABLE))) {
                    break;
                }

                util::WaitMicroSeconds(1);
                if ((--timeout) < 0) {
                    /* NOTE: Nintendo doesn't do any error handling here... */
                    return;
                }
            }

            /* Toggle on the cpu partition. */
            reg::Write(PMC + APBDEV_PMC_PWRGATE_TOGGLE, PMC_REG_BITS_ENUM (PWRGATE_TOGGLE_START,         ENABLE),
                                                        PMC_REG_BITS_VALUE(PWRGATE_TOGGLE_PARTID, toggle_partid));

            /* Wait up to 5000 us for the powergate to complete. */
            timeout = 5000;
            while (true) {
                if (IsCpuPoweredOn(mask)) {
                    break;
                }

                util::WaitMicroSeconds(1);
                if ((--timeout) < 0) {
                    /* NOTE: Nintendo doesn't do any error handling here... */
                    return;
                }
            }
        }

        void ResetCpu(int which_core) {
            reg::Write(CLK_RST + CLK_RST_CONTROLLER_RST_CPUG_CMPLX_SET, REG_BITS_VALUE(which_core + 0x00, 1, 1),  /* CPURESETn */
                                                                        REG_BITS_VALUE(which_core + 0x10, 1, 1)); /* CORERESETn */
        }

        void StartCpu(int which_core) {
            reg::Write(CLK_RST + CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR, REG_BITS_VALUE(which_core + 0x00, 1, 1),  /* CPURESETn */
                                                                        REG_BITS_VALUE(which_core + 0x10, 1, 1)); /* CORERESETn */
        }

        void PowerOffCpuImpl() {
            /* Get the current core id. */
            const auto core_id = hw::GetCurrentCoreId();

            /* Configure the flow controller to prepare for shutting down the current core. */
            flow::SetCpuCsr(core_id, FLOW_CTLR_CPUN_CSR_ENABLE_EXT_POWERGATE_CPU_ONLY);
            flow::SetHaltCpuEvents(core_id, false);
            flow::SetCc4Ctrl(core_id, 0);

            /* Save the core's context for restoration on next power-on. */
            SaveDebugRegisters();
            SetCoreOff();

            /* Ensure there are no pending memory transactions prior to our power-down. */
            FlushEntireDataCache();

            /* Finalize our powerdown and wait for an interrupt. */
            FinalizePowerOff();
        }

        void ValidateSocStateForSuspend() {
            /* Validate that all other cores are off. */
            AMS_ABORT_UNLESS(reg::HasValue(PMC + APBDEV_PMC_PWRGATE_STATUS, PMC_REG_BITS_VALUE(PWRGATE_STATUS_CE123, 0)));

            /* Validate that the bpmp is appropriately halted. */
            const bool jtag = IsJtagEnabled();
            AMS_ABORT_UNLESS(reg::Read(FLOW_CTLR + FLOW_CTLR_HALT_COP_EVENTS) == reg::Encode(FLOW_REG_BITS_ENUM    (HALT_COP_EVENTS_MODE, FLOW_MODE_STOP),
                                                                                             FLOW_REG_BITS_ENUM_SEL(HALT_COP_EVENTS_JTAG, jtag, ENABLED, DISABLED)));

            /* Further validations aren't guaranteed on < 6.0.0. */
            if (GetTargetFirmware() < TargetFirmware_6_0_0) {
                return;
            }

            /* Validate that USB2, APB-DMA, AHB-DMA are held in reset. */
            AMS_ABORT_UNLESS(reg::HasValue(CLK_RST + CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_REG_BITS_ENUM(RST_DEVICES_H_SWR_USB2_RST,   ENABLE),
                                                                                       CLK_RST_REG_BITS_ENUM(RST_DEVICES_H_SWR_APBDMA_RST, ENABLE),
                                                                                       CLK_RST_REG_BITS_ENUM(RST_DEVICES_H_SWR_AHBDMA_RST, ENABLE)));

            /* Validate that USBD is held in reset. */
            AMS_ABORT_UNLESS(reg::HasValue(CLK_RST + CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_REG_BITS_ENUM(RST_DEVICES_L_SWR_USBD_RST, ENABLE)));

            /* Validate that AHB-DMA, USB, USB2, COP are not allowed to arbitrate on the AHB. */
            AMS_ABORT_UNLESS(reg::HasValue(AHB_ARBC + AHB_ARBITRATION_DISABLE, AHB_REG_BITS_ENUM(ARBITRATION_DISABLE_COP,    DISABLE),
                                                                               AHB_REG_BITS_ENUM(ARBITRATION_DISABLE_AHBDMA, DISABLE),
                                                                               AHB_REG_BITS_ENUM(ARBITRATION_DISABLE_USB,    DISABLE),
                                                                               AHB_REG_BITS_ENUM(ARBITRATION_DISABLE_USB2,   DISABLE)));

            /* Validate that the GPIO controller has clock enabled. */
            AMS_ABORT_UNLESS(reg::HasValue(CLK_RST + CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_L_CLK_ENB_GPIO, ENABLE)));

            /* Validate that both FUSE and KFUSE have clock enabled. */
            AMS_ABORT_UNLESS(reg::HasValue(CLK_RST + CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_H_CLK_ENB_FUSE,  ENABLE),
                                                                                       CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_H_CLK_ENB_KFUSE, ENABLE)));

            /* Validate that all of IRAM has clock enabled. */
            AMS_ABORT_UNLESS(reg::HasValue(CLK_RST + CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_U_CLK_ENB_IRAMA, ENABLE),
                                                                                       CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_U_CLK_ENB_IRAMB, ENABLE),
                                                                                       CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_U_CLK_ENB_IRAMC, ENABLE),
                                                                                       CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_U_CLK_ENB_IRAMD, ENABLE)));

            /* Validate that ACTMON has clock enabled. */
            AMS_ABORT_UNLESS(reg::HasValue(CLK_RST + CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_V_CLK_ENB_ACTMON, ENABLE)));

            /* Validate that ENTROPY has clock enabled. */
            AMS_ABORT_UNLESS(reg::HasValue(CLK_RST + CLK_RST_CONTROLLER_CLK_OUT_ENB_W, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_W_CLK_ENB_ENTROPY, ENABLE)));
        }

        void GenerateCryptographicallyRandomBytes(void * const dst, int size) {
            /* Flush the region we're about to fill to ensure consistency with the SE. */
            hw::FlushDataCache(dst, size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Generate random bytes. */
            se::GenerateRandomBytes(dst, size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Flush to ensure the CPU sees consistent data for the region. */
            hw::FlushDataCache(dst, size);
            hw::DataSynchronizationBarrierInnerShareable();
        }

        void SaveSecureContextForErista() {
            /* Generate a random key source. */
            util::AlignedBuffer<hw::DataCacheLineSize, se::AesBlockSize> key_source;
            GenerateCryptographicallyRandomBytes(key_source, se::AesBlockSize);

            const u32 * const key_source_32 = reinterpret_cast<const u32 *>(static_cast<u8 *>(key_source));

            /* Ensure that the key source registers are not locked. */
            AMS_ABORT_UNLESS(pmc::GetSecureRegisterLockState(pmc::SecureRegister_KeySourceReadWrite) != pmc::LockState::Locked);

            /* Write the key source, lock writes to the key source, and verify that the key source is write-locked. */
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH24, key_source_32[0]);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH25, key_source_32[1]);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH26, key_source_32[2]);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH27, key_source_32[3]);
            pmc::LockSecureRegister(pmc::SecureRegister_KeySourceWrite);
            AMS_ABORT_UNLESS(pmc::GetSecureRegisterLockState(pmc::SecureRegister_KeySourceWrite) == pmc::LockState::Locked);

            /* Verify the key source is correct in registers, and read-lock the key source registers. */
            AMS_ABORT_UNLESS(reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH24) == key_source_32[0]);
            AMS_ABORT_UNLESS(reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH25) == key_source_32[1]);
            AMS_ABORT_UNLESS(reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH26) == key_source_32[2]);
            AMS_ABORT_UNLESS(reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH27) == key_source_32[3]);
            pmc::LockSecureRegister(pmc::SecureRegister_KeySourceRead);

            /* Ensure that the key source registers are locked. */
            AMS_ABORT_UNLESS(pmc::GetSecureRegisterLockState(pmc::SecureRegister_KeySourceReadWrite) == pmc::LockState::Locked);

            /* Generate a random kek into keyslot 2. */
            se::SetRandomKey(pkg1::AesKeySlot_TzramSaveKek);

            /* Verify that the se is in a validate state, context save, and validate again. */
            {
                se::ValidateErrStatus();
                ON_SCOPE_EXIT { se::ValidateErrStatus(); };

                {
                    /* Transition to non-secure mode for the duration of the context save operation. */
                    se::SetSecure(false);
                    ON_SCOPE_EXIT { se::SetSecure(true); };

                    /* Get a pointer to the context storage. */
                    se::Context * const context = MemoryRegionVirtualDramSecureDataStoreSecurityEngineState.GetPointer<se::Context>();
                    static_assert(MemoryRegionVirtualDramSecureDataStoreSecurityEngineState.GetSize() == sizeof(*context));

                    /* Save the context. */
                    se::SaveContext(context);

                    /* Ensure that the cpu sees consistent data. */
                    hw::FlushDataCache(context, sizeof(*context));
                    hw::DataSynchronizationBarrierInnerShareable();

                    /* Write the context pointer to pmc scratch, so that the bootrom will restore it on wake. */
                    reg::Write(PMC + APBDEV_PMC_SCRATCH43, MemoryRegionPhysicalDramSecureDataStoreSecurityEngineState.GetAddress());
                }
            }

            /* Clear keyslot 3, and then derive the save key. */
            se::ClearAesKeySlot(pkg1::AesKeySlot_TzramSaveKey);
            se::SetEncryptedAesKey256(pkg1::AesKeySlot_TzramSaveKey, pkg1::AesKeySlot_TzramSaveKek, key_source, se::AesBlockSize);

            /* Declare a temporary block to be used as both iv and mac. */
            u32 temp_block[se::AesBlockSize / sizeof(u32)] = {};

            /* Ensure that the SE sees consistent data for tzram. */
            const void * const tzram_save_src = MemoryRegionVirtualTzramReadOnlyAlias.GetPointer<u8>() + MemoryRegionVirtualTzramVolatileData.GetSize() + MemoryRegionVirtualTzramVolatileStack.GetSize();
                  void * const tzram_save_dst = MemoryRegionVirtualIramSc7Work.GetPointer<void>();
            constexpr size_t TzramSaveSize    = MemoryRegionVirtualDramSecureDataStoreTzram.GetSize();

            hw::FlushDataCache(tzram_save_src, TzramSaveSize);
            hw::FlushDataCache(tzram_save_dst, TzramSaveSize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Encrypt tzram using our random key. */
            se::EncryptAes256Cbc(tzram_save_dst, TzramSaveSize, pkg1::AesKeySlot_TzramSaveKey, tzram_save_src, TzramSaveSize, temp_block, se::AesBlockSize);
            hw::FlushDataCache(tzram_save_dst, TzramSaveSize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Copy the data from work space to the secure storage destination. */
            void * const tzram_store_dst = MemoryRegionVirtualDramSecureDataStoreTzram.GetPointer<void>();
            std::memcpy(tzram_store_dst, tzram_save_dst, TzramSaveSize);
            hw::FlushDataCache(tzram_store_dst, TzramSaveSize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Compute cmac of tzram into our temporary block. */
            se::ComputeAes256Cmac(temp_block, se::AesBlockSize, pkg1::AesKeySlot_TzramSaveKey, tzram_save_src, TzramSaveSize);

            /* Ensure that the cmac registers are not locked. */
            AMS_ABORT_UNLESS(pmc::GetSecureRegisterLockState(pmc::SecureRegister_CmacReadWrite) != pmc::LockState::Locked);

            /* Write the cmac, lock writes to the cmac, and verify that the cmac is write-locked. */
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH112, temp_block[0]);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH113, temp_block[1]);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH114, temp_block[2]);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH115, temp_block[3]);
            pmc::LockSecureRegister(pmc::SecureRegister_CmacWrite);
            AMS_ABORT_UNLESS(pmc::GetSecureRegisterLockState(pmc::SecureRegister_CmacWrite) == pmc::LockState::Locked);

            /* Verify the key source is correct in registers, and read-lock the key source registers. */
            AMS_ABORT_UNLESS(reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH112) == temp_block[0]);
            AMS_ABORT_UNLESS(reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH113) == temp_block[1]);
            AMS_ABORT_UNLESS(reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH114) == temp_block[2]);
            AMS_ABORT_UNLESS(reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH115) == temp_block[3]);
            pmc::LockSecureRegister(pmc::SecureRegister_CmacRead);

            /* Ensure that the key source registers are locked. */
            AMS_ABORT_UNLESS(pmc::GetSecureRegisterLockState(pmc::SecureRegister_CmacReadWrite) == pmc::LockState::Locked);
        }

        void SaveSecureContextForMariko() {
            /* Save security engine context to TZRAM SE carveout (inaccessible to cpu). */
            se::SaveContextAutomatic();

            /* Save TZRAM to shadow-TZRAM in always-on power domain. */
            se::SaveTzramAutomatic();
        }

        void SaveSecureContext() {
            /* Save the appropriate secure context. */
            const auto soc_type = GetSocType();
            if (soc_type == fuse::SocType_Erista) {
                SaveSecureContextForErista();
            } else /* if (soc_type == fuse::SocType_Mariko) */ {
                SaveSecureContextForMariko();
            }

            /* Save the debug code. */
            #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
            {
                const void * const debug_code_src = MemoryRegionVirtualDebugCode.GetPointer<void>();
                      void * const debug_code_dst = MemoryRegionVirtualDramDebugDataStore.GetPointer<void>();
                std::memcpy(debug_code_dst, debug_code_src, MemoryRegionVirtualDebugCode.GetSize());
                hw::FlushDataCache(debug_code_dst, MemoryRegionVirtualDebugCode.GetSize());
                hw::DataSynchronizationBarrierInnerShareable();
            }
            #endif
        }

        void LoadAndStartSc7BpmpFirmware() {
            /* Set BPMP reset. */
            reg::Write(CLK_RST + CLK_RST_CONTROLLER_RST_DEV_L_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_L_SET_SET_COP_RST, ENABLE));

            /* Set the PMC as insecure, so that the BPMP firmware can access it. */
            reg::ReadWrite(APB_MISC + APB_MISC_SECURE_REGS_APB_SLAVE_SECURITY_ENABLE_REG0_0, SLAVE_SECURITY_REG_BITS_ENUM(0, PMC, DISABLE));

            /* Set the exception vectors for the bpmp. RESET should point to RESET, all others should point to generic exception/panic. */
            constexpr const u32 Sc7FirmwareResetVector = static_cast<u32>(MemoryRegionPhysicalIramSc7Firmware.GetAddress() + 0x0);
            constexpr const u32 Sc7FirmwarePanicVector = static_cast<u32>(MemoryRegionPhysicalIramSc7Firmware.GetAddress() + 0x4);

            reg::Write(EVP + EVP_COP_RESET_VECTOR,          Sc7FirmwareResetVector);
            reg::Write(EVP + EVP_COP_UNDEF_VECTOR,          Sc7FirmwarePanicVector);
            reg::Write(EVP + EVP_COP_SWI_VECTOR,            Sc7FirmwarePanicVector);
            reg::Write(EVP + EVP_COP_PREFETCH_ABORT_VECTOR, Sc7FirmwarePanicVector);
            reg::Write(EVP + EVP_COP_DATA_ABORT_VECTOR,     Sc7FirmwarePanicVector);
            reg::Write(EVP + EVP_COP_RSVD_VECTOR,           Sc7FirmwarePanicVector);
            reg::Write(EVP + EVP_COP_IRQ_VECTOR,            Sc7FirmwarePanicVector);
            reg::Write(EVP + EVP_COP_FIQ_VECTOR,            Sc7FirmwarePanicVector);

            /* Disable activity monitor bpmp monitoring, so that we don't panic upon bpmp wake. */
            actmon::StopMonitoringBpmp();

            /* Load the bpmp firmware. */
            void * const sc7fw_load_address = MemoryRegionVirtualIramSc7Firmware.GetPointer<void>();
            std::memcpy(sc7fw_load_address, sc7fw_bin, sc7fw_bin_size);
            hw::FlushDataCache(sc7fw_load_address, sc7fw_bin_size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Ensure that the bpmp firmware was loaded. */
            AMS_ABORT_UNLESS(crypto::IsSameBytes(sc7fw_load_address, sc7fw_bin, sc7fw_bin_size));

            /* Clear BPMP reset. */
            reg::Write(CLK_RST + CLK_RST_CONTROLLER_RST_DEV_L_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_L_CLR_CLR_COP_RST, ENABLE));

            /* Start the bpmp. */
            reg::Write(FLOW_CTLR + FLOW_CTLR_HALT_COP_EVENTS, FLOW_REG_BITS_ENUM(HALT_COP_EVENTS_MODE, FLOW_MODE_NONE));
        }

        void SaveSecureContextAndSuspend() {
            /* Ensure there are no pending memory transactions before we continue */
            FlushEntireDataCache();
            hw::DataSynchronizationBarrierInnerShareable();

            /* Save all secure context (security engine state + tzram). */
            SaveSecureContext();

            /* Load and start the sc7 firmware on the bpmp. */
            if (GetTargetFirmware() >= TargetFirmware_2_0_0) {
                LoadAndStartSc7BpmpFirmware();
            }

            /* Log our suspension. */
            /* NOTE: Nintendo only does this on dev, but we will always do it. */
            if (true /* !pkg1::IsProduction() */) {
                log::SendText("OYASUMI\n", 8);
                log::Flush();
            }

            /* If we're on erista, configure the bootrom to allow our custom warmboot firmware. */
            if (GetSocType() == fuse::SocType_Erista) {
                reg::Write(PMC + APBDEV_PMC_SCRATCH31, 0x2202E012);
                reg::Write(PMC + APBDEV_PMC_SCRATCH32, 0x6001DC28);
            }

            /* Finalize our powerdown and wait for an interrupt. */
            FinalizePowerOff();
        }

        SmcResult SuspendCpuImpl(SmcArguments &args) {
            /* Decode arguments. */
            const util::BitPack32 power_state = { static_cast<u32>(args.r[1]) };
            const uintptr_t       entry_point = args.r[2];
            const uintptr_t       context_id  = args.r[3];

            const auto state_type = power_state.Get<SuspendCpuPowerState::StateType>();
            const auto state_id   = power_state.Get<SuspendCpuPowerState::StateId>();

            const auto core_id = hw::GetCurrentCoreId();

            /* Validate arguments. */
            SMC_R_UNLESS(state_type == PowerStateType_PowerDown, PsciDenied);
            SMC_R_UNLESS(state_id   == PowerStateId_Sc7,         PsciDenied);

            /* Orchestrate charger transition to Hi-Z mode if needed. */
            if (IsChargerHiZModeEnabled()) {
                /* Ensure we can do comms over i2c-1. */
                clkrst::EnableI2c1Clock();

                /* If the charger isn't in hi-z mode, perform a transition. */
                if (!charger::IsHiZMode()) {
                    charger::EnterHiZMode();

                    /* Wait up to 50ms for the transition to complete. */
                    const auto start_time = util::GetMicroSeconds();
                    auto current_time = start_time;
                    while ((current_time - start_time) <= 50'000) {
                        if (auto intr_status = reg::Read(GPIO + 0x634); (intr_status & 1) == 0) {
                            /* Wait 256 us to ensure the transition completes. */
                            util::WaitMicroSeconds(256);
                            break;
                        }
                        current_time = util::GetMicroSeconds();
                    }
                }

                /* Disable i2c-1, since we're done communicating over it. */
                clkrst::DisableI2c1Clock();
            }

            /* Enable wake event detection. */
            pmc::EnableWakeEventDetection();

            /* Ensure that i2c-5 is usable for communicating with the pmic. */
            clkrst::EnableI2c5Clock();
            i2c::Initialize(i2c::Port_5);

            /* Orchestrate sleep entry with the pmic. */
            pmic::EnableSleep();

            /* Ensure that the soc is in a state valid for us to suspend. */
            if (GetTargetFirmware() >= TargetFirmware_2_0_0) {
                ValidateSocStateForSuspend();
            }

            /* Configure the pmc for sc7 entry. */
            pmc::ConfigureForSc7Entry();

            /* Configure the flow controller for sc7 entry. */
            flow::SetCc4Ctrl(core_id, 0);
            flow::SetHaltCpuEvents(core_id, false);
            flow::ClearL2FlushControl();
            flow::SetCpuCsr(core_id, FLOW_CTLR_CPUN_CSR_ENABLE_EXT_POWERGATE_CPU_TURNOFF_CPURAIL);

            /* Save the entry context. */
            SetEntryContext(core_id, entry_point, context_id);

            /* Configure the cpu context for reset. */
            SaveDebugRegisters();
            SetCoreOff();
            SetResetExpected(true);

            /* Switch to use the common smc stack (all other cores are off), and perform suspension. */
            PivotStackAndInvoke(reinterpret_cast<void *>(CommonSmcStackTop), SaveSecureContextAndSuspend);

            /* This code will never be reached. */
            __builtin_unreachable();
        }

    }

    void PowerOffCpu() {
        /* Get the current core id. */
        const auto core_id = hw::GetCurrentCoreId();

        /* Note that we're expecting a reset for the current core. */
        SetResetExpected(true);

        /* If we're on the final core, shut down directly. Otherwise, invoke with special stack. */
        if (core_id == NumCores - 1) {
            PowerOffCpuImpl();
        } else {
            PivotStackAndInvoke(GetCoreExceptionStackVirtual(), PowerOffCpuImpl);
        }

        /* This code will never be reached. */
        __builtin_unreachable();
    }

    SmcResult SmcPowerOffCpu(SmcArguments &args) {
        AMS_UNUSED(args);

        PowerOffCpu();
    }

    SmcResult SmcPowerOnCpu(SmcArguments &args) {
        /* Get and validate the core to power on. */
        const int which_core = args.r[1];
        SMC_R_UNLESS(0 <= which_core && which_core < NumCores, PsciInvalidParameters);

        /* Ensure the core isn't already on. */
        SMC_R_UNLESS(!IsCoreOn(which_core), PsciAlreadyOn);

        /* Save the entry context. */
        SetEntryContext(which_core, args.r[2], args.r[3]);

        /* Reset the cpu. */
        ResetCpu(which_core);

        /* Turn on the core. */
        PowerOnCpu(CpuPowerGateStatusMasks[which_core], CpuPowerGateTogglePartitionIds[which_core]);

        /* Start the core. */
        StartCpu(which_core);

        return SmcResult::PsciSuccess;
    }

    SmcResult SmcSuspendCpu(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, SuspendCpuImpl);
    }

    bool IsChargerHiZModeEnabled() {
        return g_charger_hi_z_mode_enabled;
    }

    void SetChargerHiZModeEnabled(bool en) {
        g_charger_hi_z_mode_enabled = en;
    }

}
