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
#pragma once
#include <vapours.hpp>
#include "sdmmc_sd_host_standard_controller.hpp"
#include "sdmmc_clock_reset_controller.hpp"

namespace ams::sdmmc::impl {

    bool IsSocMariko();

    constexpr inline size_t SdmmcRegistersSize = 0x200;

    constexpr inline dd::PhysicalAddress ApbMiscRegistersPhysicalAddress = UINT64_C(0x70000000);
    constexpr inline size_t ApbMiscRegistersSize = 16_KB;

    class SdmmcController : public SdHostStandardController {
        private:
            struct SdmmcRegisters {
                /* Standard registers. */
                volatile SdHostStandardRegisters sd_host_standard_registers;

                /* Vendor specific registers */
                volatile uint32_t vendor_clock_cntrl;
                volatile uint32_t vendor_sys_sw_cntrl;
                volatile uint32_t vendor_err_intr_status;
                volatile uint32_t vendor_cap_overrides;
                volatile uint32_t vendor_boot_cntrl;
                volatile uint32_t vendor_boot_ack_timeout;
                volatile uint32_t vendor_boot_dat_timeout;
                volatile uint32_t vendor_debounce_count;
                volatile uint32_t vendor_misc_cntrl;
                volatile uint32_t max_current_override;
                volatile uint32_t max_current_override_hi;
                volatile uint32_t _0x12c[0x20];
                volatile uint32_t vendor_io_trim_cntrl;

                /* Start of sdmmc2/sdmmc4 only */
                volatile uint32_t vendor_dllcal_cfg;
                volatile uint32_t vendor_dll_ctrl0;
                volatile uint32_t vendor_dll_ctrl1;
                volatile uint32_t vendor_dllcal_cfg_sta;
                /* End of sdmmc2/sdmmc4 only */

                volatile uint32_t vendor_tuning_cntrl0;
                volatile uint32_t vendor_tuning_cntrl1;
                volatile uint32_t vendor_tuning_status0;
                volatile uint32_t vendor_tuning_status1;
                volatile uint32_t vendor_clk_gate_hysteresis_count;
                volatile uint32_t vendor_preset_val0;
                volatile uint32_t vendor_preset_val1;
                volatile uint32_t vendor_preset_val2;
                volatile uint32_t sdmemcomppadctrl;
                volatile uint32_t auto_cal_config;
                volatile uint32_t auto_cal_interval;
                volatile uint32_t auto_cal_status;
                volatile uint32_t io_spare;
                volatile uint32_t sdmmca_mccif_fifoctrl;
                volatile uint32_t timeout_wcoal_sdmmca;
                volatile uint32_t _0x1fc;
            };
            static_assert(sizeof(SdmmcRegisters) == SdmmcRegistersSize);
        private:
            SdmmcRegisters *sdmmc_registers;
            bool is_shutdown;
            bool is_awake;
            SpeedMode current_speed_mode;
            BusPower bus_power_before_sleep;
            BusWidth bus_width_before_sleep;
            SpeedMode speed_mode_before_sleep;
            u8 tap_value_before_sleep;
            bool is_powersaving_enable_before_sleep;
            u8 tap_value_for_hs_400;
            bool is_valid_tap_value_for_hs_400;
            Result drive_strength_calibration_status;
        private:
            void ReleaseReset(SpeedMode speed_mode);
            void AssertReset();
            Result StartupCore(BusPower bus_power);
            Result SetClockTrimmer(SpeedMode speed_mode, u8 tap_value);
            u8 GetCurrentTapValue();
            Result CalibrateDll();
            Result SetSpeedModeWithTapValue(SpeedMode speed_mode, u8 tap_value);
            Result IssueTuningCommand(u32 command_index);
        protected:
            void SetDriveCodeOffsets(BusPower bus_power);
            void CalibrateDriveStrength(BusPower bus_power);

            virtual void SetPad() = 0;

