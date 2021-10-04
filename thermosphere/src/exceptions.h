/*
 * Copyright (c) Atmosphère-NX
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

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

/**
 * Borrowed fom Xen (not copyrightable as these are facts).
 * Description of the EL2 exception syndrome register.
 */
#define HSR_EC_UNKNOWN              0x00
#define HSR_EC_WFI_WFE              0x01
#define HSR_EC_CP15_32              0x03
#define HSR_EC_CP15_64              0x04
#define HSR_EC_CP14_32              0x05        /* Trapped MCR or MRC access to CP14 */
#define HSR_EC_CP14_DBG             0x06        /* Trapped LDC/STC access to CP14 (only for debug registers) */
#define HSR_EC_CP                   0x07        /* HCPTR-trapped access to CP0-CP13 */
#define HSR_EC_CP10                 0x08
#define HSR_EC_JAZELLE              0x09
#define HSR_EC_BXJ                  0x0a
#define HSR_EC_CP14_64              0x0c
#define HSR_EC_SVC32                0x11
#define HSR_EC_HVC32                0x12
#define HSR_EC_SMC32                0x13
#define HSR_EC_HVC64                0x16
#define HSR_EC_SMC64                0x17
#define HSR_EC_SYSREG               0x18
#define HSR_EC_INSTR_ABORT_LOWER_EL 0x20
#define HSR_EC_INSTR_ABORT_CURR_EL  0x21
#define HSR_EC_DATA_ABORT_LOWER_EL  0x24
#define HSR_EC_DATA_ABORT_CURR_EL   0x25
#define HSR_EC_BRK                  0x3c

/**
 * Borrowed fom Xen (not copyrightable as these are facts).
 * Description of the EL2 exception syndrome register.
 */
union esr {
    uint32_t bits;
    struct {
        unsigned long iss:25;  /* Instruction Specific Syndrome */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    };

    /* Common to all conditional exception classes (0x0N, except 0x00). */
    struct hsr_cond {
        unsigned long iss:20;  /* Instruction Specific Syndrome */
        unsigned long cc:4;    /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } cond;

    struct hsr_wfi_wfe {
        unsigned long ti:1;    /* Trapped instruction */
        unsigned long sbzp:19;
        unsigned long cc:4;    /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } wfi_wfe;

    /* reg, reg0, reg1 are 4 bits on AArch32, the fifth bit is sbzp. */
    struct hsr_cp32 {
        unsigned long read:1;  /* Direction */
        unsigned long crm:4;   /* CRm */
        unsigned long reg:5;   /* Rt */
        unsigned long crn:4;   /* CRn */
        unsigned long op1:3;   /* Op1 */
        unsigned long op2:3;   /* Op2 */
        unsigned long cc:4;    /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } cp32; /* HSR_EC_CP15_32, CP14_32, CP10 */

    struct hsr_cp64 {
        unsigned long read:1;   /* Direction */
        unsigned long crm:4;    /* CRm */
        unsigned long reg1:5;   /* Rt1 */
        unsigned long reg2:5;   /* Rt2 */
        unsigned long sbzp2:1;
        unsigned long op1:4;    /* Op1 */
        unsigned long cc:4;     /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;    /* Instruction length */
        unsigned long ec:6;     /* Exception Class */
    } cp64; /* HSR_EC_CP15_64, HSR_EC_CP14_64 */

     struct hsr_cp {
        unsigned long coproc:4; /* Number of coproc accessed */
        unsigned long sbz0p:1;
        unsigned long tas:1;    /* Trapped Advanced SIMD */
        unsigned long res0:14;
        unsigned long cc:4;     /* Condition Code */
        unsigned long ccvalid:1;/* CC Valid */
        unsigned long len:1;    /* Instruction length */
        unsigned long ec:6;     /* Exception Class */
    } cp; /* HSR_EC_CP */

    struct hsr_sysreg {
        unsigned long read:1;   /* Direction */
        unsigned long crm:4;    /* CRm */
        unsigned long reg:5;    /* Rt */
        unsigned long crn:4;    /* CRn */
        unsigned long op1:3;    /* Op1 */
        unsigned long op2:3;    /* Op2 */
        unsigned long op0:2;    /* Op0 */
        unsigned long res0:3;
        unsigned long len:1;    /* Instruction length */
        unsigned long ec:6;
    } sysreg; /* HSR_EC_SYSREG */

    struct hsr_iabt {
        unsigned long ifsc:6;  /* Instruction fault status code */
        unsigned long res0:1;
        unsigned long s1ptw:1; /* Stage 2 fault during stage 1 translation */
        unsigned long res1:1;
        unsigned long eat:1;   /* External abort type */
        unsigned long res2:15;
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } iabt; /* HSR_EC_INSTR_ABORT_* */

    struct hsr_dabt {
        unsigned long dfsc:6;  /* Data Fault Status Code */
        unsigned long write:1; /* Write / not Read */
        unsigned long s1ptw:1; /* Stage 2 fault during stage 1 translation */
        unsigned long cache:1; /* Cache Maintenance */
        unsigned long eat:1;   /* External Abort Type */
        unsigned long sbzp0:4;
        unsigned long ar:1;    /* Acquire Release */
        unsigned long sf:1;    /* Sixty Four bit register */
        unsigned long reg:5;   /* Register */
        unsigned long sign:1;  /* Sign extend */
        unsigned long size:2;  /* Access Size */
        unsigned long valid:1; /* Syndrome Valid */
        unsigned long len:1;   /* Instruction length */
        unsigned long ec:6;    /* Exception Class */
    } dabt; /* HSR_EC_DATA_ABORT_* */

    struct hsr_brk {
        unsigned long comment:16;   /* Comment */
        unsigned long res0:9;
        unsigned long len:1;        /* Instruction length */
        unsigned long ec:6;         /* Exception Class */
    } brk;

};

/**
 * Structure that stores the saved register values on a hypercall.
 */
struct guest_state {
    uint64_t pc;
    uint64_t cpsr;

    uint64_t elr_el1;
    uint64_t spsr_el1;

    uint64_t sp_el0;
    uint64_t sp_el1;

    union esr esr_el2;
    uint64_t x[31];
}
__attribute__((packed));



#endif
