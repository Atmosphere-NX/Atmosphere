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
#include "powctl_max17050_driver.hpp"

#if defined(ATMOSPHERE_ARCH_ARM64)
#include <arm_neon.h>
#endif

namespace ams::powctl::impl::board::nintendo_nx {

    namespace max17050 {

        constexpr inline u8 Status           = 0x00;
        constexpr inline u8 VAlrtThreshold   = 0x01;
        constexpr inline u8 TAlrtThreshold   = 0x02;
        constexpr inline u8 SocAlrtThreshold = 0x03;
        constexpr inline u8 AtRate           = 0x04;
        constexpr inline u8 RemCapRep        = 0x05;
        constexpr inline u8 SocRep           = 0x06;
        constexpr inline u8 Age              = 0x07;
        constexpr inline u8 Temperature      = 0x08;
        constexpr inline u8 VCell            = 0x09;
        constexpr inline u8 Current          = 0x0A;
        constexpr inline u8 AverageCurrent   = 0x0B;

        constexpr inline u8 SocMix           = 0x0D;
        constexpr inline u8 SocAv            = 0x0E;
        constexpr inline u8 RemCapMix        = 0x0F;
        constexpr inline u8 FullCap          = 0x10;
        constexpr inline u8 Tte              = 0x11;
        constexpr inline u8 QResidual00      = 0x12;
        constexpr inline u8 FullSocThr       = 0x13;


        constexpr inline u8 AverageTemp      = 0x16;
        constexpr inline u8 Cycles           = 0x17;
        constexpr inline u8 DesignCap        = 0x18;
        constexpr inline u8 AverageVCell     = 0x19;
        constexpr inline u8 MaxMinTemp       = 0x1A;
        constexpr inline u8 MaxMinVoltage    = 0x1B;
        constexpr inline u8 MaxMinCurrent    = 0x1C;
        constexpr inline u8 Config           = 0x1D;
        constexpr inline u8 IChgTerm         = 0x1E;
        constexpr inline u8 RemCapAv         = 0x1F;

        constexpr inline u8 Version          = 0x21;
        constexpr inline u8 QResidual10      = 0x22;
        constexpr inline u8 FullCapNom       = 0x23;
        constexpr inline u8 TempNom          = 0x24;
        constexpr inline u8 TempLim          = 0x25;

        constexpr inline u8 Ain              = 0x27;
        constexpr inline u8 LearnCfg         = 0x28;
        constexpr inline u8 FilterCfg        = 0x29;
        constexpr inline u8 RelaxCfg         = 0x2A;
        constexpr inline u8 MiscCfg          = 0x2B;
        constexpr inline u8 TGain            = 0x2C;
        constexpr inline u8 TOff             = 0x2D;
        constexpr inline u8 CGain            = 0x2E;
        constexpr inline u8 COff             = 0x2F;


        constexpr inline u8 QResidual20      = 0x32;


        constexpr inline u8 FullCap0         = 0x35;
        constexpr inline u8 IAvgEmpty        = 0x36;
        constexpr inline u8 FCtc             = 0x37;
        constexpr inline u8 RComp0           = 0x38;
        constexpr inline u8 TempCo           = 0x39;
        constexpr inline u8 VEmpty           = 0x3A;


        constexpr inline u8 FStat            = 0x3D;
        constexpr inline u8 Timer            = 0x3E;
        constexpr inline u8 ShdnTimer        = 0x3F;


        constexpr inline u8 QResidual30      = 0x42;


        constexpr inline u8 DQAcc            = 0x45;
        constexpr inline u8 DPAcc            = 0x46;

        constexpr inline u8 SocVf0           = 0x48;

        constexpr inline u8 Qh0              = 0x4C;
        constexpr inline u8 Qh               = 0x4D;

        constexpr inline u8 SocVfAccess      = 0x60;

        constexpr inline u8 ModelAccess0     = 0x62;
        constexpr inline u8 ModelAccess1     = 0x63;

        constexpr inline u8 ModelChrTblStart = 0x80;
        constexpr inline u8 ModelChrTblEnd   = 0xB0;


