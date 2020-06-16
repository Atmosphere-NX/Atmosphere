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

namespace ams::pmc {

    enum SecureRegister {
        SecureRegister_Other          = (1 << 0),
        SecureRegister_DramParameters = (1 << 1),
        SecureRegister_ResetVector    = (1 << 2),
        SecureRegister_Carveout       = (1 << 3),
        SecureRegister_CmacWrite      = (1 << 4),
        SecureRegister_CmacRead       = (1 << 5),
        SecureRegister_KeySourceWrite = (1 << 6),
        SecureRegister_KeySourceRead  = (1 << 7),
        SecureRegister_Srk            = (1 << 8),

        SecureRegister_CmacReadWrite      = SecureRegister_CmacRead | SecureRegister_CmacWrite,
        SecureRegister_KeySourceReadWrite = SecureRegister_KeySourceRead | SecureRegister_KeySourceWrite,
    };

    void SetRegisterAddress(uintptr_t address);

    void InitializeRandomScratch();
    void EnableWakeEventDetection();
    void ConfigureForSc7Entry();

    void LockSecureRegister(SecureRegister reg);

    enum class LockState {
        Locked          = 0,
        NotLocked       = 1,
        PartiallyLocked = 2,
    };

    LockState GetSecureRegisterLockState(SecureRegister reg);

}