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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "sdmmc_sdmmc_controller.board.nintendo_nx.hpp"
#include "sdmmc_io_impl.board.nintendo_nx.hpp"
#include "sdmmc_timer.hpp"

namespace ams::sdmmc::impl {

    /* FOR REFERENCE: board-specific sdmmc registers. */
    //struct SdmmcRegisters {
    //    /* Standard registers. */
    //    volatile SdHostStandardRegisters sd_host_standard_registers;
    //
    //    /* Vendor specific registers */
    //    volatile uint32_t vendor_clock_cntrl;
    //    volatile uint32_t vendor_sys_sw_cntrl;
    //    volatile uint32_t vendor_err_intr_status;
    //    volatile uint32_t vendor_cap_overrides;
    //    volatile uint32_t vendor_boot_cntrl;
    //    volatile uint32_t vendor_boot_ack_timeout;
    //    volatile uint32_t vendor_boot_dat_timeout;
    //    volatile uint32_t vendor_debounce_count;
    //    volatile uint32_t vendor_misc_cntrl;
    //    volatile uint32_t max_current_override;
    //    volatile uint32_t max_current_override_hi;
    //    volatile uint32_t _0x12c[0x20];
    //    volatile uint32_t vendor_io_trim_cntrl;
    //
    //    /* Start of sdmmc2/sdmmc4 only */
    //    volatile uint32_t vendor_dllcal_cfg;
    //    volatile uint32_t vendor_dll_ctrl0;
    //    volatile uint32_t vendor_dll_ctrl1;
    //    volatile uint32_t vendor_dllcal_cfg_sta;
    //    /* End of sdmmc2/sdmmc4 only */
    //
    //    volatile uint32_t vendor_tuning_cntrl0;
    //    volatile uint32_t vendor_tuning_cntrl1;
    //    volatile uint32_t vendor_tuning_status0;
    //    volatile uint32_t vendor_tuning_status1;
    //    volatile uint32_t vendor_clk_gate_hysteresis_count;
    //    volatile uint32_t vendor_preset_val0;
    //    volatile uint32_t vendor_preset_val1;
    //    volatile uint32_t vendor_preset_val2;
    //    volatile uint32_t sdmemcomppadctrl;
    //    volatile uint32_t auto_cal_config;
    //    volatile uint32_t auto_cal_interval;
    //    volatile uint32_t auto_cal_status;
    //    volatile uint32_t io_spare;
    //    volatile uint32_t sdmmca_mccif_fifoctrl;
    //    volatile uint32_t timeout_wcoal_sdmmca;
    //    volatile uint32_t _0x1fc;
    //};

    DEFINE_SD_REG_BIT_ENUM(VENDOR_CLOCK_CNTRL_SPI_MODE_CLKEN_OVERRIDE, 2, NORMAL, OVERRIDE);
    DEFINE_SD_REG(VENDOR_CLOCK_CNTRL_TAP_VAL,  16, 8);
    DEFINE_SD_REG(VENDOR_CLOCK_CNTRL_TRIM_VAL, 24, 5);

    DEFINE_SD_REG(VENDOR_CAP_OVERRIDES_DQS_TRIM_VAL, 8, 6);

    DEFINE_SD_REG(VENDOR_IO_TRIM_CNTRL_SEL_VREG, 2, 1);

    DEFINE_SD_REG_BIT_ENUM(VENDOR_DLLCAL_CFG_CALIBRATE, 31, DISABLE, ENABLE);

    DEFINE_SD_REG_BIT_ENUM(VENDOR_DLLCAL_CFG_STA_DLL_CAL_ACTIVE, 31, DONE, RUNNING);

    DEFINE_SD_REG(VENDOR_TUNING_CNTRL0_MUL_M, 6, 7);
    DEFINE_SD_REG_THREE_BIT_ENUM(VENDOR_TUNING_CNTRL0_NUM_TUNING_ITERATIONS, 13, TRIES_40, TRIES_64, TRIES_128, TRIES_192, TRIES_256, RESERVED5, RESERVED6, RESERVED7);
    DEFINE_SD_REG_BIT_ENUM(VENDOR_TUNING_CNTRL0_TAP_VALUE_UPDATED_BY_HW, 17, NOT_UPDATED_BY_HW, UPDATED_BY_HW);

    DEFINE_SD_REG(SDMEMCOMPPADCTRL_SDMMC2TMC_CFG_SDMEMCOMP_VREF_SEL,  0, 4);
    DEFINE_SD_REG(SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD,            31, 1);

    DEFINE_SD_REG(AUTO_CAL_CONFIG_AUTO_CAL_PU_OFFSET, 0, 7);
    DEFINE_SD_REG(AUTO_CAL_CONFIG_AUTO_CAL_PD_OFFSET, 8, 7);
    DEFINE_SD_REG_BIT_ENUM(AUTO_CAL_CONFIG_AUTO_CAL_ENABLE, 29, DISABLED, ENABLED);
    DEFINE_SD_REG_BIT_ENUM(AUTO_CAL_CONFIG_AUTO_CAL_START,  31, DISABLED, ENABLED);

    DEFINE_SD_REG(AUTO_CAL_STATUS_AUTO_CAL_PULLUP, 0, 7);
    DEFINE_SD_REG_BIT_ENUM(AUTO_CAL_STATUS_AUTO_CAL_ACTIVE, 31, INACTIVE, ACTIVE);

    DEFINE_SD_REG_BIT_ENUM(IO_SPARE_SPARE_OUT_3, 19, TWO_CYCLE_DELAY, ONE_CYCLE_DELAY);

    namespace {

        constexpr inline u32 TuningCommandTimeoutMilliSeconds = 5;

