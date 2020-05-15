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
#include <exosphere.hpp>

namespace ams::secmon {

    void SaveBootInfo(const pkg1::SecureMonitorParameters &secmon_params);

    bool IsRecoveryBoot();

    u32 GetRestrictedSmcMask();

    bool IsJtagEnabled();

    void GetPackage2Hash(se::Sha256Hash *out);
    void SetPackage2Hash(const se::Sha256Hash &hash);

    u32 GetDeprecatedBootReason();

}