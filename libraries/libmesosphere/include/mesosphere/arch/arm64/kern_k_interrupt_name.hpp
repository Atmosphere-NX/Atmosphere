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

namespace ams::kern::arch::arm64 {

    namespace interrupt_name {

        enum KInterruptName : s32 {
            /* SGIs */
            KInterruptName_ThreadTerminate        = 4,
            KInterruptName_CacheOperation         = 5,
            KInterruptName_Scheduler              = 6,

            KInterruptName_PerformanceCounter     = 8,

            /* PPIs */
    #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
            KInterruptName_VirtualMaintenance     = 25,
            KInterruptName_HypervisorTimer        = 26,
            KInterruptName_VirtualTimer           = 27,
            KInterruptName_LegacyNFiq             = 28,
            KInterruptName_SecurePhysicalTimer    = 29,
            KInterruptName_NonSecurePhysicalTimer = 30,
            KInterruptName_LegacyNIrq             = 31,
    #endif

    #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
            KInterruptName_MemoryController       = 109,
    #endif
        };

    };

}
