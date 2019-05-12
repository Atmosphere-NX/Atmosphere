/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>

#include "boot_types.hpp"
#include "i2c_driver/i2c_types.hpp"

class Boot {
    public:
        static constexpr u32 GpioPhysicalBase = 0x6000D000;
        static constexpr u32 ApbMiscPhysicalBase = 0x70000000;
    public:
        /* Functions for actually booting. */
        static void ChangeGpioVoltageTo1_8v();
        static void SetInitialGpioConfiguration();
        static void CheckClock();
        static void DetectBootReason();
        static void ShowSplashScreen();
        static void CheckBatteryCharge();
        static void SetInitialClockConfiguration();
        static void ConfigurePinmux();
        static void SetInitialWakePinConfiguration();
        static void SetFanEnabled();
        static void CheckAndRepairBootImages();

        /* Power utilities. */
        static void RebootSystem();
        static void ShutdownSystem();

        /* Register Utilities. */
        static u32 ReadPmcRegister(u32 phys_addr);
        static void WritePmcRegister(u32 phys_addr, u32 value, u32 mask = UINT32_MAX);

        /* GPIO Utilities. */
        static u32 GpioConfigure(u32 gpio_pad_name);
        static u32 GpioSetDirection(u32 gpio_pad_name, GpioDirection dir);
        static u32 GpioSetValue(u32 gpio_pad_name, GpioValue val);

        /* Pinmux Utilities. */
        static u32 PinmuxUpdatePark(u32 pinmux_name);
        static u32 PinmuxUpdatePad(u32 pinmux_name, u32 config_val, u32 config_mask);
        static u32 PinmuxUpdateDrivePad(u32 pinmux_drivepad_name, u32 config_val, u32 config_mask);
        static u32 PinmuxDummyReadDrivePad(u32 pinmux_drivepad_name);
        static void ConfigurePinmuxInitialPads();
        static void ConfigurePinmuxInitialDrivePads();

        /* SPL Utilities. */
        static HardwareType GetHardwareType();
        static u32 GetBootReason();
        static bool IsRecoveryBoot();
        static bool IsMariko();

        /* I2C Utilities. */
        static Result ReadI2cRegister(I2cSessionImpl &session, u8 *dst, size_t dst_size, const u8 *cmd, size_t cmd_size);
        static Result WriteI2cRegister(I2cSessionImpl &session, const u8 *src, size_t src_size, const u8 *cmd, size_t cmd_size);
        static Result WriteI2cRegister(I2cSessionImpl &session, const u8 address, const u8 value);

        /* Splash Screen/Display utilities. */
        static void InitializeDisplay();
        static void ShowDisplay(size_t x, size_t y, size_t width, size_t height, const u32 *img);
        static void FinalizeDisplay();

        /* Battery Display utilities. */
        static void ShowLowBatteryIcon();
        static void StartShowChargingIcon(size_t battery_percentage, bool wait);
        static void EndShowChargingIcon();

        /* Calibration utilities. */
        static u16 GetCrc16(const void *data, size_t size);
        static u32 GetBatteryVersion();
        static u32 GetBatteryVendor();

        /* Wake pin utiliies. */
        static void SetWakeEventLevel(u32 index, u32 level);
        static void SetWakeEventEnabled(u32 index, bool enabled);
};
