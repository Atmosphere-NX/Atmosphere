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
#include <stratosphere.hpp>
#include "powctl_bq24193_driver.hpp"

namespace ams::powctl::impl::board::nintendo::nx {

    namespace bq24193 {

        constexpr inline u8 InputSourceControl                     = 0x00;
        constexpr inline u8 PowerOnConfiguration                   = 0x01;
        constexpr inline u8 ChargeCurrentControl                   = 0x02;
        constexpr inline u8 PreChargeTerminationCurrentControl     = 0x03;
        constexpr inline u8 ChargeVoltageControl                   = 0x04;
        constexpr inline u8 ChargeTerminationTimerControl          = 0x05;
        constexpr inline u8 IrCompensationThermalRegulationControl = 0x06;
        constexpr inline u8 MiscOperationControl                   = 0x07;
        constexpr inline u8 SystemStatus                           = 0x08;
        constexpr inline u8 Fault                                  = 0x09;
        constexpr inline u8 VendorPartRevisionStatus               = 0x0A;

        constexpr u8 EncodePreChargeCurrentLimit(int ma) {
            constexpr int Minimum = 128;
            constexpr int Maximum = 2048;
            ma = std::max(std::min(ma, Maximum), Minimum);

            return static_cast<u8>((static_cast<u32>(ma - Minimum) >> 7) << 4);
        }

        constexpr u8 EncodeTerminationCurrentLimit(int ma) {
            constexpr int Minimum = 128;
            constexpr int Maximum = 2048;
            ma = std::max(std::min(ma, Maximum), Minimum);

            return static_cast<u8>((static_cast<u32>(ma - Minimum) >> 7) << 0);
        }

        constexpr u8 EncodeMinimumSystemVoltageLimit(int mv) {
            constexpr int Minimum = 3000;
            constexpr int Maximum = 3700;
            mv = std::max(std::min(mv, Maximum), Minimum);

            return static_cast<u8>(((mv - Minimum) / 100) << 1);
        }

        constexpr u8 EncodeFastChargeCurrentLimit(int ma) {
            constexpr int Minimum = 512;
            constexpr int Maximum = 4544;
            ma = std::max(std::min(ma, Maximum), Minimum);

            return static_cast<u8>((static_cast<u32>(ma - Minimum) >> 6) << 2);
        }

        constexpr int DecodeFastChargeCurrentLimit(u8 reg) {
            constexpr int Minimum = 512;

            return Minimum + (static_cast<u32>(reg & 0xFC) << 4);
        }

        static_assert(DecodeFastChargeCurrentLimit(EncodeFastChargeCurrentLimit(512)) == 512);
        static_assert(DecodeFastChargeCurrentLimit(EncodeFastChargeCurrentLimit(4544)) == 4544);
        static_assert(DecodeFastChargeCurrentLimit(EncodeFastChargeCurrentLimit(576)) == 576);

        constexpr u8 EncodeChargeVoltageLimit(int mv) {
            constexpr int Minimum = 3504;
            constexpr int Maximum = 4400;
            mv = std::max(std::min(mv, Maximum), Minimum);

            return static_cast<u8>((static_cast<u32>(mv - Minimum) >> 4) << 2);
        }

        constexpr int DecodeChargeVoltageLimit(u8 reg) {
            constexpr int Minimum = 3504;

            return Minimum + (static_cast<u32>(reg & 0xFC) << 2);
        }

        static_assert(DecodeChargeVoltageLimit(EncodeChargeVoltageLimit(3504)) == 3504);
        static_assert(DecodeChargeVoltageLimit(EncodeChargeVoltageLimit(4400)) == 4400);
        static_assert(DecodeChargeVoltageLimit(EncodeChargeVoltageLimit(3520)) == 3520);