        constexpr void GetDividerSetting(u32 *out_target_clock_frequency_khz, u16 *out_x, SpeedMode speed_mode) {
            switch (speed_mode) {
                case SpeedMode_MmcIdentification:
                    *out_target_clock_frequency_khz = 26000;
                    *out_x                          = 66;
                    break;
                case SpeedMode_MmcLegacySpeed:
                    *out_target_clock_frequency_khz = 26000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_MmcHighSpeed:
                    *out_target_clock_frequency_khz = 52000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_MmcHs200:
                    *out_target_clock_frequency_khz = 200000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_MmcHs400:
                    *out_target_clock_frequency_khz = 200000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_SdCardIdentification:
                    *out_target_clock_frequency_khz = 25000;
                    *out_x                          = 64;
                    break;
                case SpeedMode_SdCardDefaultSpeed:
                    *out_target_clock_frequency_khz = 25000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_SdCardHighSpeed:
                    *out_target_clock_frequency_khz = 50000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_SdCardSdr12:
                    *out_target_clock_frequency_khz = 25000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_SdCardSdr50:
                    *out_target_clock_frequency_khz = 100000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_SdCardSdr104:
                    *out_target_clock_frequency_khz = 200000;
                    *out_x                          = 1;
                    break;
                case SpeedMode_GcAsicFpgaSpeed:
                    *out_target_clock_frequency_khz = 40800;
                    *out_x                          = 1;
                    break;
                case SpeedMode_GcAsicSpeed:
                    *out_target_clock_frequency_khz = 200000;
                    *out_x                          = 2;
                    break;
                case SpeedMode_SdCardSdr25:
                case SpeedMode_SdCardDdr50:
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    namespace {

        #if defined(AMS_SDMMC_THREAD_SAFE)
            constinit os::SdkMutex g_soc_mutex;

            #define AMS_SDMMC_LOCK_SOC_MUTEX() std::scoped_lock lk(g_soc_mutex)

        #else

            #define AMS_SDMMC_LOCK_SOC_MUTEX()

        #endif

        constinit bool g_determined_soc = false;
        constinit bool g_is_soc_mariko  = false;

    }

    bool IsSocMariko() {
        if (!g_determined_soc) {
            /* Ensure we have exclusive access to the soc variables. */
            AMS_SDMMC_LOCK_SOC_MUTEX();

            /* Check the SocType. */
            #if defined(ATMOSPHERE_IS_EXOSPHERE)
            {
                g_is_soc_mariko = fuse::GetSocType() == fuse::SocType_Mariko;
            }
            #elif defined(ATMOSPHERE_IS_MESOSPHERE)
            {
                MESOSPHERE_TODO("Detect mariko via KSystemControl call?");
            }
            #elif defined(ATMOSPHERE_IS_STRATOSPHERE)
            {
                /* Connect to spl for the duration of our check. */
                spl::Initialize();
                ON_SCOPE_EXIT { spl::Finalize(); };

                g_is_soc_mariko = spl::GetSocType() == spl::SocType_Mariko;
            }
            #else
                #error "Unknown execution context for ams::sdmmc::impl::IsSocMariko"
            #endif

            /* Note that we determined the soc. */
            g_determined_soc = true;
        }

        return g_is_soc_mariko;
    }

    void SdmmcController::ReleaseReset(SpeedMode speed_mode) {
        /* Get the clock reset module. */
        const auto module = this->GetClockResetModule();

        /* If the module is available, disable clock. */
        if (ClockResetController::IsAvailable(module)) {
            SdHostStandardController::DisableDeviceClock();
            SdHostStandardController::EnsureControl();
        }

        /* Get the correct divider setting for the speed mode. */
        u32 target_clock_frequency_khz;
        u16 x;
        GetDividerSetting(std::addressof(target_clock_frequency_khz), std::addressof(x), speed_mode);

        /* Release reset. */
        ClockResetController::ReleaseReset(module, target_clock_frequency_khz);
    }

    void SdmmcController::AssertReset() {
        return ClockResetController::AssertReset(this->GetClockResetModule());
    }

    Result SdmmcController::StartupCore(BusPower bus_power) {
        /* Set schmitt trigger. */
        this->SetSchmittTrigger(bus_power);

        /* Select one-cycle delay version of cmd_oen. */
        reg::ReadWrite(this->sdmmc_registers->io_spare, SD_REG_BITS_ENUM(IO_SPARE_SPARE_OUT_3, ONE_CYCLE_DELAY));

        /* Select regulated reference voltage for trimmer and DLL supply. */
        reg::ReadWrite(this->sdmmc_registers->vendor_io_trim_cntrl, SD_REG_BITS_VALUE(VENDOR_IO_TRIM_CNTRL_SEL_VREG, 0));

        /* Configure outbound tap value. */
        reg::ReadWrite(this->sdmmc_registers->vendor_clock_cntrl, SD_REG_BITS_VALUE(VENDOR_CLOCK_CNTRL_TRIM_VAL, this->GetOutboundTapValue()));

        /* Configure SPI_MODE_CLKEN_OVERRIDE. */
        reg::ReadWrite(this->sdmmc_registers->vendor_clock_cntrl, SD_REG_BITS_ENUM(VENDOR_CLOCK_CNTRL_SPI_MODE_CLKEN_OVERRIDE, NORMAL));

        /* Set slew codes. */
        this->SetSlewCodes();

        /* Set vref sel. */
        reg::ReadWrite(this->sdmmc_registers->sdmemcomppadctrl, SD_REG_BITS_VALUE(SDMEMCOMPPADCTRL_SDMMC2TMC_CFG_SDMEMCOMP_VREF_SEL, this->GetVrefSelValue()));

        /* Perform drive strength calibration at the new power. */
        this->SetDriveCodeOffsets(bus_power);
        this->CalibrateDriveStrength(bus_power);

        /* Enable internal clock. */
        R_TRY(SdHostStandardController::EnableInternalClock());

        return ResultSuccess();
    }

    Result SdmmcController::SetClockTrimmer(SpeedMode speed_mode, u8 tap_value) {
        /* If speed mode is Hs400, set the dqs trim value. */
        if (speed_mode == SpeedMode_MmcHs400) {
            reg::ReadWrite(this->sdmmc_registers->vendor_cap_overrides, SD_REG_BITS_VALUE(VENDOR_CAP_OVERRIDES_DQS_TRIM_VAL, 40));
        }

        /* Configure tap value as updated by software. */
        reg::ReadWrite(this->sdmmc_registers->vendor_tuning_cntrl0, SD_REG_BITS_ENUM(VENDOR_TUNING_CNTRL0_TAP_VALUE_UPDATED_BY_HW, NOT_UPDATED_BY_HW));

        /* Set the inbound tap value. */
        reg::ReadWrite(this->sdmmc_registers->vendor_clock_cntrl, SD_REG_BITS_VALUE(VENDOR_CLOCK_CNTRL_TAP_VAL, tap_value));

        /* Reset the cmd/dat line. */
        R_TRY(SdHostStandardController::ResetCmdDatLine());

        return ResultSuccess();
    }

    u8 SdmmcController::GetCurrentTapValue() {
        return static_cast<u8>(reg::GetValue(this->sdmmc_registers->vendor_clock_cntrl, SD_REG_BITS_MASK(VENDOR_CLOCK_CNTRL_TAP_VAL)));
    }

    Result SdmmcController::CalibrateDll() {
        /* Check if we need to temporarily re-enable the device clock. */
        const bool clock_disabled = reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));

        /* Ensure that the clock is enabled for the period we're using it. */
        if (clock_disabled) {
            /* Turn on the clock. */
            reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));
        }
        ON_SCOPE_EXIT { if (clock_disabled) { reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE)); } };

        /* Begin calibration. */
        reg::ReadWrite(this->sdmmc_registers->vendor_dllcal_cfg, SD_REG_BITS_ENUM(VENDOR_DLLCAL_CFG_CALIBRATE, ENABLE));

        /* Wait up to 5ms for calibration to begin. */
        {
            ManualTimer timer(5);
            while (true) {
                /* If calibration is done, we're done. */
                if (!reg::HasValue(this->sdmmc_registers->vendor_dllcal_cfg, SD_REG_BITS_ENUM(VENDOR_DLLCAL_CFG_CALIBRATE, ENABLE))) {
                    break;
                }

                /* Otherwise, check if we've timed out. */
                R_UNLESS((timer.Update()), sdmmc::ResultSdmmcDllCalibrationSoftwareTimeout());
            }
        }

        /* Wait up to 10ms for calibration to complete. */
        {
            ManualTimer timer(10);
            while (true) {
                /* If calibration is done, we're done. */
                if (reg::HasValue(this->sdmmc_registers->vendor_dllcal_cfg_sta, SD_REG_BITS_ENUM(VENDOR_DLLCAL_CFG_STA_DLL_CAL_ACTIVE, DONE))) {
                    break;
                }

                /* Otherwise, check if we've timed out. */
                R_UNLESS((timer.Update()), sdmmc::ResultSdmmcDllApplicationSoftwareTimeout());
            }
        }

        return ResultSuccess();
    }

    Result SdmmcController::SetSpeedModeWithTapValue(SpeedMode speed_mode, u8 tap_value) {
        /* Check if we need to temporarily disable the device clock. */
        const bool clock_enabled = reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));

        /* Ensure that the clock is disabled for the period we're using it. */
        if (clock_enabled) {
            /* Turn off the clock. */
            reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));
        }

        /* Set clock trimmer. */
        /* NOTE: Nintendo does not re-enable the clock if this fails... */
        R_TRY(this->SetClockTrimmer(speed_mode, tap_value));

        /* Configure for the desired speed mode. */
        switch (speed_mode) {
            case SpeedMode_MmcIdentification:
            case SpeedMode_SdCardIdentification:
            case SpeedMode_MmcLegacySpeed:
            case SpeedMode_SdCardDefaultSpeed:
                /* Set as normal speed, 3.3V. */
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control,  SD_REG_BITS_ENUM(HOST_CONTROL_HIGH_SPEED_ENABLE, NORMAL_SPEED));
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_1_8V_SIGNALING_ENABLE, 3_3V_SIGNALING));
                break;
            case SpeedMode_MmcHighSpeed:
            case SpeedMode_SdCardHighSpeed:
                /* Set as high speed, 3.3V. */
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control,  SD_REG_BITS_ENUM(HOST_CONTROL_HIGH_SPEED_ENABLE, HIGH_SPEED));
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_1_8V_SIGNALING_ENABLE, 3_3V_SIGNALING));
                break;
            case SpeedMode_MmcHs200:
                /* Set as HS200, 1.8V. */
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_UHS_MODE_SELECT, HS200));
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_1_8V_SIGNALING_ENABLE, 1_8V_SIGNALING));
                break;
            case SpeedMode_MmcHs400:
                /* Set as HS400, 1.8V. */
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_UHS_MODE_SELECT, HS400));
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_1_8V_SIGNALING_ENABLE, 1_8V_SIGNALING));
                break;
            case SpeedMode_SdCardSdr12:
                /* Set as SDR12, 1.8V. */
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_UHS_MODE_SELECT, SDR12));
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_1_8V_SIGNALING_ENABLE, 1_8V_SIGNALING));
                break;
            case SpeedMode_SdCardSdr50:
            case SpeedMode_SdCardSdr104:
            case SpeedMode_GcAsicFpgaSpeed:
            case SpeedMode_GcAsicSpeed:
                /* Set as SDR104, 1.8V. */
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_UHS_MODE_SELECT, SDR104));
                reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_1_8V_SIGNALING_ENABLE, 1_8V_SIGNALING));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
        SdHostStandardController::EnsureControl();

        /* Get the divider setting. */
        u32 target_source_clock_frequency_khz;
        u16 x;
        GetDividerSetting(std::addressof(target_source_clock_frequency_khz), std::addressof(x), speed_mode);

        /* Set the clock frequency. */
        u32 actual_source_clock_frequency_khz;
        ClockResetController::SetClockFrequencyKHz(std::addressof(actual_source_clock_frequency_khz), this->GetClockResetModule(), target_source_clock_frequency_khz);

        /* Set the device clock frequency. */
        const u32 actual_device_clock_frequency_khz = util::DivideUp(actual_source_clock_frequency_khz, x);
        SdHostStandardController::SetDeviceClockFrequencyKHz(actual_device_clock_frequency_khz);

        /* Check that the divider is correct. */
        AMS_ABORT_UNLESS((x == 1) || util::IsAligned(x, 2));

        /* Write the divider val to clock control. */
        const u16 n = x / 2;
        const u16 upper_n = n >> 8;
        reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_VALUE(CLOCK_CONTROL_SDCLK_FREQUENCY_SELECT,                     n),
                                                                                        SD_REG_BITS_VALUE(CLOCK_CONTROL_UPPER_BITS_OF_SDCLK_FREQUENCY_SELECT, upper_n));

        /* Re-enable the clock, if we should. */
        if (clock_enabled) {
            /* Turn on the clock. */
            reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));
        }

        /* If speed mode is Hs400, calibrate dll. */
        if (speed_mode == SpeedMode_MmcHs400) {
            R_TRY(this->CalibrateDll());
        }

        /* Set the current speed mode. */
        this->current_speed_mode = speed_mode;

        return ResultSuccess();
    }

    Result SdmmcController::IssueTuningCommand(u32 command_index) {
        /* Check that we're not power saving enable. */
        AMS_ABORT_UNLESS(!SdHostStandardController::IsPowerSavingEnable());

        /* Wait until command inhibit is done. */
        R_TRY(SdHostStandardController::WaitWhileCommandInhibit(true));

        /* Set transfer for tuning. */
        SdHostStandardController::SetTransferForTuning();

        /* If necessary, clear interrupt and enable buffer read ready signal. */
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            this->ClearInterrupt();

            reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.normal_signal_enable, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_BUFFER_READ_READY, ENABLED));
        }
        #endif

        /* Set the buffer read ready enable, and read status to ensure it takes. */
        reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.normal_int_enable, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_BUFFER_READ_READY, ENABLED));
        reg::Write(this->sdmmc_registers->sd_host_standard_registers.normal_int_status, reg::Read(this->sdmmc_registers->sd_host_standard_registers.normal_int_status));

        /* Issue command with clock disabled. */
        reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));
        {
            SdHostStandardController::SetCommandForTuning(command_index);

            SdHostStandardController::EnsureControl();
            WaitMicroSeconds(1);
            SdHostStandardController::AbortTransaction();
        }
        reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));

        /* When we're done waiting, ensure that we clean up appropriately. */
        ON_SCOPE_EXIT {
            /* Clear the buffer read ready signal, if we should. */
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
            reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.normal_signal_enable, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_BUFFER_READ_READY, MASKED));
            #endif

            /* Clear the buffer read ready enable. */
            reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.normal_int_enable, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_BUFFER_READ_READY, MASKED));

            /* Wait 8 clocks to ensure configuration takes. */
            SdHostStandardController::EnsureControl();
            WaitClocks(8, SdHostStandardController::GetDeviceClockFrequencyKHz());
        };

        /* Wait for the command to finish. */
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            const auto result = SdHostStandardController::WaitInterrupt(TuningCommandTimeoutMilliSeconds);
            if (R_SUCCEEDED(result)) {
                /* If we succeeded, clear the interrupt. */
                reg::Write(this->sdmmc_registers->sd_host_standard_registers.normal_int_status, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_BUFFER_READ_READY, ENABLED));
                this->ClearInterrupt();
                return ResultSuccess();
            } else if (sdmmc::ResultWaitInterruptSoftwareTimeout::Includes(result)) {
                SdHostStandardController::AbortTransaction();
                return sdmmc::ResultIssueTuningCommandSoftwareTimeout();
            } else {
                return result;
            }
        }
        #else
        {
            SdHostStandardController::EnsureControl();
            ManualTimer timer(TuningCommandTimeoutMilliSeconds);
            while (true) {
                /* Check if we received the interrupt. */
                if (reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.normal_int_status, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_BUFFER_READ_READY, ENABLED))) {
                    /* If we did, acknowledge it. */
                    reg::Write(this->sdmmc_registers->sd_host_standard_registers.normal_int_status, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_BUFFER_READ_READY, ENABLED));
                    return ResultSuccess();
                }

                /* Otherwise, check if we timed out. */
                if (!timer.Update()) {
                    SdHostStandardController::AbortTransaction();
                    return sdmmc::ResultIssueTuningCommandSoftwareTimeout();
                }
            }
        }
        #endif
    }

    void SdmmcController::SetDriveCodeOffsets(BusPower bus_power) {
        /* Get the offsets. */
        u8 pd, pu;
        this->GetAutoCalOffsets(std::addressof(pd), std::addressof(pu), bus_power);

        /* Set the offsets. */
        reg::ReadWrite(this->sdmmc_registers->auto_cal_config, SD_REG_BITS_VALUE(AUTO_CAL_CONFIG_AUTO_CAL_PD_OFFSET, pd),
                                                               SD_REG_BITS_VALUE(AUTO_CAL_CONFIG_AUTO_CAL_PU_OFFSET, pu));

        /* Wait for 1ms to ensure that our configuration takes. */
        /* NOTE/HACK: Nintendo does not (need) to do this, but they use SvcSleepThread for waits. */
        /* It's unclear why this wait is necessary, but not doing it causes drive strength calibration to fail if done immediately afterwards. */
        util::WaitMicroSeconds(1000);
    }

    void SdmmcController::CalibrateDriveStrength(BusPower bus_power) {
        /* Reset drive strength calibration status. */
        this->drive_strength_calibration_status = sdmmc::ResultDriveStrengthCalibrationNotCompleted();

        /* Check if we need to temporarily disable the device clock. */
        const bool clock_enabled = reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));

        /* Ensure that the clock is disabled for the period we're using it. */
        if (clock_enabled) {
            /* Turn off the clock. */
            reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));
        }

        /* Calibrate with the clock disabled. */
        {
            /* Set SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD. */
            if (reg::HasValue(this->sdmmc_registers->sdmemcomppadctrl, SD_REG_BITS_VALUE(SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD, 0))) {
                reg::ReadWrite(this->sdmmc_registers->sdmemcomppadctrl, SD_REG_BITS_VALUE(SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD, 1));
                SdHostStandardController::EnsureControl();
                WaitMicroSeconds(1);
            }

            /* Calibrate with SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD set. */
            {
                /* Begin autocal. */
                reg::ReadWrite(this->sdmmc_registers->auto_cal_config, SD_REG_BITS_ENUM(AUTO_CAL_CONFIG_AUTO_CAL_START,  ENABLED),
                                                                       SD_REG_BITS_ENUM(AUTO_CAL_CONFIG_AUTO_CAL_ENABLE, ENABLED));
                SdHostStandardController::EnsureControl();
                WaitMicroSeconds(2);

                /* Wait up to 10ms for auto cal to complete. */
                ManualTimer timer(10);
                while (true) {
                    /* Check if auto cal is inactive. */
                    if (reg::HasValue(this->sdmmc_registers->auto_cal_status, SD_REG_BITS_ENUM(AUTO_CAL_STATUS_AUTO_CAL_ACTIVE, INACTIVE))) {
                        /* Check the pullup status. */
                        const u32 pullup = (reg::GetValue(this->sdmmc_registers->auto_cal_status, SD_REG_BITS_MASK(AUTO_CAL_STATUS_AUTO_CAL_PULLUP))) & 0x1F;
                        if (pullup == 0x1F) {
                            this->drive_strength_calibration_status = sdmmc::ResultSdmmcCompShortToGnd();
                        }
                        if (pullup == 0) {
                            this->drive_strength_calibration_status = sdmmc::ResultSdmmcCompOpen();
                        }

                        break;
                    }

                    /* Otherwise, check if we've timed out. */
                    if (!timer.Update()) {
                        this->drive_strength_calibration_status = sdmmc::ResultDriveStrengthCalibrationSoftwareTimeout();

                        this->SetDriveStrengthToDefaultValues(bus_power);
                        reg::ReadWrite(this->sdmmc_registers->auto_cal_config, SD_REG_BITS_ENUM(AUTO_CAL_CONFIG_AUTO_CAL_ENABLE, DISABLED));
                        break;
                    }
                }
            }

            /* Clear SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD. */
            reg::ReadWrite(this->sdmmc_registers->sdmemcomppadctrl, SD_REG_BITS_VALUE(SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD, 0));
        }

        /* Re-enable the clock, if we should. */
        if (clock_enabled) {
            /* Turn on the clock. */
            reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));
        }

        /* If calibration didn't receive a replacement error, set internal state to success. */
        if (sdmmc::ResultDriveStrengthCalibrationNotCompleted::Includes(this->drive_strength_calibration_status)) {
            this->drive_strength_calibration_status = ResultSuccess();
        }
    }

    Result SdmmcController::Startup(BusPower bus_power, BusWidth bus_width, SpeedMode speed_mode, bool power_saving_enable) {
        /* Verify that we're awake. */
        AMS_ABORT_UNLESS(this->is_awake);

        /* Release the controller from reset. */
        this->ReleaseReset(speed_mode);

        /* Mark that we're not shutdown. */
        this->is_shutdown = false;

        /* Power on the controller. */
        R_TRY(this->PowerOn(bus_power));

        /* Start up for the specific power. */
        R_TRY(this->StartupCore(bus_power));

        /* Set our current power/width/speed. */
        SdHostStandardController::SetBusWidth(bus_width);
        SdHostStandardController::SetBusPower(bus_power);
        R_TRY(this->SetSpeedMode(speed_mode));
        this->SetPowerSaving(power_saving_enable);

        /* Enable clock to the device. */
        SdHostStandardController::EnableDeviceClock();

        /* Ensure that we can control the device. */
        SdHostStandardController::EnsureControl();

        return ResultSuccess();
    }

    void SdmmcController::Shutdown() {
        /* If we're already shut down, there's nothing to do. */
        if (this->is_shutdown) {
            return;
        }

        /* If we're currently awake, we need to disable clock/power. */
        if (this->is_awake) {
            SdHostStandardController::DisableDeviceClock();
            SdHostStandardController::SetBusPower(BusPower_Off);
            SdHostStandardController::EnsureControl();
        }

        /* Power off. */
        this->PowerOff();

        /* If awake, assert reset. */
        if (this->is_awake) {
            this->AssertReset();
        }

        /* Mark that we're shutdown. */
        this->is_shutdown = true;
    }

    void SdmmcController::PutToSleep() {
        /* If we're already shut down or asleep, there's nothing to do. */
        if (this->is_shutdown || !this->is_awake) {
            return;
        }

        /* Save values before sleep. */
        this->bus_power_before_sleep             = SdHostStandardController::GetBusPower();
        this->bus_width_before_sleep             = SdHostStandardController::GetBusWidth();
        this->speed_mode_before_sleep            = this->current_speed_mode;
        this->tap_value_before_sleep             = this->GetCurrentTapValue();
        this->is_powersaving_enable_before_sleep = SdHostStandardController::IsPowerSavingEnable();

        /* Disable clock/power to the device. */
        SdHostStandardController::DisableDeviceClock();
        SdHostStandardController::SetBusPower(BusPower_Off);
        SdHostStandardController::EnsureControl();

        /* Assert reset. */
        this->AssertReset();

        /* Mark that we're asleep. */
        this->is_awake = false;
    }

    Result SdmmcController::Awaken() {
        /* If we're shut down, or if we're awake already, there's nothing to do. */
        R_SUCCEED_IF(this->is_shutdown);
        R_SUCCEED_IF(this->is_awake);

        /* Mark that we're awake. */
        this->is_awake = true;

        /* Clear pad parked status. */
        this->ClearPadParked();

        /* Release reset. */
        this->ReleaseReset(this->speed_mode_before_sleep);

        /* Start up for the correct power. */
        R_TRY(this->StartupCore(this->bus_power_before_sleep));

        /* Configure values to what they were before sleep. */
        SdHostStandardController::SetBusWidth(this->bus_width_before_sleep);
        SdHostStandardController::SetBusPower(this->bus_power_before_sleep);
        R_TRY(this->SetSpeedModeWithTapValue(this->speed_mode_before_sleep, this->tap_value_before_sleep));
        this->SetPowerSaving(this->is_powersaving_enable_before_sleep);

        /* Enable clock to the device. */
        SdHostStandardController::EnableDeviceClock();
        SdHostStandardController::EnsureControl();

        return ResultSuccess();
    }

    Result SdmmcController::SwitchToSdr12() {
        /* Disable clock. */
        reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));

        /* Check that the dat lines are all low. */
        R_UNLESS(reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.present_state, SD_REG_BITS_VALUE(PRESENT_STATE_DAT0_3_LINE_SIGNAL_LEVEL, 0b0000)), sdmmc::ResultSdCardNotReadyToVoltageSwitch());

        /* Set Speed Mode. */
        R_TRY(this->SetSpeedMode(SpeedMode_SdCardSdr12));

        /* Set voltage to 1.8V. */
        SdHostStandardController::EnsureControl();
        R_TRY(this->LowerBusPower());
        this->SetSchmittTrigger(BusPower_1_8V);

        /* Perform drive strength calibration at the new power. */
        this->SetDriveCodeOffsets(BusPower_1_8V);
        this->CalibrateDriveStrength(BusPower_1_8V);

        /* Set the bus power in standard controller. */
        SdHostStandardController::SetBusPower(BusPower_1_8V);

        /* Wait up to 5ms for the switch to take. */
        SdHostStandardController::EnsureControl();
        WaitMicroSeconds(5000);

        /* Check that we switched to 1.8V. */
        R_UNLESS(reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_1_8V_SIGNALING_ENABLE, 1_8V_SIGNALING)), sdmmc::ResultSdHostStandardFailSwitchTo1_8V());

        /* Enable clock, and wait 1ms. */
        reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));
        SdHostStandardController::EnsureControl();
        WaitMicroSeconds(1000);

        /* Check that the dat lines are all high. */
        R_UNLESS(reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.present_state, SD_REG_BITS_VALUE(PRESENT_STATE_DAT0_3_LINE_SIGNAL_LEVEL, 0b1111)), sdmmc::ResultSdCardNotCompleteVoltageSwitch());

        return ResultSuccess();
    }

    Result SdmmcController::SetSpeedMode(SpeedMode speed_mode) {
        /* Get the tap value. */
        u8 tap_value;
        if (speed_mode == SpeedMode_MmcHs400) {
            AMS_ABORT_UNLESS(this->is_valid_tap_value_for_hs_400);
            tap_value = this->tap_value_for_hs_400;
        } else {
            tap_value = this->GetDefaultInboundTapValue();
        }

        /* Set the speed mode. */
        R_TRY(this->SetSpeedModeWithTapValue(speed_mode, tap_value));

        return ResultSuccess();
    }

    void SdmmcController::SetPowerSaving(bool en) {
        /* If necessary, calibrate the drive strength. */
        if (this->IsNeedPeriodicDriveStrengthCalibration() && !en && SdHostStandardController::IsDeviceClockEnable()) {
            this->CalibrateDriveStrength(SdHostStandardController::GetBusPower());
        }

        return SdHostStandardController::SetPowerSaving(en);
    }

    void SdmmcController::EnableDeviceClock() {
        /* If necessary, calibrate the drive strength. */
        if (this->IsNeedPeriodicDriveStrengthCalibration() && !SdHostStandardController::IsPowerSavingEnable()) {
            this->CalibrateDriveStrength(SdHostStandardController::GetBusPower());
        }

        return SdHostStandardController::EnableDeviceClock();
    }

    Result SdmmcController::IssueCommand(const Command *command, TransferData *xfer_data, u32 *out_num_transferred_blocks) {
        /* If necessary, calibrate the drive strength. */
        if (this->IsNeedPeriodicDriveStrengthCalibration() && SdHostStandardController::IsPowerSavingEnable()) {
            this->CalibrateDriveStrength(SdHostStandardController::GetBusPower());
        }

        return SdHostStandardController::IssueCommand(command, xfer_data, out_num_transferred_blocks);
    }

    Result SdmmcController::IssueStopTransmissionCommand(u32 *out_response) {
        /* If necessary, calibrate the drive strength. */
        if (this->IsNeedPeriodicDriveStrengthCalibration() && SdHostStandardController::IsPowerSavingEnable()) {
            this->CalibrateDriveStrength(SdHostStandardController::GetBusPower());
        }

        return SdHostStandardController::IssueStopTransmissionCommand(out_response);
    }

    Result SdmmcController::Tuning(SpeedMode speed_mode, u32 command_index) {
        /* Clear vendor tuning control 1. */
        reg::Write(this->sdmmc_registers->vendor_tuning_cntrl1, 0);

        /* Determine/configure the number of tries. */
        int num_tries;
        switch (speed_mode) {
            case SpeedMode_MmcHs200:
            case SpeedMode_MmcHs400:
            case SpeedMode_SdCardSdr104:
                num_tries      = 128;
                reg::ReadWrite(this->sdmmc_registers->vendor_tuning_cntrl0, SD_REG_BITS_ENUM(VENDOR_TUNING_CNTRL0_NUM_TUNING_ITERATIONS, TRIES_128));
                break;
            case SpeedMode_SdCardSdr50:
            case SpeedMode_GcAsicFpgaSpeed:
            case SpeedMode_GcAsicSpeed:
                num_tries      = 256;
                reg::ReadWrite(this->sdmmc_registers->vendor_tuning_cntrl0, SD_REG_BITS_ENUM(VENDOR_TUNING_CNTRL0_NUM_TUNING_ITERATIONS, TRIES_256));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Configure the multiplier. */
        reg::ReadWrite(this->sdmmc_registers->vendor_tuning_cntrl0, SD_REG_BITS_VALUE(VENDOR_TUNING_CNTRL0_MUL_M, 1));

        /* Configure tap value to be updated by hardware. */
        reg::ReadWrite(this->sdmmc_registers->vendor_tuning_cntrl0, SD_REG_BITS_ENUM(VENDOR_TUNING_CNTRL0_TAP_VALUE_UPDATED_BY_HW, UPDATED_BY_HW));

        /* Configure to execute tuning. */
        reg::ReadWrite(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_EXECUTE_TUNING, EXECUTE_TUNING));

        /* Perform tuning num_tries times. */
        for (int i = 0; /* ... */; ++i) {
            /* Check if we've been removed. */
            R_TRY(this->CheckRemoved());

            /* Issue the command. */
            this->IssueTuningCommand(command_index);

            /* Check if tuning is done. */
            if (i >= num_tries) {
                break;
            }

            if (reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_EXECUTE_TUNING, TUNING_COMPLETED))) {
                break;
            }
        }

        /* Check if we're using the tuned clock. */
        R_UNLESS(reg::HasValue(this->sdmmc_registers->sd_host_standard_registers.host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_SAMPLING_CLOCK, USING_TUNED_CLOCK)), sdmmc::ResultTuningFailed());

        return ResultSuccess();
    }

    void SdmmcController::SaveTuningStatusForHs400() {
        /* Save the current tap value. */
        this->tap_value_for_hs_400          = GetCurrentTapValue();
        this->is_valid_tap_value_for_hs_400 = true;
    }

    Result Sdmmc1Controller::PowerOnForRegisterControl(BusPower bus_power) {
        AMS_ABORT_UNLESS(bus_power == BusPower_3_3V);

        /* Nintendo sets the current bus power regardless of whether the call succeeds. */
        ON_SCOPE_EXIT { this->current_bus_power = BusPower_3_3V; };

        /* pcv::PowerOn(pcv::PowerControlTarget_SdCard, 3300000); */
        R_TRY(this->power_controller->PowerOn(BusPower_3_3V));

        return ResultSuccess();
    }

    void Sdmmc1Controller::PowerOffForRegisterControl() {
        /* If we're already off, there's nothing to do. */
        if (this->current_bus_power == BusPower_Off) {
            return;
        }

        /* If we're at 3.3V, lower to 1.8V. */
        if (this->current_bus_power == BusPower_3_3V) {
            /* pcv::ChangeVoltage(pcv::PowerControlTarget_SdCard, 1800000); */
            this->power_controller->LowerBusPower();

            /* Set our bus power. */
            this->current_bus_power = BusPower_1_8V;
        }

        /* pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1OutputHigh); */
        pinmux_impl::SetPinAssignment(pinmux_impl::PinAssignment_Sdmmc1OutputHigh);


        /* pcv::PowerOff(pcv::PowerControlTarget_SdCard); */
        this->power_controller->PowerOff();

        /* Set our bus power. */
        this->current_bus_power = BusPower_Off;

        /* pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1ResetState); */
        pinmux_impl::SetPinAssignment(pinmux_impl::PinAssignment_Sdmmc1ResetState);
    }

    Result Sdmmc1Controller::LowerBusPowerForRegisterControl() {
        /* Nintendo sets the current bus power regardless of whether the call succeeds. */
        ON_SCOPE_EXIT { this->current_bus_power = BusPower_1_8V; };

        /* pcv::ChangeVoltage(pcv::PowerControlTarget_SdCard, 1800000); */
        R_TRY(this->power_controller->LowerBusPower());

        return ResultSuccess();
    }

    void Sdmmc1Controller::SetSchmittTriggerForRegisterControl(BusPower bus_power) {
        SdHostStandardController::EnsureControl();

        if (IsSocMariko()) {
            /* pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1SchmtEnable); */
            pinmux_impl::SetPinAssignment(pinmux_impl::PinAssignment_Sdmmc1SchmtEnable);
        } else {
            switch (bus_power) {
                case BusPower_1_8V:
                    /* pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1SchmtEnable); */
                    pinmux_impl::SetPinAssignment(pinmux_impl::PinAssignment_Sdmmc1SchmtEnable);
                    break;
                case BusPower_3_3V:
                    /* pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1SchmtDisable); */
                    pinmux_impl::SetPinAssignment(pinmux_impl::PinAssignment_Sdmmc1SchmtDisable);
                    break;
                case BusPower_Off:
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }

    #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
    Result Sdmmc1Controller::PowerOnForPcvControl(BusPower bus_power) {
        AMS_ABORT_UNLESS(bus_power == BusPower_3_3V);

        /* Nintendo sets the current bus power regardless of whether the call succeeds. */
        ON_SCOPE_EXIT { this->current_bus_power = BusPower_3_3V; };

        /* TODO: return pcv::PowerOn(pcv::PowerControlTarget_SdCard, 3300000); */
        return ResultSuccess();
    }

    void Sdmmc1Controller::PowerOffForPcvControl() {
        /* If we're already off, there's nothing to do. */
        if (this->current_bus_power == BusPower_Off) {
            return;
        }

        /* If we're at 3.3V, lower to 1.8V. */
        {
            /* TODO: pcv::ChangeVoltage(pcv::PowerControlTarget_SdCard, 1800000); */
            this->current_bus_power = BusPower_1_8V;
        }

        /* TODO: pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1OutputHigh); */

        /* TODO: pcv::PowerOff(pcv::PowerControlTarget_SdCard); */
        this->current_bus_power = BusPower_Off;

        /* TODO: pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1ResetState); */

    }

    Result Sdmmc1Controller::LowerBusPowerForPcvControl() {
        /* Nintendo sets the current bus power regardless of whether the call succeeds. */
        ON_SCOPE_EXIT { this->current_bus_power = BusPower_1_8V; };

        /* TODO: return pcv::ChangeVoltage(pcv::PowerControlTarget_SdCard, 1800000); */
        return ResultSuccess();
    }

    void Sdmmc1Controller::SetSchmittTriggerForPcvControl(BusPower bus_power) {
        SdHostStandardController::EnsureControl();

        if (IsSocMariko()) {
            /* TODO: pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1SchmtEnable); */
        } else {
            switch (bus_power) {
                case BusPower_1_8V:
                    /* TODO: pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1SchmtEnable); */
                    break;
                case BusPower_3_3V:
                    /* TODO: pinmux::SetPinAssignment(std::addressof(this->pinmux_session), pinmux::PinAssignment_Sdmmc1SchmtDisable); */
                    break;
                case BusPower_Off:
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }
    }
    #endif

    Result Sdmmc1Controller::PowerOn(BusPower bus_power) {
        #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
        if (this->is_pcv_control) {
            return this->PowerOnForPcvControl(bus_power);
        } else
        #endif
        {
            return this->PowerOnForRegisterControl(bus_power);
        }
    }

    void Sdmmc1Controller::PowerOff() {
        #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
        if (this->is_pcv_control) {
            return this->PowerOffForPcvControl();
        } else
        #endif
        {
            return this->PowerOffForRegisterControl();
        }
    }

    Result Sdmmc1Controller::LowerBusPower() {
        #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
        if (this->is_pcv_control) {
            return this->LowerBusPowerForPcvControl();
        } else
        #endif
        {
            return this->LowerBusPowerForRegisterControl();
        }
    }

    void Sdmmc1Controller::SetSchmittTrigger(BusPower bus_power) {
        #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
        if (this->is_pcv_control) {
            return this->SetSchmittTriggerForPcvControl(bus_power);
        } else
        #endif
        {
            return this->SetSchmittTriggerForRegisterControl(bus_power);
        }
    }

    void Sdmmc1Controller::Initialize() {
        return this->InitializeForRegisterControl();
    }

    void Sdmmc1Controller::Finalize() {
        #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
        if (this->is_pcv_control) {
            return this->FinalizeForPcvControl();
        } else
        #endif
        {
            return this->FinalizeForRegisterControl();
        }
    }

    void Sdmmc1Controller::InitializeForRegisterControl() {
        #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
        /* Mark ourselves as initialized by register control. */
        this->is_pcv_control = false;
        #endif

        /* pinmux::Initialize(); */
        /* This just opens a session handle to pinmux service, no work to do. */

        /* pinmux::OpenSession(std::addressof(this->pinmux_session), pinmux::AssignablePinGroupName_Sdmmc1); */
        /* This just sets the session's internal value to the pin group name, so nothing to do here either. */

        /* pcv::Initialize(); */
        /* This initializes a lot of globals in pcv, most of which we don't care about. */
        /* However, we do care about the Sdmmc1PowerController. */
        AMS_ABORT_UNLESS(this->power_controller == nullptr);
        this->power_controller = new (GetPointer(this->power_controller_storage)) PowerController;

        /* Perform base initialization. */
        SdmmcController::Initialize();
    }

    void Sdmmc1Controller::FinalizeForRegisterControl() {
        /* Perform base finalization. */
        SdmmcController::Finalize();

        /* pcv::Finalize(); */
        /* As with initialize, we mostly don't care about the globals this touches. */
        /* However, we do want to finalize the Sdmmc1PowerController. */
        AMS_ABORT_UNLESS(this->power_controller != nullptr);
        this->power_controller->~PowerController();
        this->power_controller = nullptr;

        /* pinmux::CloseSession(std::addressof(this->pinmux_session)); */
        /* This does nothing. */

        /* pinmux::Finalize(); */
        /* This does nothing. */

        #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
        /* Mark ourselves as initialized by register control. */
        this->is_pcv_control = false;
        #endif
    }

    #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
    void Sdmmc1Controller::InitializeForPcvControl() {
        /* Mark ourselves as initialized by pcv control. */
        this->is_pcv_control = true;

        /* TODO: pinmux::Initialize(); */
        /* TODO: pinmux::OpenSession(std::addressof(this->pinmux_session), pinmux::AssignablePinGroupName_Sdmmc1); */
        /* TODO: pcv::Initialize(); */

        /* Perform base initialization. */
        SdmmcController::Initialize();
    }

    void Sdmmc1Controller::FinalizeForPcvControl() {
        /* Perform base finalization. */
        SdmmcController::Finalize();

        /* TODO: pcv::Finalize(); */
        /* TODO: pinmux::CloseSession(std::addressof(this->pinmux_session)); */
        /* TODO: pinmux::Finalize(); */

        /* Mark ourselves as initialized by register control. */
        this->is_pcv_control = false;
    }
    #endif

    namespace {

        constexpr inline dd::PhysicalAddress PmcRegistersPhysicalAddress = 0x7000E400;

    }

    Result Sdmmc1Controller::PowerController::ControlVddioSdmmc1(BusPower bus_power) {
        /* Configure appropriate voltage. */
        switch (bus_power) {
            case BusPower_Off:
                R_TRY(SetSdCardVoltageEnabled(false));
                break;
            case BusPower_1_8V:
                R_TRY(SetSdCardVoltageValue(1'800'000));
                R_TRY(SetSdCardVoltageEnabled(true));
                break;
            case BusPower_3_3V:
                R_TRY(SetSdCardVoltageValue(3'300'000));
                R_TRY(SetSdCardVoltageEnabled(true));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return ResultSuccess();
    }

    void Sdmmc1Controller::PowerController::SetSdmmcIoMode(bool is_3_3V) {
        /* Determine the address we're updating. */
        constexpr dd::PhysicalAddress ApbdevPmcPwrDetValAddress = PmcRegistersPhysicalAddress + APBDEV_PMC_PWR_DET_VAL;

        /* Read the current value. */
        u32 value = dd::ReadIoRegister(ApbdevPmcPwrDetValAddress);

        /* Mask out the existing bits. */
        value &= ~(reg::EncodeMask(PMC_REG_BITS_MASK(PWR_DET_VAL_SDMMC1)));

        /* ORR in the new bits. */
        value |= reg::Encode(PMC_REG_BITS_ENUM_SEL(PWR_DET_VAL_SDMMC1, is_3_3V, ENABLE, DISABLE));

        /* Write the new value. */
        dd::WriteIoRegister(ApbdevPmcPwrDetValAddress, value);

        /* Read the value back to be sure our write takes. */
        dd::ReadIoRegister(ApbdevPmcPwrDetValAddress);
    }

    void Sdmmc1Controller::PowerController::ControlRailSdmmc1Io(bool is_power_on) {
        /* Determine the address we're updating. */
        constexpr dd::PhysicalAddress ApbdevPmcNoIoPowerAddress = PmcRegistersPhysicalAddress + APBDEV_PMC_NO_IOPOWER;

        /* Read the current value. */
        u32 value = dd::ReadIoRegister(ApbdevPmcNoIoPowerAddress);

        /* Mask out the existing bits. */
        value &= ~(reg::EncodeMask(PMC_REG_BITS_MASK(NO_IOPOWER_SDMMC1)));

        /* ORR in the new bits. */
        value |= reg::Encode(PMC_REG_BITS_ENUM_SEL(NO_IOPOWER_SDMMC1, is_power_on, DISABLE, ENABLE));

        /* Write the new value. */
        dd::WriteIoRegister(ApbdevPmcNoIoPowerAddress, value);

        /* Read the value back to be sure our write takes. */
        dd::ReadIoRegister(ApbdevPmcNoIoPowerAddress);
    }

    Sdmmc1Controller::PowerController::PowerController() : current_bus_power(BusPower_Off) {
        /* gpio::Initialize(); */
        /* ... */

        /* Open gpio session. */
        /* gpio::OpenSession(std::addressof(this->gpio_pad_session), gpio::GpioPadName_PowSdEn); */
        gpio_impl::OpenSession(gpio_impl::GpioPadName_PowSdEn);

        /* Configure the gpio as low/output. */
        /* gpio::SetValue(std::addressof(this->gpio_pad_session), gpio::GpioValue_Low); */
        gpio_impl::SetValue(gpio_impl::GpioPadName_PowSdEn, gpio_impl::GpioValue_Low);

        /* gpio::SetDirection(std::addressof(this->gpio_pad_session), gpio::Direction_Output); */
        gpio_impl::SetDirection(gpio_impl::GpioPadName_PowSdEn, gpio_impl::Direction_Output);
    }

    Sdmmc1Controller::PowerController::~PowerController() {
        /* gpio::CloseSession(std::addressof(this->gpio_pad_session)); */
        gpio_impl::CloseSession(gpio_impl::GpioPadName_PowSdEn);

        /* gpio::Finalize(); */
        /* ... */
    }

    Result Sdmmc1Controller::PowerController::PowerOn(BusPower bus_power) {
        /* Bus power should be off, and if it's not we don't need to do anything. */
        AMS_ASSERT(this->current_bus_power == BusPower_Off);
        R_SUCCEED_IF(this->current_bus_power != BusPower_Off);

        /* Power on requires the target bus power be 3.3V. */
        AMS_ABORT_UNLESS(bus_power == BusPower_3_3V);

        /* Enable the rail. */
        this->ControlRailSdmmc1Io(true);

        /* Set the SD power GPIO to high. */
        /* gpio::SetValue(std::addressof(this->gpio_pad_session), gpio::GpioValue_High); */
        gpio_impl::SetValue(gpio_impl::GpioPadName_PowSdEn, gpio_impl::GpioValue_High);

        /* Wait 10ms for power change to take. */
        WaitMicroSeconds(10000);

        /* Configure Sdmmc1 IO as 3.3V. */
        this->SetSdmmcIoMode(true);
        R_TRY(this->ControlVddioSdmmc1(BusPower_3_3V));

        /* Wait 130 us for changes to take. */
        WaitMicroSeconds(130);

        /* Update our current bus power. */
        this->current_bus_power = bus_power;

        return ResultSuccess();
    }

    Result Sdmmc1Controller::PowerController::PowerOff() {
        /* Bus power should be on, and if it's not we don't need to do anything. */
        AMS_ASSERT(this->current_bus_power != BusPower_Off);
        R_SUCCEED_IF(this->current_bus_power == BusPower_Off);

        /* Bus power should be 1.8V. */
        /* NOTE: the result returned here is 0x8C0 (regulator::ResultIllegalRequest()) on newer firmwares. */
        AMS_ASSERT(this->current_bus_power == BusPower_1_8V);
        R_UNLESS(this->current_bus_power == BusPower_1_8V, pcv::ResultIllegalRequest());

        /* Disable vddio, and wait 4 ms. */
        this->ControlVddioSdmmc1(BusPower_Off);
        WaitMicroSeconds(4000);

        /* Set the SD power GPIO to low. */
        /* gpio::SetValue(std::addressof(this->gpio_pad_session), gpio::GpioValue_Low); */
        gpio_impl::SetValue(gpio_impl::GpioPadName_PowSdEn, gpio_impl::GpioValue_Low);

        /* Wait 239ms for the gpio config to take. */
        WaitMicroSeconds(239000);

        /* Disable the rail. */
        this->ControlRailSdmmc1Io(false);
        this->SetSdmmcIoMode(true);

        /* Update our current bus power. */
        this->current_bus_power = BusPower_Off;

        return ResultSuccess();
    }

    Result Sdmmc1Controller::PowerController::LowerBusPower() {
        /* Bus power should be 3.3V, and if it's not we don't need to do anything. */
        AMS_ASSERT(this->current_bus_power == BusPower_3_3V);
        R_SUCCEED_IF(this->current_bus_power != BusPower_3_3V);

        /* Configure as 1.8V, then wait 150us for it to take. */
        R_TRY(this->ControlVddioSdmmc1(BusPower_1_8V));
        WaitMicroSeconds(150);
        this->SetSdmmcIoMode(false);

        /* Update our current bus power. */
        this->current_bus_power = BusPower_1_8V;

        return ResultSuccess();
    }

}