        constexpr inline u8 VFocV            = 0xFB;
        constexpr inline u8 SocVf            = 0xFF;

        constexpr inline size_t ModelChrTblSize = ModelChrTblEnd - ModelChrTblStart;

        namespace {

            struct CustomParameters {
                u16 relaxcfg;
                u16 rcomp0;
                u16 tempco;
                u16 ichgterm;
                u16 tgain;
                u16 toff;
                u16 vempty;
                u16 qresidual00;
                u16 qresidual10;
                u16 qresidual20;
                u16 qresidual30;
                u16 fullcap;
                u16 vffullcap;
                u16 modeltbl[ModelChrTblSize];
                u16 fullsocthr;
                u16 iavgempty;
            };

            #include "powctl_max17050_custom_parameters.inc"

            const CustomParameters &GetCustomParameters(const char *battery_vendor, u8 battery_version) {
                if (battery_version == 2) {
                    if (battery_vendor[7] == 'M') {
                        return CustomParameters2M;
                    } else if (battery_vendor[7] == 'R') {
                        return CustomParameters2R;
                    } else /* if (battery_vendor[7] == 'A') */ {
                        return CustomParameters2A;
                    }
                } else if (battery_version == 1) {
                    return CustomParameters1;
                } else /* if (battery_version == 0) */ {
                    if (battery_vendor[7] == 'M') {
                        return CustomParameters0M;
                    } else if (battery_vendor[7] == 'R') {
                        return CustomParameters0R;
                    } else /* if (battery_vendor[7] == 'A') */ {
                        return CustomParameters0A;
                    }
                }
            }

        }

    }

    namespace {

        ALWAYS_INLINE Result ReadWriteRegister(const i2c::I2cSession &session, u8 address, u16 mask, u16 value) {
            /* Read the current value. */
            u16 cur_val;
            R_TRY(i2c::ReadSingleRegister(session, address, std::addressof(cur_val)));

            /* Update the value. */
            const u16 new_val = (cur_val & ~mask) | (value & mask);
            R_TRY(i2c::WriteSingleRegister(session, address, new_val));

            return ResultSuccess();
        }

        ALWAYS_INLINE Result ReadRegister(const i2c::I2cSession &session, u8 address, u16 *out) {
            return i2c::ReadSingleRegister(session, address, out);
        }

        ALWAYS_INLINE Result WriteRegister(const i2c::I2cSession &session, u8 address, u16 val) {
            return i2c::WriteSingleRegister(session, address, val);
        }

        ALWAYS_INLINE bool WriteValidateRegister(const i2c::I2cSession &session, u8 address, u16 val) {
            /* Write the value. */
            R_ABORT_UNLESS(WriteRegister(session, address, val));

            /* Give it time to take. */
            os::SleepThread(TimeSpan::FromMilliSeconds(3));

            /* Read it back. */
            u16 new_val;
            R_ABORT_UNLESS(ReadRegister(session, address, std::addressof(new_val)));

            return new_val == val;
        }

        ALWAYS_INLINE Result ReadWriteValidateRegister(const i2c::I2cSession &session, u8 address, u16 mask, u16 value) {
            /* Read the current value. */
            u16 cur_val;
            R_TRY(i2c::ReadSingleRegister(session, address, std::addressof(cur_val)));

            /* Update the value. */
            const u16 new_val = (cur_val & ~mask) | (value & mask);
            while (!WriteValidateRegister(session, address, new_val)) { /* ... */ }

            return ResultSuccess();
        }

        double CoerceToDouble(u64 value) {
            static_assert(sizeof(value) == sizeof(double));

            double d;
            __builtin_memcpy(std::addressof(d), std::addressof(value), sizeof(d));
            return d;
        }