            virtual ClockResetController::Module GetClockResetModule() const = 0;

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                virtual int GetInterruptNumber() const = 0;
                virtual os::InterruptEventType *GetInterruptEvent() const = 0;
            #endif

            virtual bool IsNeedPeriodicDriveStrengthCalibration() = 0;
            virtual void ClearPadParked() = 0;
            virtual Result PowerOn(BusPower bus_power) = 0;
            virtual void PowerOff() = 0;
            virtual Result LowerBusPower() = 0;
            virtual void SetSchmittTrigger(BusPower bus_power) = 0;
            virtual u8 GetOutboundTapValue() const = 0;
            virtual u8 GetDefaultInboundTapValue() const = 0;
            virtual u8 GetVrefSelValue() const = 0;
            virtual void SetSlewCodes() = 0;
            virtual void GetAutoCalOffsets(u8 *out_auto_cal_pd_offset, u8 *out_auto_cal_pu_offset, BusPower bus_power) const = 0;
            virtual void SetDriveStrengthToDefaultValues(BusPower bus_power) = 0;
        public:
            explicit SdmmcController(dd::PhysicalAddress registers_phys_addr) : SdHostStandardController(registers_phys_addr, SdmmcRegistersSize) {
                /* Set sdmmc registers. */
                static_assert(offsetof(SdmmcRegisters, sd_host_standard_registers) == 0);
                this->sdmmc_registers = reinterpret_cast<SdmmcRegisters *>(this->registers);

                this->is_shutdown                        = true;
                this->is_awake                           = true;
                this->is_valid_tap_value_for_hs_400      = false;
                this->drive_strength_calibration_status  = sdmmc::ResultDriveStrengthCalibrationNotCompleted();
                this->tap_value_for_hs_400               = 0;
                this->current_speed_mode                 = SpeedMode_MmcIdentification;
                this->bus_power_before_sleep             = BusPower_Off;
                this->bus_width_before_sleep             = BusWidth_1Bit;
                this->speed_mode_before_sleep            = SpeedMode_MmcIdentification;
                this->tap_value_before_sleep             = 0;
                this->is_powersaving_enable_before_sleep = false;
            }

            virtual void Initialize() override {
                /* Set pad. */
                this->SetPad();

                /* Initialize our clock/reset module. */
                ClockResetController::Initialize(this->GetClockResetModule());

                /* If necessary, initialize our interrupt event. */
                #if defined(AMS_SDMMC_USE_OS_EVENTS)
                {
                    os::InterruptEventType *interrupt_event = this->GetInterruptEvent();
                    os::InitializeInterruptEvent(interrupt_event, this->GetInterruptNumber(), os::EventClearMode_ManualClear);
                    SdHostStandardController::PreSetInterruptEvent(interrupt_event);
                }
                #endif

                /* Perform base initialization. */
                SdHostStandardController::Initialize();
            }

            virtual void Finalize() override {
                /* Perform base finalization. */
                SdHostStandardController::Finalize();

                /* If necessary, finalize our interrupt event. */
                #if defined(AMS_SDMMC_USE_OS_EVENTS)
                {
                    os::FinalizeInterruptEvent(this->GetInterruptEvent());
                }
                #endif

                /* Finalize our clock/reset module. */
                ClockResetController::Finalize(this->GetClockResetModule());
            }

            virtual Result Startup(BusPower bus_power, BusWidth bus_width, SpeedMode speed_mode, bool power_saving_enable) override;
            virtual void Shutdown() override;
            virtual void PutToSleep() override;
            virtual Result Awaken() override;
            virtual Result SwitchToSdr12() override;
            virtual Result SetSpeedMode(SpeedMode speed_mode) override;

            virtual SpeedMode GetSpeedMode() const override {
                return this->current_speed_mode;
            }

            virtual void SetPowerSaving(bool en) override;
            virtual void EnableDeviceClock() override;

