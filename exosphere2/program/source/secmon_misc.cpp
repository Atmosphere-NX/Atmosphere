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
#include "secmon_misc.hpp"

namespace ams::secmon {

    namespace {

        pkg1::BctParameters g_bct_params;

    }

    void SaveBootInfo(const pkg1::SecureMonitorParameters &secmon_params) {
        /* Save the BCT parameters. */
        g_bct_params = secmon_params.bct_params;
    }

    bool IsRecoveryBoot() {
        return (g_bct_params.bootloader_attributes & pkg1::BootloaderAttribute_RecoveryBoot) != 0;
    }

    u32 GetRestrictedSmcMask() {
        return (g_bct_params.bootloader_attributes & pkg1::BootloaderAttribute_RestrictedSmcMask) >> pkg1::BootloaderAttribute_RestrictedSmcShift;
    }

    bool IsJtagEnabled() {
        util::BitPack32 dbg_auth;
        HW_CPU_GET_DBGAUTHSTATUS_EL1(dbg_auth);
        return dbg_auth.Get<hw::DbgAuthStatusEl1::Nsid>() == 3;
    }

}