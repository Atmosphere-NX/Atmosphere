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
#include "boot_battery_driver.hpp"
#include "boot_calibration.hpp"
#include "boot_i2c_utils.hpp"

namespace ams::boot {

    /* Include configuration into anonymous namespace. */
    namespace {

#include "boot_battery_parameters.inc"

        const Max17050Parameters *GetBatteryParameters() {
            const u32 battery_version = GetBatteryVersion();
            const u32 battery_vendor = GetBatteryVendor();

            if (battery_version == 2) {
                if (battery_vendor == 'M') {
                    return &Max17050Params2M;
                } else {
                    return &Max17050Params2;
                }
            } else if (battery_version == 1) {
                return &Max17050Params1;
            } else {
                switch (battery_vendor) {
                    case 'M':
                        return &Max17050ParamsM;
                    case 'R':
                        return &Max17050ParamsR;
                    case 'A':
                    default:
                        return &Max17050ParamsA;
                }
            }
        }

    }

    Result BatteryDriver::Read(u8 addr, u16 *out) {
        return ReadI2cRegister(this->i2c_session, reinterpret_cast<u8 *>(out), sizeof(*out), &addr, sizeof(addr));
    }

    Result BatteryDriver::Write(u8 addr, u16 val) {
        return WriteI2cRegister(this->i2c_session, reinterpret_cast<u8 *>(&val), sizeof(val), &addr, sizeof(addr));
    }

    Result BatteryDriver::ReadWrite(u8 addr, u16 mask, u16 val) {
        u16 cur_val;
        R_TRY(this->Read(addr, &cur_val));

        const u16 new_val = (cur_val & ~mask) | val;
        R_TRY(this->Write(addr, new_val));

        return ResultSuccess();
    }

    bool BatteryDriver::WriteValidate(u8 addr, u16 val) {
        /* Nintendo doesn't seem to check errors when doing this? */
        /* It's probably okay, since the value does get validated. */
        /* That said, we will validate the read to avoid uninit data problems. */
        this->Write(addr, val);
        svcSleepThread(3'000'000ul);

        u16 new_val;
        return R_SUCCEEDED(this->Read(addr, &new_val)) && new_val == val;
    }

    bool BatteryDriver::IsPowerOnReset() {
        /* N doesn't check result... */
        u16 val = 0;
        this->Read(Max17050Status, &val);
        return (val & 0x0002) == 0x0002;
    }

    Result BatteryDriver::LockVfSoc() {
        return this->Write(Max17050SocVfAccess, 0x0000);
    }

    Result BatteryDriver::UnlockVfSoc() {
        return this->Write(Max17050SocVfAccess, 0x0080);
    }

    Result BatteryDriver::LockModelTable() {
        R_TRY(this->Write(Max17050ModelAccess0, 0x0000));
        R_TRY(this->Write(Max17050ModelAccess1, 0x0000));
        return ResultSuccess();
    }

    Result BatteryDriver::UnlockModelTable() {
        R_TRY(this->Write(Max17050ModelAccess0, 0x0059));
        R_TRY(this->Write(Max17050ModelAccess1, 0x00C4));
        return ResultSuccess();
    }

    Result BatteryDriver::SetModelTable(const u16 *model_table) {
        for (size_t i = 0; i < Max17050ModelChrTblSize; i++) {
            R_TRY(this->Write(Max17050ModelChrTblStart + i, model_table[i]));
        }
        return ResultSuccess();
    }

    bool BatteryDriver::IsModelTableLocked() {
        bool locked = true;

        u16 cur_val = 0;
        for (size_t i = 0; i < Max17050ModelChrTblSize; i++) {
            this->Read(Max17050ModelChrTblStart + i, &cur_val);
            locked &= (cur_val == 0);
        }

        return locked;
    }

    bool BatteryDriver::IsModelTableSet(const u16 *model_table) {
        bool set = true;

        u16 cur_val = 0;
        for (size_t i = 0; i < Max17050ModelChrTblSize; i++) {
            this->Read(Max17050ModelChrTblStart + i, &cur_val);
            set &= (cur_val == model_table[i]);
        }

        return set;
    }

