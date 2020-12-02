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
#include "max77620.h"
#include "max7762x.h"

namespace ams::pmic {

    namespace {

        constexpr inline int I2cAddressEristaMax77621   = 0x1B;
        constexpr inline int I2cAddressMarikoMax77812_A = 0x31;
        constexpr inline int I2cAddressMarikoMax77812_B = 0x33;

        constexpr inline int I2cAddressMax77620Pmic     = 0x3C;

        /* https://github.com/Atmosphere-NX/Atmosphere/blob/master/emummc/source/power/max77620.h */
        /* https://github.com/Atmosphere-NX/Atmosphere/blob/master/emummc/source/power/max7762x.h */
        /* TODO: Find datasheet, link to it instead. */
        /* NOTE: Tentatively, Max77620 "mostly" matches https://datasheets.maximintegrated.com/en/ds/MAX77863.pdf. */
        /*       This does not contain Max77621 documentation, though. */
        constexpr inline int Max77620RegisterOnOffStat  = 0x15;
        constexpr inline int Max77620RegisterGpio0      = 0x36;
        constexpr inline int Max77620RegisterAmeGpio    = 0x40;
        constexpr inline int Max77620RegisterOnOffCnfg1 = 0x41;

        constexpr inline int Max77621RegisterVOut     = 0x00;
        constexpr inline int Max77621RegisterVOutDvc  = 0x01;
        constexpr inline int Max77621RegisterControl1 = 0x02;
        constexpr inline int Max77621RegisterControl2 = 0x03;


        /* https://datasheets.maximintegrated.com/en/ds/MAX77812.pdf */
        constexpr inline int Max77812RegisterEnCtrl = 0x06;
        constexpr inline int Max77812RegisterM4VOut = 0x26;

        void Max77620EnableGpio(int gpio) {
            u8 val;

            /* Clear the AE for the GPIO */
            if (i2c::Query(std::addressof(val), sizeof(val), i2c::Port_5, I2cAddressEristaMax77621, Max77620RegisterAmeGpio)) {
                val &= ~(1 << gpio);
                i2c::SendByte(i2c::Port_5, I2cAddressEristaMax77621, Max77620RegisterAmeGpio, val);
            }

            /* Set GPIO_DRV_PUSHPULL (bit 0), GPIO_OUTPUT_VAL_HIGH (bit 3). */
            i2c::SendByte(i2c::Port_5, I2cAddressEristaMax77621, Max77620RegisterGpio0 + gpio, MAX77620_CNFG_GPIO_DRV_PUSHPULL | MAX77620_CNFG_GPIO_OUTPUT_VAL_HIGH);
        }

        void SetEnBitErista() {
            i2c::SendByte(i2c::Port_5, I2cAddressEristaMax77621, Max77621RegisterVOut, MAX77621_VOUT_ENABLE);
        }

        void EnableVddCpuErista() {
            /* Enable GPIO 5. */
            /* TODO: What does this control? */
            Max77620EnableGpio(5);

            /* Configure Max77621 control registers. */
            i2c::SendByte(i2c::Port_5, I2cAddressEristaMax77621, Max77621RegisterControl1, MAX77621_AD_ENABLE | MAX77621_NFSR_ENABLE | MAX77621_SNS_ENABLE | MAX77621_RAMP_12mV_PER_US);
            i2c::SendByte(i2c::Port_5, I2cAddressEristaMax77621, Max77621RegisterControl2, MAX77621_T_JUNCTION_120 | MAX77621_WDTMR_ENABLE | MAX77621_CKKADV_TRIP_75mV_PER_US| MAX77621_INDUCTOR_NOMINAL);

            /* Configure Max77621 VOut to 0.95v */
            i2c::SendByte(i2c::Port_5, I2cAddressEristaMax77621, Max77621RegisterVOut,    MAX77621_VOUT_ENABLE | MAX77621_VOUT_0_95V);
            i2c::SendByte(i2c::Port_5, I2cAddressEristaMax77621, Max77621RegisterVOutDvc, MAX77621_VOUT_ENABLE | MAX77621_VOUT_0_95V);
        }

        void DisableVddCpuErista() {
            /* Disable Max77621 VOut. */
            i2c::SendByte(i2c::Port_5, I2cAddressEristaMax77621, Max77621RegisterVOut, MAX77621_VOUT_DISABLE);
        }