            virtual Result IssueCommand(const Command *command, TransferData *xfer_data, u32 *out_num_transferred_blocks) override;
            virtual Result IssueStopTransmissionCommand(u32 *out_response) override;

            virtual bool IsSupportedTuning() const override {
                return true;
            }

            virtual Result Tuning(SpeedMode speed_mode, u32 command_index) override;
            virtual void SaveTuningStatusForHs400() override;

            virtual Result GetInternalStatus() const override {
                return this->drive_strength_calibration_status;
            }
    };

    constexpr inline dd::PhysicalAddress Sdmmc1RegistersPhysicalAddress = UINT64_C(0x700B0000);

    class Sdmmc1Controller : public SdmmcController {
        private:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                static constinit inline os::InterruptEventType s_interrupt_event{};
            #endif

            /* NOTE: This is a fascimile of pcv's Sdmmc1PowerController. */
            class PowerController {
                NON_COPYABLE(PowerController);
                NON_MOVEABLE(PowerController);
                private:
                    BusPower current_bus_power;
                private:
                    Result ControlVddioSdmmc1(BusPower bus_power);
                    void   SetSdmmcIoMode(bool is_3_3V);
                    void   ControlRailSdmmc1Io(bool is_power_on);
                public:
                    PowerController();
                    ~PowerController();

                    Result PowerOn(BusPower bus_power);
                    Result PowerOff();
                    Result LowerBusPower();
            };
        private:
            #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
            /* TODO: pinmux::PinmuxSession pinmux_session; */
            #endif
            BusPower current_bus_power;
            #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
            bool is_pcv_control;
            #endif
            TYPED_STORAGE(PowerController) power_controller_storage;
            PowerController *power_controller;
        private:
            Result PowerOnForRegisterControl(BusPower bus_power);
            void PowerOffForRegisterControl();
            Result LowerBusPowerForRegisterControl();
            void SetSchmittTriggerForRegisterControl(BusPower bus_power);

            #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
            Result PowerOnForPcvControl(BusPower bus_power);
            void PowerOffForPcvControl();
            Result LowerBusPowerForPcvControl();
            void SetSchmittTriggerForPcvControl(BusPower bus_power);
            #endif
        protected:
            virtual void SetPad() override {
                /* Nothing is needed here. */
            }

            virtual ClockResetController::Module GetClockResetModule() const override {
                return ClockResetController::Module_Sdmmc1;
            }

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
            virtual int GetInterruptNumber() const override {
                return 46;
            }

            virtual os::InterruptEventType *GetInterruptEvent() const override {
                return std::addressof(s_interrupt_event);
            }
            #endif

            virtual bool IsNeedPeriodicDriveStrengthCalibration() override {
                return !IsSocMariko();
            }

            virtual void ClearPadParked() override {
                /* Nothing is needed here. */
            }

            virtual Result PowerOn(BusPower bus_power) override;
            virtual void PowerOff() override;
            virtual Result LowerBusPower() override;

            virtual void SetSchmittTrigger(BusPower bus_power) override;

            virtual u8 GetOutboundTapValue() const override {
                if (IsSocMariko()) {
                    return 0xE;
                } else {
                    return 0x2;
                }
            }

            virtual u8 GetDefaultInboundTapValue() const override {
                if (IsSocMariko()) {
                    return 0xB;
                } else {
                    return 0x4;
                }
            }

            virtual u8 GetVrefSelValue() const override {
                if (IsSocMariko()) {
                    return 0x0;
                } else {
                    return 0x7;
                }
            }

