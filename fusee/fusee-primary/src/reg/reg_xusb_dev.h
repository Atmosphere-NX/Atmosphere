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

#include "reg_util.h"

namespace t210 {
	static const uintptr_t XUSB_DEV_BASE = 0x700d0000;

    const struct T_XUSB_DEV_XHCI {
        static const uintptr_t base_addr = XUSB_DEV_BASE;
        using Peripheral = T_XUSB_DEV_XHCI;

        BEGIN_DEFINE_WO_REGISTER(DB, uint32_t, 0x4)
            DEFINE_WO_FIELD(STREAMID, 16, 31)
            DEFINE_WO_FIELD(TARGET, 8, 15)
        END_DEFINE_WO_REGISTER(DB)
        
        BEGIN_DEFINE_REGISTER(ERSTSZ, uint32_t, 0x8)
            DEFINE_RW_FIELD(ERST1SZ, 16, 31)
            DEFINE_RW_FIELD(ERST0SZ, 0, 15)
        END_DEFINE_REGISTER(ERSTSZ)

        BEGIN_DEFINE_REGISTER(ERST0BALO, uint32_t, 0x10)
            DEFINE_RW_FIELD(ADDRLO, 4, 31, uintptr_t, 4)
        END_DEFINE_REGISTER(ERST0BALO)

        BEGIN_DEFINE_REGISTER(ERST0BAHI, uint32_t, 0x14)
            DEFINE_RW_FIELD(ADDRHI, 0, 31)
        END_DEFINE_REGISTER(ERST0BAHI)

        BEGIN_DEFINE_REGISTER(ERST1BALO, uint32_t, 0x18)
            DEFINE_RW_FIELD(ADDRLO, 4, 31, uintptr_t, 4)
        END_DEFINE_REGISTER(ERST1BALO)

        BEGIN_DEFINE_REGISTER(ERST1BAHI, uint32_t, 0x1c)
            DEFINE_RW_FIELD(ADDRHI, 0, 31)
        END_DEFINE_REGISTER(ERST1BAHI)

        BEGIN_DEFINE_REGISTER(ERDPLO, uint32_t, 0x20)
            DEFINE_RW_FIELD(ADDRLO, 4, 31, uintptr_t, 4)
            DEFINE_RW1C_FIELD(EHB, 3)
        END_DEFINE_REGISTER(ERDPLO)

        BEGIN_DEFINE_REGISTER(ERDPHI, uint32_t, 0x24)
            DEFINE_RW_FIELD(ADDRHI, 0, 31)
        END_DEFINE_REGISTER(ERDPHI)

        BEGIN_DEFINE_REGISTER(EREPLO, uint32_t, 0x28)
            DEFINE_RW_FIELD(ADDRLO, 4, 31, uintptr_t, 4)
            DEFINE_RW_FIELD(SEGI, 1)
            DEFINE_RW_FIELD(ECS, 0)
        END_DEFINE_REGISTER(EREPLO)

        BEGIN_DEFINE_REGISTER(EREPHI, uint32_t, 0x2c)
            DEFINE_RW_FIELD(ADDRHI, 0, 31)
        END_DEFINE_REGISTER(EREPHI)

        BEGIN_DEFINE_REGISTER(CTRL, uint32_t, 0x30)
            DEFINE_RW_FIELD(ENABLE, 31)
            DEFINE_RW_FIELD(DEVADR, 24, 30)
            DEFINE_RW_FIELD(RSVD0, 8, 23)
            DEFINE_RW_FIELD(EWE, 7)
            DEFINE_RW_FIELD(SMI_DSE, 6)
            DEFINE_RW_FIELD(SMI_EVT, 5)
            DEFINE_RW_FIELD(IE, 4)
            DEFINE_RW_FIELD(RSVD1, 2, 3)
            DEFINE_RW_FIELD(LSE, 1)
            DEFINE_RW_FIELD(RUN, 0)
        END_DEFINE_REGISTER(CTRL)

        BEGIN_DEFINE_REGISTER(ST, uint32_t, 0x34)
            DEFINE_RO_FIELD(RSVD1, 6, 1)
            DEFINE_RW1C_FIELD(DSE, 5)
            DEFINE_RW1C_FIELD(IP, 4)
            DEFINE_RO_FIELD(RSVD0, 1, 3)
            DEFINE_RW1C_FIELD(RC, 0)
        END_DEFINE_REGISTER(ST)

        BEGIN_DEFINE_REGISTER(PORTSC, uint32_t, 0x3c)
            DEFINE_RW_FIELD(WPR, 30)
            DEFINE_RW_FIELD(RSVD4, 28, 29)
            DEFINE_RW1C_FIELD(CEC, 23)
            DEFINE_RW1C_FIELD(PLC, 22)
            DEFINE_RW1C_FIELD(PRC, 21)
            DEFINE_RW_FIELD(RSVD3, 20)
            DEFINE_RW1C_FIELD(WRC, 19)
            DEFINE_RW1C_FIELD(CSC, 17)
            DEFINE_RW_FIELD(LWS, 16)
            DEFINE_RW_FIELD(RSVD2, 15)
            BEGIN_DEFINE_RO_SYMBOLIC_FIELD(PS, 10, 13)
                DEFINE_RO_SYMBOLIC_VALUE(UNDEFINED, 0)
                DEFINE_RO_SYMBOLIC_VALUE(FS, 1)
                DEFINE_RO_SYMBOLIC_VALUE(LS, 2)
                DEFINE_RO_SYMBOLIC_VALUE(HS, 3)
                DEFINE_RO_SYMBOLIC_VALUE(SS, 4)
            END_DEFINE_SYMBOLIC_FIELD(PS)
            DEFINE_RO_FIELD(LANE_POLARITY_VALUE, 9)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(PLS, 5, 8)
                DEFINE_RW_SYMBOLIC_VALUE(U0, 0)
                DEFINE_RW_SYMBOLIC_VALUE(U1, 1)
                DEFINE_RW_SYMBOLIC_VALUE(U2, 2)
                DEFINE_RW_SYMBOLIC_VALUE(U3, 3)
                DEFINE_RW_SYMBOLIC_VALUE(DISABLED, 4)
                DEFINE_RW_SYMBOLIC_VALUE(RXDETECT, 5)
                DEFINE_RW_SYMBOLIC_VALUE(INACTIVE, 6)
                DEFINE_RW_SYMBOLIC_VALUE(POLLING, 7)
                DEFINE_RW_SYMBOLIC_VALUE(RECOVERY, 8)
                DEFINE_RW_SYMBOLIC_VALUE(HOTRESET, 9)
                DEFINE_RW_SYMBOLIC_VALUE(COMPLIANCE, 0xa)
                DEFINE_RW_SYMBOLIC_VALUE(LOOPBACK, 0xb)
                DEFINE_RW_SYMBOLIC_VALUE(RESUME, 0xf)
            END_DEFINE_SYMBOLIC_FIELD(PLS)
            DEFINE_RO_FIELD(PR, 4)
            DEFINE_RW_FIELD(LANE_POLARITY_OVRD_VALUE, 3)
            DEFINE_RW_FIELD(LANE_POLARITY_OVRD, 2)
            DEFINE_RW1C_FIELD(PED, 1) // TODO: is this rw1c?
            DEFINE_RW_FIELD(CCS, 0)
        END_DEFINE_REGISTER(PORTSC)
        
        BEGIN_DEFINE_REGISTER(ECPLO, uint32_t, 0x40)
            DEFINE_RW_FIELD(ADDRLO, 6, 31, uintptr_t, 6)
        END_DEFINE_REGISTER(ECPLO)

        BEGIN_DEFINE_REGISTER(ECPHI, uint32_t, 0x44)
            DEFINE_RW_FIELD(ADDRHI, 0, 31)
        END_DEFINE_REGISTER(ECPHI)

        DEFINE_BIT_ARRAY_REGISTER(EP_HALT, uint32_t, 0x50)
        DEFINE_BIT_ARRAY_REGISTER(EP_PAUSE, uint32_t, 0x54)
        DEFINE_BIT_ARRAY_REGISTER(EP_RELOAD, uint32_t, 0x58)
        DEFINE_RW1C_BIT_ARRAY_REGISTER(EP_STCHG, uint32_t, 0x5c)
        
        BEGIN_DEFINE_REGISTER(PORTHALT, uint32_t, 0x6c)
            // TRM labels both bit 26 and bit 20 as STCHG_REQ, but Linux kernel sources disambiguate it.
            //   https://patchwork.kernel.org/patch/11129617/
            //DEFINE_RW_FIELD(???, 26)
            DEFINE_RW_FIELD(STCHG_PME_EN, 25)
            DEFINE_RW_FIELD(STCHG_INTR_EN, 24)
            DEFINE_RW1C_FIELD(STCHG_REQ, 20)
            DEFINE_RW_FIELD(STCHG_STATE, 16, 19)
            DEFINE_RW1C_FIELD(HALT_REJECT, 1) // TODO: RW1C semantics
            DEFINE_RW_FIELD(HALT_LTSSM, 0)
        END_DEFINE_REGISTER(PORTHALT)

        DEFINE_RW1C_BIT_ARRAY_REGISTER(EP_STOPPED, uint32_t, 0x78)
        
        BEGIN_DEFINE_REGISTER(CFG_DEV_FE, uint32_t, 0x85c)
            // "Note: Do not change the values in bits 31:2"
            DEFINE_RW_FIELD(INFINITE_SS_RETRY, 29)
            DEFINE_RW_FIELD(EN_PRIME_EVENT, 28)
            DEFINE_RW_FIELD(FEATURE_LPM, 27)
            DEFINE_RW_FIELD(EN_STALL_EVENT, 26)
            DEFINE_RW_FIELD(PORTDISCON_RST_HW, 25)
            DEFINE_RW_FIELD(CTX_RESTORE, 24)
            DEFINE_RW_FIELD(MFCOUNT_MIN, 4, 23)
            DEFINE_RW_FIELD(PORTRST_HW, 3)
            DEFINE_RW_FIELD(SEQNUM_INIT, 2)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(PORTREGSEL, 0, 1)
                DEFINE_RW_SYMBOLIC_VALUE(INIT, 0)
                DEFINE_RW_SYMBOLIC_VALUE(SS, 1)
                DEFINE_RW_SYMBOLIC_VALUE(HSFS, 2)
            END_DEFINE_SYMBOLIC_FIELD(PORTREGSEL)
        END_DEFINE_REGISTER(CFG_DEV_FE)
    } T_XUSB_DEV_XHCI;

