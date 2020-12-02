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

namespace ams::secmon::boot {

    void ClearIramBootCode();
    void ClearIramBootKeys();
    void ClearIramDebugCode();

    void WaitForNxBootloader(const pkg1::SecureMonitorParameters &params, pkg1::BootloaderState state);

    void LoadBootConfig(const void *src);
    void VerifyOrClearBootConfig();

    void EnableTsc(u64 initial_tsc_value);

    void WriteGpuCarveoutMagicNumbers();

    void UpdateBootConfigForPackage2Header(const pkg2::Package2Header &header);
    void VerifyPackage2HeaderSignature(pkg2::Package2Header &header, bool verify);
    void DecryptPackage2Header(pkg2::Package2Meta *dst, const pkg2::Package2Meta &src, bool encrypted);

    void VerifyPackage2Header(const pkg2::Package2Meta &meta);

    void DecryptAndLoadPackage2Payloads(uintptr_t dst, const pkg2::Package2Meta &meta, uintptr_t src, bool encrypted);

    void CheckVerifyResult(bool verify_result, pkg1::ErrorInfo error_info, const char *message);

}