            virtual void SetSlewCodes() override {
                if (IsSocMariko()) {
                    /* Do nothing. */
                } else {
                    /* Ensure that we can control registers. */
                    SdHostStandardController::EnsureControl();

                    /* Get the apb registers address. */
                    const uintptr_t apb_address = dd::QueryIoMapping(ApbMiscRegistersPhysicalAddress, ApbMiscRegistersSize);

                    /* Write the slew values to APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL. */
                    reg::ReadWrite(apb_address + APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL, APB_MISC_REG_BITS_VALUE(GP_SDMMC1_PAD_CFGPADCTRL_CFG2TMC_SDMMC1_CLK_CFG_CAL_DRVDN_SLWR, 0x1),
                                                                                    APB_MISC_REG_BITS_VALUE(GP_SDMMC1_PAD_CFGPADCTRL_CFG2TMC_SDMMC1_CLK_CFG_CAL_DRVDN_SLWF, 0x1));

                    /* Read to be sure our config takes. */
                    reg::Read(apb_address + APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL);
                }
            }

            virtual void GetAutoCalOffsets(u8 *out_auto_cal_pd_offset, u8 *out_auto_cal_pu_offset, BusPower bus_power) const override {
                /* Ensure that we can write the offsets. */
                AMS_ABORT_UNLESS(out_auto_cal_pd_offset != nullptr);
                AMS_ABORT_UNLESS(out_auto_cal_pu_offset != nullptr);

                /* Set the offsets. */
                if (IsSocMariko()) {
                    switch (bus_power) {
                        case BusPower_1_8V:
                            *out_auto_cal_pd_offset = 6;
                            *out_auto_cal_pu_offset = 6;
                            break;
                        case BusPower_3_3V:
                            *out_auto_cal_pd_offset = 0;
                            *out_auto_cal_pu_offset = 0;
                            break;
                        case BusPower_Off:
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                } else {
                    switch (bus_power) {
                        case BusPower_1_8V:
                            *out_auto_cal_pd_offset = 0x7B;
                            *out_auto_cal_pu_offset = 0x7B;
                            break;
                        case BusPower_3_3V:
                            *out_auto_cal_pd_offset = 0x7D;
                            *out_auto_cal_pu_offset = 0;
                            break;
                        case BusPower_Off:
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }
            }

            virtual void SetDriveStrengthToDefaultValues(BusPower bus_power) override {
                /* Ensure that we can control registers. */
                SdHostStandardController::EnsureControl();

                /* Get the apb registers address. */
                const uintptr_t apb_address = dd::QueryIoMapping(ApbMiscRegistersPhysicalAddress, ApbMiscRegistersSize);

                /* Determine the drive code values. */
                u8 drvdn, drvup;
                if (IsSocMariko()) {
                    drvdn = 0x8;
                    drvup = 0x8;
                } else {
                    switch (bus_power) {
                        case BusPower_1_8V:
                            drvdn = 0xF;
                            drvup = 0xB;
                            break;
                        case BusPower_3_3V:
                            drvdn = 0xC;
                            drvup = 0xC;
                            break;
                        case BusPower_Off:
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }

                /* Write the drv up/down values to APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL. */
                reg::ReadWrite(apb_address + APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL, APB_MISC_REG_BITS_VALUE(GP_SDMMC1_PAD_CFGPADCTRL_CFG2TMC_SDMMC1_PAD_CAL_DRVDN, drvdn),
                                                                                APB_MISC_REG_BITS_VALUE(GP_SDMMC1_PAD_CFGPADCTRL_CFG2TMC_SDMMC1_PAD_CAL_DRVUP, drvup));

                /* Read to be sure our config takes. */
                reg::Read(apb_address + APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL);
            }
        public:
            Sdmmc1Controller() : SdmmcController(Sdmmc1RegistersPhysicalAddress) {
                this->current_bus_power = BusPower_Off;
                #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
                this->is_pcv_control = false;
                #endif
                this->power_controller = nullptr;
            }

            virtual void Initialize() override;
            virtual void Finalize() override;

            void InitializeForRegisterControl();
            void FinalizeForRegisterControl();

            #if defined(AMS_SDMMC_USE_PCV_CLOCK_RESET_CONTROL)
            void InitializeForPcvControl();
            void FinalizeForPcvControl();
            #endif

            virtual bool IsSupportedBusPower(BusPower bus_power) const override {
                switch (bus_power) {
                    case BusPower_Off:  return true;
                    case BusPower_1_8V: return true;
                    case BusPower_3_3V: return true;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            virtual bool IsSupportedBusWidth(BusWidth bus_width) const override {
                switch (bus_width) {
                    case BusWidth_1Bit: return true;
                    case BusWidth_4Bit: return true;
                    case BusWidth_8Bit: return false;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }
    };

    class Sdmmc2And4Controller : public SdmmcController {
        protected:
            virtual bool IsNeedPeriodicDriveStrengthCalibration() override {
                return false;
            }

            virtual Result PowerOn(BusPower bus_power) override {
                /* Power for SDMMC2/4 is assumed on, so we don't need to do anything. */
                AMS_UNUSED(bus_power);
                return ResultSuccess();
            }

            virtual void PowerOff() override {
                /* Power for SDMMC2/4 is assumed on, so we don't need to do anything. */
            }

            virtual Result LowerBusPower() override {
                AMS_ABORT("Sdmmc2And4Controller cannot lower bus power\n");
            }

            virtual void SetSchmittTrigger(BusPower bus_power) override {
                /* Do nothing. */
                AMS_UNUSED(bus_power);
            }

            virtual u8 GetOutboundTapValue() const override {
                if (IsSocMariko()) {
                    return 0xD;
                } else {
                    return 0x8;
                }
            }

            virtual u8 GetDefaultInboundTapValue() const override {
                if (IsSocMariko()) {
                    return 0xB;
                } else {
                    return 0x0;
                }
            }

            virtual u8 GetVrefSelValue() const override {
                return 0x7;
            }

            virtual void SetSlewCodes() override {
                /* Do nothing. */
            }

            virtual void GetAutoCalOffsets(u8 *out_auto_cal_pd_offset, u8 *out_auto_cal_pu_offset, BusPower bus_power) const override {
                /* Ensure that we can write the offsets. */
                AMS_ABORT_UNLESS(out_auto_cal_pd_offset != nullptr);
                AMS_ABORT_UNLESS(out_auto_cal_pu_offset != nullptr);

                /* Sdmmc2And4Controller only supports 1.8v. */
                AMS_ABORT_UNLESS(bus_power == BusPower_1_8V);

                /* Set the offsets. */
                *out_auto_cal_pd_offset = 5;
                *out_auto_cal_pu_offset = 5;
            }
        public:
            explicit Sdmmc2And4Controller(dd::PhysicalAddress registers_phys_addr) : SdmmcController(registers_phys_addr) {
                /* ... */
            }

            virtual bool IsSupportedBusPower(BusPower bus_power) const override {
                switch (bus_power) {
                    case BusPower_Off:  return true;
                    case BusPower_1_8V: return true;
                    case BusPower_3_3V: return false;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            virtual bool IsSupportedBusWidth(BusWidth bus_width) const override {
                switch (bus_width) {
                    case BusWidth_1Bit: return true;
                    case BusWidth_4Bit: return true;
                    case BusWidth_8Bit: return true;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }
    };

    constexpr inline dd::PhysicalAddress Sdmmc2RegistersPhysicalAddress = UINT64_C(0x700B0200);

    class Sdmmc2Controller : public Sdmmc2And4Controller {
        private:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                static constinit inline os::InterruptEventType s_interrupt_event{};
            #endif
        protected:
            virtual void SetPad() override {
                /* Nothing is needed here. */
            }

            virtual ClockResetController::Module GetClockResetModule() const override {
                return ClockResetController::Module_Sdmmc2;
            }

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
            virtual int GetInterruptNumber() const override {
                return 47;
            }

            virtual os::InterruptEventType *GetInterruptEvent() const override {
                return std::addressof(s_interrupt_event);
            }
            #endif

            virtual void ClearPadParked() override {
                if (IsSocMariko()) {
                    /* Nothing is needed here. */
                } else {
                    /* Get the apb registers address. */
                    const uintptr_t apb_address = dd::QueryIoMapping(ApbMiscRegistersPhysicalAddress, ApbMiscRegistersSize);

                    /* Clear all MISC2PMC_EMMC2_*_PARK bits. */
                    reg::ReadWrite(apb_address + APB_MISC_GP_EMMC2_PAD_CFGPADCTRL, APB_MISC_REG_BITS_VALUE(GP_EMMC2_PAD_CFGPADCTRL_MISC2PMC_EMMC2_ALL_PARK, 0));

                    /* Read to be sure our config takes. */
                    reg::Read(apb_address + APB_MISC_GP_EMMC2_PAD_CFGPADCTRL);
                }
            }

            virtual void SetDriveStrengthToDefaultValues(BusPower bus_power) override {
                /* SDMMC2 only supports 1.8v. */
                AMS_ABORT_UNLESS(bus_power == BusPower_1_8V);

                /* Ensure that we can control registers. */
                SdHostStandardController::EnsureControl();

                /* Get the apb registers address. */
                const uintptr_t apb_address = dd::QueryIoMapping(ApbMiscRegistersPhysicalAddress, ApbMiscRegistersSize);

                if (IsSocMariko()) {
                    /* Write the drv up/down values to APB_MISC_GP_SDMMC2_PAD_CFGPADCTRL. */
                    reg::ReadWrite(apb_address + APB_MISC_GP_SDMMC2_PAD_CFGPADCTRL, APB_MISC_REG_BITS_VALUE(GP_SDMMC2_PAD_CFGPADCTRL_CFG2TMC_SDMMC2_PAD_CAL_DRVDN, 0xA),
                                                                                    APB_MISC_REG_BITS_VALUE(GP_SDMMC2_PAD_CFGPADCTRL_CFG2TMC_SDMMC2_PAD_CAL_DRVUP, 0xA));

                    /* Read to be sure our config takes. */
                    reg::Read(apb_address + APB_MISC_GP_SDMMC2_PAD_CFGPADCTRL);
                } else {
                    /* Write the drv up/down values to APB_MISC_GP_EMMC4_PAD_CFGPADCTRL. */
                    reg::ReadWrite(apb_address + APB_MISC_GP_EMMC2_PAD_CFGPADCTRL, APB_MISC_REG_BITS_VALUE(GP_EMMC2_PAD_CFGPADCTRL_CFG2TMC_EMMC2_PAD_DRVDN_COMP, 0x10),
                                                                                   APB_MISC_REG_BITS_VALUE(GP_EMMC2_PAD_CFGPADCTRL_CFG2TMC_EMMC2_PAD_DRVUP_COMP, 0x10));

                    /* Read to be sure our config takes. */
                    reg::Read(apb_address + APB_MISC_GP_EMMC2_PAD_CFGPADCTRL);
                }
            }
        public:
            Sdmmc2Controller() : Sdmmc2And4Controller(Sdmmc2RegistersPhysicalAddress) {
                /* ... */
            }
    };

    constexpr inline dd::PhysicalAddress Sdmmc4RegistersPhysicalAddress = UINT64_C(0x700B0600);

    class Sdmmc4Controller : public Sdmmc2And4Controller {
        private:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                static constinit inline os::InterruptEventType s_interrupt_event{};
            #endif
        protected:
            virtual void SetPad() override {
                if (IsSocMariko()) {
                    /* Get the apb registers address. */
                    const uintptr_t apb_address = dd::QueryIoMapping(ApbMiscRegistersPhysicalAddress, ApbMiscRegistersSize);

                    /* Enable Schmitt Trigger in emmc4 iobrick. */
                    reg::ReadWrite(apb_address + APB_MISC_GP_EMMC4_PAD_CFGPADCTRL, APB_MISC_REG_BITS_ENUM(GP_EMMC4_PAD_CFGPADCTRL_CFG2TMC_EMMC4_PAD_E_SCH, ENABLE));

                    /* Clear CMD_PULLU, CLK_PULLD, DQS_PULLD. */
                    reg::ReadWrite(apb_address + APB_MISC_GP_EMMC4_PAD_PUPD_CFGPADCTRL, APB_MISC_REG_BITS_VALUE(GP_EMMC4_PAD_PUPD_CFGPADCTRL_CFG2TMC_EMMC4_PAD_CMD_PUPD_PULLU, 0),
                                                                                        APB_MISC_REG_BITS_VALUE(GP_EMMC4_PAD_PUPD_CFGPADCTRL_CFG2TMC_EMMC4_PAD_CLK_PUPD_PULLD, 0),
                                                                                        APB_MISC_REG_BITS_VALUE(GP_EMMC4_PAD_PUPD_CFGPADCTRL_CFG2TMC_EMMC4_PAD_DQS_PUPD_PULLD, 0));

                    /* Read again to be sure our config takes. */
                    reg::Read(apb_address + APB_MISC_GP_EMMC4_PAD_PUPD_CFGPADCTRL);
                } else {
                    /* On Erista, we can just leave the reset value intact. */
                }
            }

            virtual ClockResetController::Module GetClockResetModule() const override {
                return ClockResetController::Module_Sdmmc4;
            }

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
            virtual int GetInterruptNumber() const override {
                return 63;
            }

            virtual os::InterruptEventType *GetInterruptEvent() const override {
                return std::addressof(s_interrupt_event);
            }
            #endif

            virtual void ClearPadParked() override {
                /* Get the apb registers address. */
                const uintptr_t apb_address = dd::QueryIoMapping(ApbMiscRegistersPhysicalAddress, ApbMiscRegistersSize);

                /* Clear all MISC2PMC_EMMC4_*_PARK bits. */
                reg::ReadWrite(apb_address + APB_MISC_GP_EMMC4_PAD_CFGPADCTRL, APB_MISC_REG_BITS_VALUE(GP_EMMC4_PAD_CFGPADCTRL_MISC2PMC_EMMC4_ALL_PARK, 0));

                /* Read to be sure our config takes. */
                reg::Read(apb_address + APB_MISC_GP_EMMC4_PAD_CFGPADCTRL);
            }

            virtual void SetDriveStrengthToDefaultValues(BusPower bus_power) override {
                /* SDMMC4 only supports 1.8v. */
                AMS_ABORT_UNLESS(bus_power == BusPower_1_8V);

                /* Ensure that we can control registers. */
                SdHostStandardController::EnsureControl();

                /* Get the apb registers address. */
                const uintptr_t apb_address = dd::QueryIoMapping(ApbMiscRegistersPhysicalAddress, ApbMiscRegistersSize);

                /* Determine the drv up/down values. */
                u8 drvdn, drvup;
                if (IsSocMariko()) {
                    drvdn = 0xA;
                    drvup = 0xA;
                } else {
                    drvdn = 0x10;
                    drvup = 0x10;
                }

                /* Write the drv up/down values to APB_MISC_GP_EMMC4_PAD_CFGPADCTRL. */
                reg::ReadWrite(apb_address + APB_MISC_GP_EMMC4_PAD_CFGPADCTRL, APB_MISC_REG_BITS_VALUE(GP_EMMC4_PAD_CFGPADCTRL_CFG2TMC_EMMC4_PAD_DRVDN_COMP, drvdn),
                                                                               APB_MISC_REG_BITS_VALUE(GP_EMMC4_PAD_CFGPADCTRL_CFG2TMC_EMMC4_PAD_DRVUP_COMP, drvup));

                /* Read to be sure our config takes. */
                reg::Read(apb_address + APB_MISC_GP_EMMC4_PAD_CFGPADCTRL);
            }
        public:
            Sdmmc4Controller() : Sdmmc2And4Controller(Sdmmc4RegistersPhysicalAddress) {
                /* ... */
            }
    };

}