    const struct T_XUSB_DEV {
        static const uintptr_t base_addr = XUSB_DEV_BASE + 0x8000;
        using Peripheral = T_XUSB_DEV;
        
        BEGIN_DEFINE_REGISTER(CFG_1, uint32_t, 0x4)
            DEFINE_RW_FIELD(IO_SPACE, 0)
            DEFINE_RW_FIELD(MEMORY_SPACE, 1)
            DEFINE_RW_FIELD(BUS_MASTER, 2)
        END_DEFINE_REGISTER(CFG_1)

        BEGIN_DEFINE_REGISTER(CFG_4, uint32_t, 0x10)
            DEFINE_RW_FIELD(BASE_ADDRESS, 15, 31, uintptr_t, 15)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(PREFETCHABLE, 3, 3)
                DEFINE_RW_SYMBOLIC_VALUE(NOT, 0)
                DEFINE_RW_SYMBOLIC_VALUE(MERGABLE, 1)
            END_DEFINE_SYMBOLIC_FIELD(PREFETCHABLE)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(ADDRESS_TYPE, 1, 2)
                DEFINE_RW_SYMBOLIC_VALUE(32_BIT, 0)
                DEFINE_RW_SYMBOLIC_VALUE(64_BIT, 2)
            END_DEFINE_SYMBOLIC_FIELD(ADDRESS_TYPE)
            BEGIN_DEFINE_RW_SYMBOLIC_FIELD(SPACE_TYPE, 0, 0)
                DEFINE_RW_SYMBOLIC_VALUE(MEMORY, 0)
                DEFINE_RW_SYMBOLIC_VALUE(IO, 1)
            END_DEFINE_SYMBOLIC_FIELD(SPACE_TYPE)
        END_DEFINE_REGISTER(CFG_4)
    } T_XUSB_DEV;

    const struct XUSB_DEV {
        static const uintptr_t base_addr = XUSB_DEV_BASE + 0x9000;
        using Peripheral = XUSB_DEV;

        BEGIN_DEFINE_REGISTER(CONFIGURATION_0, uint32_t, 0x180)
            DEFINE_RW_FIELD(EN_FPCI, 0)
        END_DEFINE_REGISTER(CONFIGURATION_0)

        BEGIN_DEFINE_REGISTER(INTR_MASK_0, uint32_t, 0x188)
            DEFINE_RW_FIELD(IP_INT_MASK, 16)
            DEFINE_RW_FIELD(MSI_MASK, 8)
            DEFINE_RW_FIELD(INT_MASK, 0)
        END_DEFINE_REGISTER(INTR_MASK_0)
    } XUSB_DEV;
} // namespace t210