        int GetI2cAddressForMarikoMax77812(Regulator regulator) {
            switch (regulator) {
                case Regulator_Mariko_Max77812_A: return I2cAddressMarikoMax77812_A;
                case Regulator_Mariko_Max77812_B: return I2cAddressMarikoMax77812_B;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void SetEnBitMariko(Regulator regulator) {
            /* Set EN_M3_LPM to enable BUCK Master 3 low power mode. */
            i2c::SendByte(i2c::Port_5, GetI2cAddressForMarikoMax77812(regulator), Max77812RegisterEnCtrl, 0x40);
        }

        void EnableVddCpuMariko(Regulator regulator) {
            const int address = GetI2cAddressForMarikoMax77812(regulator);

            /* Set EN_M3_LPM to enable BUCK Master 3 low power mode. */
            u8 ctrl;
            if (i2c::Query(std::addressof(ctrl), sizeof(ctrl), i2c::Port_5, address, Max77812RegisterEnCtrl)) {
                ctrl |= 0x40;
                i2c::SendByte(i2c::Port_5, address, Max77812RegisterEnCtrl, ctrl);
            }

            /* Set BUCK Master 4 output voltage to 110. */
            i2c::SendByte(i2c::Port_5, address, Max77812RegisterM4VOut, 110);
        }

        void DisableVddCpuMariko(Regulator regulator) {
            const int address = GetI2cAddressForMarikoMax77812(regulator);

            /* Clear EN_M3_LPM to disable BUCK Master 3 low power mode. */
            u8 ctrl;
            if (i2c::Query(std::addressof(ctrl), sizeof(ctrl), i2c::Port_5, address, Max77812RegisterEnCtrl)) {
                ctrl &= ~0x40;
                i2c::SendByte(i2c::Port_5, address, Max77812RegisterEnCtrl, ctrl);
            }
        }

        u8 GetPmicOnOffStat() {
            return i2c::QueryByte(i2c::Port_5, I2cAddressMax77620Pmic, Max77620RegisterOnOffStat);
        }

    }

    void SetEnBit(Regulator regulator) {
        switch (regulator) {
            case Regulator_Erista_Max77621:
                return SetEnBitErista();
            case Regulator_Mariko_Max77812_A:
            case Regulator_Mariko_Max77812_B:
                return SetEnBitMariko(regulator);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void EnableVddCpu(Regulator regulator) {
        switch (regulator) {
            case Regulator_Erista_Max77621:
                return EnableVddCpuErista();
            case Regulator_Mariko_Max77812_A:
            case Regulator_Mariko_Max77812_B:
                return EnableVddCpuMariko(regulator);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void DisableVddCpu(Regulator regulator) {
        switch (regulator) {
            case Regulator_Erista_Max77621:
                return DisableVddCpuErista();
            case Regulator_Mariko_Max77812_A:
            case Regulator_Mariko_Max77812_B:
                return DisableVddCpuMariko(regulator);
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void EnableSleep() {
        /* Get the current onoff cfg. */
        u8 cnfg = i2c::QueryByte(i2c::Port_5, I2cAddressMax77620Pmic, Max77620RegisterOnOffCnfg1);

        /* Set SlpEn. */
        cnfg |= MAX77620_ONOFFCNFG1_SLPEN;

        /* Write the new cfg. */
        i2c::SendByte(i2c::Port_5, I2cAddressMax77620Pmic, Max77620RegisterOnOffCnfg1, cnfg);
    }

    void PowerOff() {
        /* Write power-off to onoff cfg. */
        i2c::SendByte(i2c::Port_5, I2cAddressMax77620Pmic, Max77620RegisterOnOffCnfg1, MAX77620_ONOFFCNFG1_PWR_OFF);
    }

    void ShutdownSystem(bool reboot) {
        /* Get value, set or clear software reset mask. */
        u8 on_off_2_val = i2c::QueryByte(i2c::Port_5, I2cAddressMax77620Pmic, MAX77620_REG_ONOFFCNFG2);
        if (reboot) {
            on_off_2_val |= MAX77620_ONOFFCNFG2_SFT_RST_WK;
        } else {
            on_off_2_val &= ~(MAX77620_ONOFFCNFG2_SFT_RST_WK);
        }
        i2c::SendByte(i2c::Port_5, I2cAddressMax77620Pmic, MAX77620_REG_ONOFFCNFG2, on_off_2_val);

        /* Get value, set software reset mask. */
        u8 on_off_1_val = i2c::QueryByte(i2c::Port_5, I2cAddressMax77620Pmic, MAX77620_REG_ONOFFCNFG1);
        on_off_1_val |= MAX77620_ONOFFCNFG1_SFT_RST;

        /* NOTE: Here, userland finalizes the battery on non-Calcio. */
        if (fuse::GetHardwareType() != fuse::HardwareType_Calcio) {
            /* ... */
        }

        /* Actually write the value to trigger shutdown/reset. */
        i2c::SendByte(i2c::Port_5, I2cAddressMax77620Pmic, MAX77620_REG_ONOFFCNFG1, on_off_1_val);

        /* Allow up to 5 seconds for shutdown/reboot to take place. */
        util::WaitMicroSeconds(5'000'000ul);

        AMS_ABORT("Shutdown failed");
    }

    bool IsAcOk() {
        return (GetPmicOnOffStat() & (1 << 1)) != 0;
    }

    bool IsPowerButtonPressed() {
        return (GetPmicOnOffStat() & (1 << 2)) != 0;
    }

}