        constexpr u8 EncodeChargerConfiguration(bq24193::ChargerConfiguration cfg) {
            switch (cfg) {
                case ChargerConfiguration_ChargeDisable: return 0x00;
                case ChargerConfiguration_ChargeBattery: return 0x10;
                case ChargerConfiguration_Otg:           return 0x20;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        constexpr u8 EncodeWatchdogTimerSetting(int seconds) {
            if (seconds == 0) {
                return 0x00;
            } else if (seconds < 80) {
                return 0x10;
            } else if (seconds < 160) {
                return 0x20;
            } else {
                return 0x30;
            }
        }

        constexpr u8 EncodeBatteryCompensation(int mo) {
            constexpr int Minimum = 0;
            constexpr int Maximum = 70;
            mo = std::max(std::min(mo, Maximum), Minimum);

            return static_cast<u8>((static_cast<u32>(mo - Minimum) / 10) << 5);
        }

        constexpr int DecodeBatteryCompensation(u8 reg) {
            constexpr int Minimum = 0;

            return Minimum + (static_cast<u32>(reg & 0xE0) >> 5) * 10;
        }

        static_assert(DecodeBatteryCompensation(EncodeBatteryCompensation(0))  == 0);
        static_assert(DecodeBatteryCompensation(EncodeBatteryCompensation(70)) == 70);
        static_assert(DecodeBatteryCompensation(EncodeBatteryCompensation(30)) == 30);

        constexpr u8 EncodeVoltageClamp(int mv) {
            constexpr int Minimum = 0;
            constexpr int Maximum = 112;
            mv = std::max(std::min(mv, Maximum), Minimum);

            return static_cast<u8>((static_cast<u32>(mv - Minimum) >> 4) << 2);
        }

        constexpr int DecodeVoltageClamp(u8 reg) {
            constexpr int Minimum = 0;

            return Minimum + (static_cast<u32>(reg & 0x1C) << 2);
        }

        static_assert(DecodeVoltageClamp(EncodeVoltageClamp(0))   == 0);
        static_assert(DecodeVoltageClamp(EncodeVoltageClamp(112)) == 112);
        static_assert(DecodeVoltageClamp(EncodeVoltageClamp(64))  == 64);

        constexpr u8 EncodeInputCurrentLimit(int ma) {
            if (ma < 150) {
                return 0;
            } else if (ma < 500) {
                return 1;
            } else if (ma < 900) {
                return 2;
            } else if (ma < 1200) {
                return 3;
            } else if (ma < 1500) {
                return 4;
            } else if (ma < 2000) {
                return 5;
            } else if (ma < 3000) {
                return 6;
            } else{
                return 7;
            }
        }

        constexpr int DecodeInputCurrentLimit(u8 reg) {
            switch (reg & 0x07) {
                case 0: return  100;
                case 1: return  150;
                case 2: return  500;
                case 3: return  900;
                case 4: return 1200;
                case 5: return 1500;
                case 6: return 2000;
                case 7: return 3000;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(100)) == 100);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(150)) == 150);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(500)) == 500);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(900)) == 900);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(1200)) == 1200);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(1500)) == 1500);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(2000)) == 2000);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(3000)) == 3000);

        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(0)) == 100);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(9999)) == 3000);

        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(149)) == 100);
        static_assert(DecodeInputCurrentLimit(EncodeInputCurrentLimit(151)) == 150);

        constexpr u8 EncodeInputVoltageLimit(int mv) {
            constexpr int Minimum = 3880;
            constexpr int Maximum = 5080;
            mv = std::max(std::min(mv, Maximum), Minimum);

            return static_cast<u8>(((static_cast<u32>(mv - Minimum) / 80) & 0xF) << 3);
        }

        constexpr u8 EncodeBoostModeCurrentLimit(int ma) {
            return ma >= 1300 ? 1 : 0;
        }

        constexpr bq24193::ChargerStatus DecodeChargerStatus(u8 reg) {
            switch (reg & 0x30) {
                case 0x00: return bq24193::ChargerStatus_NotCharging;
                case 0x10: return bq24193::ChargerStatus_PreCharge;
                case 0x20: return bq24193::ChargerStatus_FastCharging;
                case 0x30: return bq24193::ChargerStatus_ChargeTerminationDone;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    namespace {

        ALWAYS_INLINE Result ReadWriteRegister(const i2c::I2cSession &session, u8 address, u8 mask, u8 value) {
            /* Read the current value. */
            u8 cur_val;
            R_TRY(i2c::ReadSingleRegister(session, address, std::addressof(cur_val)));

            /* Update the value. */
            const u8 new_val = (cur_val & ~mask) | (value & mask);
            R_TRY(i2c::WriteSingleRegister(session, address, new_val));

            return ResultSuccess();
        }

    }

    Result Bq24193Driver::InitializeSession() {
        /* Set fast charge current limit. */
        R_TRY(this->SetFastChargeCurrentLimit(512));

        /* Disable force 20 percent charge. */
        R_TRY(this->SetForce20PercentChargeCurrent(false));

        /* Set pre-charge current limit. */
        R_TRY(this->SetPreChargeCurrentLimit(128));

        /* Set termination current limit. */
        R_TRY(this->SetTerminationCurrentLimit(128));

        /* Set minimum system voltage limit. */
        R_TRY(this->SetMinimumSystemVoltageLimit(3000));

        /* Set watchdog timer setting. */
        R_TRY(this->SetWatchdogTimerSetting(0));

        /* Disable charging safety timer. */
        R_TRY(this->SetChargingSafetyTimerEnabled(false));

        /* Reset the watchdog timer. */
        R_TRY(this->ResetWatchdogTimer());

        return ResultSuccess();
    }

    Result Bq24193Driver::SetPreChargeCurrentLimit(int ma) {
        return ReadWriteRegister(this->i2c_session, bq24193::PreChargeTerminationCurrentControl, 0xF0, bq24193::EncodePreChargeCurrentLimit(ma));
    }

    Result Bq24193Driver::SetTerminationCurrentLimit(int ma) {
        return ReadWriteRegister(this->i2c_session, bq24193::PreChargeTerminationCurrentControl, 0x0F, bq24193::EncodeTerminationCurrentLimit(ma));
    }

    Result Bq24193Driver::SetMinimumSystemVoltageLimit(int mv) {
        return ReadWriteRegister(this->i2c_session, bq24193::PowerOnConfiguration, 0x0E, bq24193::EncodeMinimumSystemVoltageLimit(mv));
    }

    Result Bq24193Driver::SetChargingSafetyTimerEnabled(bool en) {
        return ReadWriteRegister(this->i2c_session, bq24193::ChargeTerminationTimerControl, 0x08, en ? 0x08 : 0x00);
    }

    Result Bq24193Driver::GetForce20PercentChargeCurrent(bool *out) {
        /* Get the register. */
        u8 val;
        R_TRY(i2c::ReadSingleRegister(this->i2c_session, bq24193::ChargeCurrentControl, std::addressof(val)));

        /* Extract the value. */
        *out = (val & 0x01) != 0;
        return ResultSuccess();
    }

    Result Bq24193Driver::SetForce20PercentChargeCurrent(bool en) {
        return ReadWriteRegister(this->i2c_session, bq24193::ChargeCurrentControl, 0x01, en ? 0x01 : 0x00);
    }

    Result Bq24193Driver::GetFastChargeCurrentLimit(int *out_ma) {
        /* Get the register. */
        u8 val;
        R_TRY(i2c::ReadSingleRegister(this->i2c_session, bq24193::ChargeCurrentControl, std::addressof(val)));

        /* Extract the value. */
        *out_ma = bq24193::DecodeFastChargeCurrentLimit(val);
        return ResultSuccess();
    }

    Result Bq24193Driver::SetFastChargeCurrentLimit(int ma) {
        return ReadWriteRegister(this->i2c_session, bq24193::ChargeCurrentControl, 0xFC, bq24193::EncodeFastChargeCurrentLimit(ma));
    }

    Result Bq24193Driver::GetChargeVoltageLimit(int *out_mv) {
        /* Get the register. */
        u8 val;
        R_TRY(i2c::ReadSingleRegister(this->i2c_session, bq24193::ChargeVoltageControl, std::addressof(val)));

        /* Extract the value. */
        *out_mv = bq24193::DecodeChargeVoltageLimit(val);
        return ResultSuccess();
    }

    Result Bq24193Driver::SetChargeVoltageLimit(int mv) {
        return ReadWriteRegister(this->i2c_session, bq24193::ChargeVoltageControl, 0xFC, bq24193::EncodeChargeVoltageLimit(mv));
    }

    Result Bq24193Driver::SetChargerConfiguration(bq24193::ChargerConfiguration cfg) {
        return ReadWriteRegister(this->i2c_session, bq24193::PowerOnConfiguration, 0x30, bq24193::EncodeChargerConfiguration(cfg));
    }

    Result Bq24193Driver::IsHiZEnabled(bool *out) {
        /* Get the register. */
        u8 val;
        R_TRY(i2c::ReadSingleRegister(this->i2c_session, bq24193::InputSourceControl, std::addressof(val)));

        /* Extract the value. */
        *out = (val & 0x80) != 0;
        return ResultSuccess();
    }

    Result Bq24193Driver::SetHiZEnabled(bool en) {
        return ReadWriteRegister(this->i2c_session, bq24193::InputSourceControl, 0x80, en ? 0x80 : 0x00);
    }

    Result Bq24193Driver::GetInputCurrentLimit(int *out_ma) {
        /* Get the register. */
        u8 val;
        R_TRY(i2c::ReadSingleRegister(this->i2c_session, bq24193::InputSourceControl, std::addressof(val)));

        /* Extract the value. */
        *out_ma = bq24193::DecodeInputCurrentLimit(val);
        return ResultSuccess();
    }

    Result Bq24193Driver::SetInputCurrentLimit(int ma) {
        return ReadWriteRegister(this->i2c_session, bq24193::InputSourceControl, 0x07, bq24193::EncodeInputCurrentLimit(ma));
    }

    Result Bq24193Driver::SetInputVoltageLimit(int mv) {
        return ReadWriteRegister(this->i2c_session, bq24193::InputSourceControl, 0x78, bq24193::EncodeInputVoltageLimit(mv));
    }

    Result Bq24193Driver::SetBoostModeCurrentLimit(int ma) {
        return ReadWriteRegister(this->i2c_session, bq24193::PowerOnConfiguration, 0x01, bq24193::EncodeBoostModeCurrentLimit(ma));
    }

    Result Bq24193Driver::GetChargerStatus(bq24193::ChargerStatus *out) {
        /* Get the register. */
        u8 val;
        R_TRY(i2c::ReadSingleRegister(this->i2c_session, bq24193::SystemStatus, std::addressof(val)));

        /* Extract the value. */
        *out = bq24193::DecodeChargerStatus(val);
        return ResultSuccess();
    }

    Result Bq24193Driver::ResetWatchdogTimer() {
        return ReadWriteRegister(this->i2c_session, bq24193::PowerOnConfiguration, 0x40, 0x40);
    }

    Result Bq24193Driver::SetWatchdogTimerSetting(int seconds) {
        return ReadWriteRegister(this->i2c_session, bq24193::ChargeTerminationTimerControl, 0x30, bq24193::EncodeWatchdogTimerSetting(seconds));
    }

    Result Bq24193Driver::GetBatteryCompensation(int *out_mo) {
        /* Get the register. */
        u8 val;
        R_TRY(i2c::ReadSingleRegister(this->i2c_session, bq24193::IrCompensationThermalRegulationControl, std::addressof(val)));

        /* Extract the value. */
        *out_mo = bq24193::DecodeBatteryCompensation(val);
        return ResultSuccess();
    }

    Result Bq24193Driver::SetBatteryCompensation(int mo) {
        return ReadWriteRegister(this->i2c_session, bq24193::IrCompensationThermalRegulationControl, 0xE0, bq24193::EncodeBatteryCompensation(mo));
    }

    Result Bq24193Driver::GetVoltageClamp(int *out_mv) {
        /* Get the register. */
        u8 val;
        R_TRY(i2c::ReadSingleRegister(this->i2c_session, bq24193::IrCompensationThermalRegulationControl, std::addressof(val)));

        /* Extract the value. */
        *out_mv = bq24193::DecodeVoltageClamp(val);
        return ResultSuccess();
    }

    Result Bq24193Driver::SetVoltageClamp(int mv) {
        return ReadWriteRegister(this->i2c_session, bq24193::IrCompensationThermalRegulationControl, 0x1C, bq24193::EncodeVoltageClamp(mv));
    }

}