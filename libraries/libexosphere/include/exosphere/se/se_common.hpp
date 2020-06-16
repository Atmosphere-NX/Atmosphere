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

namespace ams::secmon {

    u8 *GetSecurityEngineEphemeralWorkBlock();

}

namespace ams::se {

    using DoneHandler = void(*)();

    enum KeySlotLockFlags {
        KeySlotLockFlags_None            = 0,
        KeySlotLockFlags_KeyRead         = (1u << 0),
        KeySlotLockFlags_KeyWrite        = (1u << 1),
        KeySlotLockFlags_OriginalIvRead  = (1u << 2),
        KeySlotLockFlags_OriginalIvWrite = (1u << 3),
        KeySlotLockFlags_UpdatedIvRead   = (1u << 4),
        KeySlotLockFlags_UpdatedIvWrite  = (1u << 5),
        KeySlotLockFlags_KeyUse          = (1u << 6),
        KeySlotLockFlags_DstKeyTableOnly = (1u << 7),
        KeySlotLockFlags_PerKey          = (1u << 8),

        KeySlotLockFlags_AllReadLock = (KeySlotLockFlags_KeyRead | KeySlotLockFlags_OriginalIvRead | KeySlotLockFlags_UpdatedIvRead),
        KeySlotLockFlags_AllLockKek  = 0x1FF,
        KeySlotLockFlags_AllLockKey  = (KeySlotLockFlags_AllLockKek & ~KeySlotLockFlags_DstKeyTableOnly),

        KeySlotLockFlags_EristaMask  = 0x7F,
        KeySlotLockFlags_MarikoMask  = 0xFF,
    };

    ALWAYS_INLINE u8 *GetEphemeralWorkBlock() {
        return ::ams::secmon::GetSecurityEngineEphemeralWorkBlock();
    }

}