        double ExponentiateTwoToPower(s16 exponent, double scale) {
            if (exponent >= 1024) {
                exponent = exponent - 1023;
                scale = scale * 8.98846567e307;
                if (exponent >= 1024) {
                    exponent = std::min<s16>(exponent, 2046) - 1023;
                    scale = scale * 8.98846567e307;
                }
            } else if (exponent <= -1023) {
                exponent = exponent + 969;
                scale = scale * 2.00416836e-292;
                if (exponent <= -1023) {
                    exponent = std::max<s16>(exponent, -1991) + 969;
                    scale = scale * 2.00416836e-292;
                }
            }
            return scale * CoerceToDouble(static_cast<u64>(exponent + 1023) << 52);
        }

    }

    Result Max17050Driver::InitializeSession(const char *battery_vendor, u8 battery_version) {
        /* Get the custom parameters. */
        const auto &params = max17050::GetCustomParameters(battery_vendor, battery_version);

        /* We only want to write the parameters on power on reset. */
        R_SUCCEED_IF(!this->IsPowerOnReset());

        /* Set that we need to restore parameters. */
        R_TRY(this->SetNeedToRestoreParameters(true));

        /* Wait for our configuration to take. */
        os::SleepThread(TimeSpan::FromMilliSeconds(500));

        /* Write initial config. */
        R_TRY(WriteRegister(this->i2c_session, max17050::Config, 0x7210));

        /* Write initial filter config. */
        R_TRY(WriteRegister(this->i2c_session, max17050::FilterCfg, 0x8784));

        /* Write relax config. */
        R_TRY(WriteRegister(this->i2c_session, max17050::RelaxCfg, params.relaxcfg));

        /* Write initial learn config. */
        R_TRY(WriteRegister(this->i2c_session, max17050::LearnCfg, 0x2603));

        /* Write fullsocthr. */
        R_TRY(WriteRegister(this->i2c_session, max17050::FullSocThr, params.fullsocthr));

        /* Write iavgempty. */
        R_TRY(WriteRegister(this->i2c_session, max17050::IAvgEmpty, params.iavgempty));

        /* Unlock model table, write model table. */
        do {
            R_TRY(this->UnlockModelTable());
            R_TRY(this->SetModelTable(params.modeltbl));
        } while (!this->IsModelTableSet(params.modeltbl));

        /* Lock the model table, trying up to ten times. */
        {
            size_t i = 0;
            while (true) {
                ++i;

                R_TRY(this->LockModelTable());

                if (this->IsModelTableLocked()) {
                    break;
                }

                R_SUCCEED_IF(i >= 10);
            }
        }

        /* Write and validate rcomp0 */
        while (!WriteValidateRegister(this->i2c_session, max17050::RComp0, params.rcomp0)) { /* ... */ }

        /* Write and validate tempco */
        while (!WriteValidateRegister(this->i2c_session, max17050::TempCo, params.tempco)) { /* ... */ }

        /* Write ichgterm. */
        R_TRY(WriteRegister(this->i2c_session, max17050::IChgTerm, params.ichgterm));

        /* Write tgain. */
        R_TRY(WriteRegister(this->i2c_session, max17050::TGain, params.tgain));

        /* Write toff. */
        R_TRY(WriteRegister(this->i2c_session, max17050::TOff, params.toff));

        /* Write and validate vempty. */
        while (!WriteValidateRegister(this->i2c_session, max17050::VEmpty, params.vempty)) { /* ... */ }

        /* Write and validate qresidual. */
        while (!WriteValidateRegister(this->i2c_session, max17050::QResidual00, params.qresidual00)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::QResidual10, params.qresidual10)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::QResidual20, params.qresidual20)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::QResidual30, params.qresidual30)) { /* ... */ }

        /* Write capacity parameters. */
        while (!WriteValidateRegister(this->i2c_session, max17050::FullCap, params.fullcap)) { /* ... */ }
        R_TRY(WriteRegister(this->i2c_session,           max17050::DesignCap, params.vffullcap));
        while (!WriteValidateRegister(this->i2c_session, max17050::FullCapNom, params.vffullcap)) { /* ... */ }

        /* Give some time for configuration to take. */
        os::SleepThread(TimeSpan::FromMilliSeconds(350));

        /* Write vfsoc to vfsoc0, qh, to qh0. */
        u16 vfsoc, qh;
        {
            R_TRY(ReadRegister(this->i2c_session, max17050::SocVf, std::addressof(vfsoc)));
            R_TRY(this->UnlockVfSoc());
            while (!WriteValidateRegister(this->i2c_session, max17050::SocVf0, vfsoc)) { /* ... */ }
            R_TRY(ReadRegister(this->i2c_session, max17050::Qh, std::addressof(qh)));
            R_TRY(WriteRegister(this->i2c_session, max17050::Qh0, qh));
            R_TRY(this->LockVfSoc());
        }

        /* Reset cycles. */
        while (!WriteValidateRegister(this->i2c_session, max17050::Cycles, 0x0060)) { /* ... */ }

        /* Load new capacity parameters. */
        const u16 remcap = static_cast<u16>((vfsoc * params.vffullcap) / 0x6400);
        const u16 repcap = static_cast<u16>(remcap * (params.fullcap / params.vffullcap));
        const u16 dpacc  = 0x0C80;
        const u16 dqacc  = params.vffullcap / 0x10;
        while (!WriteValidateRegister(this->i2c_session, max17050::RemCapMix, remcap)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::RemCapRep, repcap)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::DPAcc, dpacc)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::DQAcc, dqacc)) { /* ... */ }

        /* Write capacity parameters. */
        while (!WriteValidateRegister(this->i2c_session, max17050::FullCap, params.fullcap)) { /* ... */ }
        R_TRY(WriteRegister(this->i2c_session,           max17050::DesignCap, params.vffullcap));
        while (!WriteValidateRegister(this->i2c_session, max17050::FullCapNom, params.vffullcap)) { /* ... */ }

        /* Write soc rep. */
        R_TRY(WriteRegister(this->i2c_session, max17050::SocRep, vfsoc));

        /* Clear power on reset. */
        R_TRY(ReadWriteValidateRegister(this->i2c_session, max17050::Status, 0x0002, 0x0000));

        /* Set cgain. */
        R_TRY(WriteRegister(this->i2c_session, max17050::CGain, 0x7FFF));

        return ResultSuccess();
    }

    Result Max17050Driver::SetMaximumShutdownTimerThreshold() {
        return WriteRegister(this->i2c_session, max17050::ShdnTimer, 0xE000);
    }

    bool Max17050Driver::IsPowerOnReset() {
        /* Get the register. */
        u16 val;
        R_ABORT_UNLESS(ReadRegister(this->i2c_session, max17050::Status, std::addressof(val)));

        /* Extract the value. */
        return (val & 0x0002) != 0;
    }

    Result Max17050Driver::LockVfSoc() {
        return WriteRegister(this->i2c_session, max17050::SocVfAccess, 0x0000);
    }

    Result Max17050Driver::UnlockVfSoc() {
        return WriteRegister(this->i2c_session, max17050::SocVfAccess, 0x0080);
    }

    Result Max17050Driver::LockModelTable() {
        R_TRY(WriteRegister(this->i2c_session, max17050::ModelAccess0, 0x0000));
        R_TRY(WriteRegister(this->i2c_session, max17050::ModelAccess1, 0x0000));
        return ResultSuccess();
    }

    Result Max17050Driver::UnlockModelTable() {
        R_TRY(WriteRegister(this->i2c_session, max17050::ModelAccess0, 0x0059));
        R_TRY(WriteRegister(this->i2c_session, max17050::ModelAccess1, 0x00C4));
        return ResultSuccess();
    }

    bool Max17050Driver::IsModelTableLocked() {
        for (size_t i = 0; i < max17050::ModelChrTblSize; ++i) {
            u16 val;
            R_ABORT_UNLESS(ReadRegister(this->i2c_session, max17050::ModelChrTblStart + i, std::addressof(val)));

            if (val != 0) {
                return false;
            }
        }

        return true;
    }

    Result Max17050Driver::SetModelTable(const u16 *model_table) {
        for (size_t i = 0; i < max17050::ModelChrTblSize; ++i) {
            R_TRY(WriteRegister(this->i2c_session, max17050::ModelChrTblStart + i, model_table[i]));
        }

        return ResultSuccess();
    }

    bool Max17050Driver::IsModelTableSet(const u16 *model_table) {
        for (size_t i = 0; i < max17050::ModelChrTblSize; ++i) {
            u16 val;
            R_ABORT_UNLESS(ReadRegister(this->i2c_session, max17050::ModelChrTblStart + i, std::addressof(val)));

            if (val != model_table[i]) {
                return false;
            }
        }

        return true;
    }

    Result Max17050Driver::ReadInternalState() {
        R_TRY(ReadRegister(this->i2c_session, max17050::RComp0, std::addressof(this->internal_state.rcomp0)));
        R_TRY(ReadRegister(this->i2c_session, max17050::TempCo, std::addressof(this->internal_state.tempco)));
        R_TRY(ReadRegister(this->i2c_session, max17050::FullCap, std::addressof(this->internal_state.fullcap)));
        R_TRY(ReadRegister(this->i2c_session, max17050::Cycles, std::addressof(this->internal_state.cycles)));
        R_TRY(ReadRegister(this->i2c_session, max17050::FullCapNom, std::addressof(this->internal_state.fullcapnom)));
        R_TRY(ReadRegister(this->i2c_session, max17050::IAvgEmpty, std::addressof(this->internal_state.iavgempty)));
        R_TRY(ReadRegister(this->i2c_session, max17050::QResidual00, std::addressof(this->internal_state.qresidual00)));
        R_TRY(ReadRegister(this->i2c_session, max17050::QResidual10, std::addressof(this->internal_state.qresidual10)));
        R_TRY(ReadRegister(this->i2c_session, max17050::QResidual20, std::addressof(this->internal_state.qresidual20)));
        R_TRY(ReadRegister(this->i2c_session, max17050::QResidual30, std::addressof(this->internal_state.qresidual30)));
        return ResultSuccess();
    }

    Result Max17050Driver::WriteInternalState() {
        while (!WriteValidateRegister(this->i2c_session, max17050::RComp0, this->internal_state.rcomp0)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::TempCo, this->internal_state.tempco)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::FullCapNom, this->internal_state.fullcapnom)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::IAvgEmpty,  this->internal_state.iavgempty)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::QResidual00, this->internal_state.qresidual00)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::QResidual10, this->internal_state.qresidual10)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::QResidual20, this->internal_state.qresidual20)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::QResidual30, this->internal_state.qresidual30)) { /* ... */ }

        os::SleepThread(TimeSpan::FromMilliSeconds(350));

        u16 fullcap0, socmix;
        R_TRY(ReadRegister(this->i2c_session, max17050::FullCap0, std::addressof(fullcap0)));
        R_TRY(ReadRegister(this->i2c_session, max17050::SocMix, std::addressof(socmix)));

        while (!WriteValidateRegister(this->i2c_session, max17050::RemCapMix, static_cast<u16>((fullcap0 * socmix) / 0x6400))) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::FullCap, this->internal_state.fullcap)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::DPAcc, 0x0C80)) { /* ... */ }
        while (!WriteValidateRegister(this->i2c_session, max17050::DQAcc, this->internal_state.fullcapnom / 0x10)) { /* ... */ }

        os::SleepThread(TimeSpan::FromMilliSeconds(350));

        while (!WriteValidateRegister(this->i2c_session, max17050::Cycles, this->internal_state.cycles)) { /* ... */ }
        if (this->internal_state.cycles >= 0x100) {
            while (!WriteValidateRegister(this->i2c_session, max17050::LearnCfg, 0x2673)) { /* ... */ }
        }

        return ResultSuccess();
    }

    Result Max17050Driver::GetSocRep(double *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::SocRep, std::addressof(val)));

        /* Set output. */
        *out = static_cast<double>(val) * 0.00390625;
        return ResultSuccess();
    }

    Result Max17050Driver::GetSocVf(double *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::SocVf, std::addressof(val)));

        /* Set output. */
        *out = static_cast<double>(val) * 0.00390625;
        return ResultSuccess();
    }

    Result Max17050Driver::GetFullCapacity(double *out, double sense_resistor) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);
        AMS_ABORT_UNLESS(sense_resistor > 0.0);

        /* Read the values. */
        u16 cgain, fullcap;
        R_TRY(ReadRegister(this->i2c_session, max17050::CGain, std::addressof(cgain)));
        R_TRY(ReadRegister(this->i2c_session, max17050::FullCap, std::addressof(fullcap)));

        /* Set output. */
        *out = ((static_cast<double>(fullcap) * 0.005) / sense_resistor) / (static_cast<double>(cgain) * 0.0000610351562);
        return ResultSuccess();
    }

    Result Max17050Driver::GetRemainingCapacity(double *out, double sense_resistor) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);
        AMS_ABORT_UNLESS(sense_resistor > 0.0);

        /* Read the values. */
        u16 cgain, remcap;
        R_TRY(ReadRegister(this->i2c_session, max17050::CGain, std::addressof(cgain)));
        R_TRY(ReadRegister(this->i2c_session, max17050::RemCapRep, std::addressof(remcap)));

        /* Set output. */
        *out = ((static_cast<double>(remcap) * 0.005) / sense_resistor) / (static_cast<double>(cgain) * 0.0000610351562);
        return ResultSuccess();
    }

    Result Max17050Driver::SetPercentageMinimumAlertThreshold(int percentage) {
        return ReadWriteRegister(this->i2c_session, max17050::SocAlrtThreshold, 0x00FF, static_cast<u8>(percentage));
    }

    Result Max17050Driver::SetPercentageMaximumAlertThreshold(int percentage) {
        return ReadWriteRegister(this->i2c_session, max17050::SocAlrtThreshold, 0xFF00, static_cast<u16>(static_cast<u8>(percentage)) << 8);
    }

    Result Max17050Driver::SetPercentageFullThreshold(double percentage) {
        #if defined(ATMOSPHERE_ARCH_ARM64)
            const u16 val = vcvtd_n_s64_f64(percentage, BITSIZEOF(u8));
        #else
            #error "Unknown architecture for floating point -> fixed point"
        #endif

        return WriteRegister(this->i2c_session, max17050::FullSocThr, val);
    }

    Result Max17050Driver::GetAverageCurrent(double *out, double sense_resistor) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);
        AMS_ABORT_UNLESS(sense_resistor > 0.0);

        /* Read the values. */
        u16 cgain, coff, avg_current;
        R_TRY(ReadRegister(this->i2c_session, max17050::CGain, std::addressof(cgain)));
        R_TRY(ReadRegister(this->i2c_session, max17050::COff, std::addressof(coff)));
        R_TRY(ReadRegister(this->i2c_session, max17050::AverageCurrent, std::addressof(avg_current)));

        /* Set output. */
        *out = (((static_cast<double>(avg_current) - (static_cast<double>(coff) + static_cast<double>(coff))) / (static_cast<double>(cgain) * 0.0000610351562)) * 1.5625) / (sense_resistor * 1000.0);
        return ResultSuccess();
    }

    Result Max17050Driver::GetCurrent(double *out, double sense_resistor) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);
        AMS_ABORT_UNLESS(sense_resistor > 0.0);

        /* Read the values. */
        u16 cgain, coff, current;
        R_TRY(ReadRegister(this->i2c_session, max17050::CGain, std::addressof(cgain)));
        R_TRY(ReadRegister(this->i2c_session, max17050::COff, std::addressof(coff)));
        R_TRY(ReadRegister(this->i2c_session, max17050::Current, std::addressof(current)));

        /* Set output. */
        *out = (((static_cast<double>(current) - (static_cast<double>(coff) + static_cast<double>(coff))) / (static_cast<double>(cgain) * 0.0000610351562)) * 1.5625) / (sense_resistor * 1000.0);
        return ResultSuccess();
    }

    Result Max17050Driver::GetNeedToRestoreParameters(bool *out) {
        /* Get the register. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::MiscCfg, std::addressof(val)));

        /* Extract the value. */
        *out = (val & 0x8000) != 0;
        return ResultSuccess();
    }

    Result Max17050Driver::SetNeedToRestoreParameters(bool en) {
        return ReadWriteRegister(this->i2c_session, max17050::MiscCfg, 0x8000, en ? 0x8000 : 0);
    }

    Result Max17050Driver::IsI2cShutdownEnabled(bool *out) {
        /* Get the register. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::Config, std::addressof(val)));

        /* Extract the value. */
        *out = (val & 0x0040) != 0;
        return ResultSuccess();
    }

    Result Max17050Driver::SetI2cShutdownEnabled(bool en) {
        return ReadWriteRegister(this->i2c_session, max17050::Config, 0x0040, en ? 0x0040 : 0);
    }

    Result Max17050Driver::GetStatus(u16 *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        return ReadRegister(this->i2c_session, max17050::Status, out);
    }

    Result Max17050Driver::GetCycles(u16 *out) {
        /* Get the register. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::Cycles, std::addressof(val)));

        /* Extract the value. */
        *out = std::max<u16>(val, 0x60) - 0x60;
        return ResultSuccess();
    }

    Result Max17050Driver::ResetCycles() {
        return WriteRegister(this->i2c_session, max17050::Cycles, 0x0060);
    }

    Result Max17050Driver::GetAge(double *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::Age, std::addressof(val)));

        /* Set output. */
        *out = static_cast<double>(val) * 0.00390625;
        return ResultSuccess();
    }

    Result Max17050Driver::GetTemperature(double *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::Temperature, std::addressof(val)));

        /* Set output. */
        *out = static_cast<double>(val) * 0.00390625;
        return ResultSuccess();
    }

    Result Max17050Driver::GetMaximumTemperature(u8 *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::MaxMinTemp, std::addressof(val)));

        /* Set output. */
        *out = static_cast<u8>(val >> 8);
        return ResultSuccess();
    }

    Result Max17050Driver::SetTemperatureMinimumAlertThreshold(int c) {
        return ReadWriteRegister(this->i2c_session, max17050::TAlrtThreshold, 0x00FF, static_cast<u8>(c));
    }

    Result Max17050Driver::SetTemperatureMaximumAlertThreshold(int c) {
        return ReadWriteRegister(this->i2c_session, max17050::TAlrtThreshold, 0xFF00, static_cast<u16>(static_cast<u8>(c)) << 8);
    }

    Result Max17050Driver::GetVCell(int *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::VCell, std::addressof(val)));

        /* Set output. */
        *out = (625 * (val >> 3)) / 1000;
        return ResultSuccess();
    }

    Result Max17050Driver::GetAverageVCell(int *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::AverageVCell, std::addressof(val)));

        /* Set output. */
        *out = (625 * (val >> 3)) / 1000;
        return ResultSuccess();
    }

    Result Max17050Driver::GetAverageVCellTime(double *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::FilterCfg, std::addressof(val)));

        /* Set output. */
        *out = 175.8 * ExponentiateTwoToPower(6 + ((val >> 4) & 7), 1.0);
        return ResultSuccess();
    }

    Result Max17050Driver::GetOpenCircuitVoltage(int *out) {
        /* Validate parameters. */
        AMS_ABORT_UNLESS(out != nullptr);

        /* Read the value. */
        u16 val;
        R_TRY(ReadRegister(this->i2c_session, max17050::VFocV, std::addressof(val)));

        /* Set output. */
        *out = (1250 * (val >> 4)) / 1000;
        return ResultSuccess();
    }

    Result Max17050Driver::SetVoltageMinimumAlertThreshold(int mv) {
        return ReadWriteRegister(this->i2c_session, max17050::VAlrtThreshold, 0x00FF, static_cast<u8>(util::DivideUp(mv, 20)));
    }

    Result Max17050Driver::SetVoltageMaximumAlertThreshold(int mv) {
        return ReadWriteRegister(this->i2c_session, max17050::VAlrtThreshold, 0xFF00, static_cast<u16>(static_cast<u8>(mv / 20)) << 8);
    }

}