    Result BatteryDriver::InitializeBatteryParameters() {
        const Max17050Parameters *params = GetBatteryParameters();

        if (IsPowerOnReset()) {
            /* Do initial config. */
            R_TRY(this->ReadWrite(Max17050MiscCfg, 0x8000, 0x8000));

            svcSleepThread(500'000'000ul);

            R_TRY(this->Write(Max17050Config, 0x7210));
            R_TRY(this->Write(Max17050FilterCfg, 0x8784));
            R_TRY(this->Write(Max17050RelaxCfg, params->relaxcfg));
            R_TRY(this->Write(Max17050LearnCfg, 0x2603));
            R_TRY(this->Write(Max17050FullSocThr, params->fullsocthr));
            R_TRY(this->Write(Max17050IAvgEmpty, params->iavgempty));

            /* Unlock model table, write model table. */
            do {
                R_TRY(this->UnlockModelTable());
                R_TRY(this->SetModelTable(params->modeltbl));
            } while (!this->IsModelTableSet(params->modeltbl));

            /* Lock model table. */
            size_t lock_i = 0;
            while (true) {
                lock_i++;
                R_TRY(this->LockModelTable());

                if (this->IsModelTableLocked()) {
                    break;
                }

                if (lock_i >= 8) {
                    /* This is regarded as guaranteed success. */
                    return ResultSuccess();
                }
            }

            /* Write custom parameters. */
            while (!this->WriteValidate(Max17050RComp0, params->rcomp0)) { /* ... */ }
            while (!this->WriteValidate(Max17050TempCo, params->tempco)) { /* ... */ }

            R_TRY(this->Write(Max17050IChgTerm, params->ichgterm));
            R_TRY(this->Write(Max17050TGain, params->tgain));
            R_TRY(this->Write(Max17050TOff, params->toff));

            while (!this->WriteValidate(Max17050VEmpty, params->vempty)) { /* ... */ }
            while (!this->WriteValidate(Max17050QResidual00, params->qresidual00)) { /* ... */ }
            while (!this->WriteValidate(Max17050QResidual10, params->qresidual10)) { /* ... */ }
            while (!this->WriteValidate(Max17050QResidual20, params->qresidual20)) { /* ... */ }
            while (!this->WriteValidate(Max17050QResidual30, params->qresidual30)) { /* ... */ }


            /* Write full capacity parameters. */
            while (!this->WriteValidate(Max17050FullCap, params->fullcap)) { /* ... */ }
            R_TRY(this->Write(Max17050DesignCap, params->vffullcap));
            while (!this->WriteValidate(Max17050FullCapNom, params->vffullcap)) { /* ... */ }

            svcSleepThread(350'000'000ul);

            /* Write VFSOC to VFSOC 0. */
            u16 vfsoc, qh;
            {
                R_TRY(this->Read(Max17050SocVf, &vfsoc));
                R_TRY(this->UnlockVfSoc());
                R_TRY(this->Write(Max17050SocVf0, vfsoc));
                R_TRY(this->Read(Max17050Qh, &qh));
                R_TRY(this->Write(Max17050Qh0, qh));
                R_TRY(this->LockVfSoc());
            }

            /* Write cycles. */
            while (!this->WriteValidate(Max17050Cycles, 0x0060)) { /* ... */ }

            /* Load new capacity parameters. */
            const u16 remcap = static_cast<u16>((vfsoc * params->vffullcap) / 0x6400);
            const u16 repcap = static_cast<u16>(remcap * (params->fullcap / params->vffullcap));
            const u16 dqacc = params->vffullcap / 0x10;
            while (!this->WriteValidate(Max17050RemCapMix, remcap)) { /* ... */ }
            while (!this->WriteValidate(Max17050RemCapRep, repcap)) { /* ... */ }
            while (!this->WriteValidate(Max17050DPAcc, 0x0C80)) { /* ... */ }
            while (!this->WriteValidate(Max17050DQAcc, dqacc)) { /* ... */ }
            while (!this->WriteValidate(Max17050FullCap, params->fullcap)) { /* ... */ }
            R_TRY(this->Write(Max17050DesignCap, params->vffullcap));
            while (!this->WriteValidate(Max17050FullCapNom, params->vffullcap)) { /* ... */ }
            R_TRY(this->Write(Max17050SocRep, vfsoc));

            /* Finish initialization. */
            {
                u16 status;
                R_TRY(this->Read(Max17050Status, &status));
                while (!this->WriteValidate(Max17050Status, status & 0xFFFD)) { /* ... */ }
            }
            R_TRY(this->Write(Max17050CGain, 0x7FFF));
        }

        return ResultSuccess();
    }

    Result BatteryDriver::IsBatteryRemoved(bool *out) {
        /* N doesn't check result, but we will. */
        u16 val = 0;
        R_TRY(this->Read(Max17050Status, &val));
        *out = (val & 0x0008) == 0x0008;
        return ResultSuccess();
    }

    Result BatteryDriver::GetTemperature(double *out) {
        u16 val = 0;
        R_TRY(this->Read(Max17050Temperature, &val));
        *out = static_cast<double>(val) * double(0.00390625);
        return ResultSuccess();
    }

    Result BatteryDriver::GetAverageVCell(u32 *out) {
        u16 val = 0;
        R_TRY(this->Read(Max17050AverageVCell, &val));
        *out = (625 * u32(val >> 3)) / 1000;
        return ResultSuccess();
    }

    Result BatteryDriver::GetSocRep(double *out) {
        u16 val = 0;
        R_TRY(this->Read(Max17050SocRep, &val));
        *out = static_cast<double>(val) * double(0.00390625);
        return ResultSuccess();
    }

    Result BatteryDriver::GetBatteryPercentage(size_t *out) {
        double raw_charge;
        R_TRY(this->GetSocRep(&raw_charge));
        int converted_percentage = (((raw_charge - 3.93359375) * 98.0) / 94.2304688) + 2.0;
        if (converted_percentage < 1) {
            *out = 1;
        } else if (converted_percentage > 100) {
            *out = 100;
        } else {
            *out = static_cast<size_t>(converted_percentage);
        }
        return ResultSuccess();
    }

    Result BatteryDriver::SetShutdownTimer() {
        return this->Write(Max17050ShdnTimer, 0xE000);
    }

    Result BatteryDriver::GetShutdownEnabled(bool *out) {
        u16 val = 0;
        R_TRY(this->Read(Max17050Config, &val));
        *out = (val & 0x0040) != 0;
        return ResultSuccess();
    }

    Result BatteryDriver::SetShutdownEnabled(bool enabled) {
        return this->ReadWrite(Max17050Config, 0x0040, enabled ? 0x0040 : 0x0000);
    }

}
