/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 Atmosphère-NX
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
 
#ifndef FUSEE_TSEC_H_
#define FUSEE_TSEC_H_

#include <string.h>
#include <stdbool.h>

#define TSEC_BASE 0x54500000
#define SOR1_BASE 0x54580000

#define SOR1_DP_HDCP_BKSV_LSB       MAKE_REG32(SOR1_BASE + 0x1E8)
#define SOR1_TMDS_HDCP_BKSV_LSB     MAKE_REG32(SOR1_BASE + 0x21C)
#define SOR1_TMDS_HDCP_CN_MSB       MAKE_REG32(SOR1_BASE + 0x208)
#define SOR1_TMDS_HDCP_CN_LSB       MAKE_REG32(SOR1_BASE + 0x20C)

typedef struct {
    uint8_t _0x0[0x1000];       /* Ignore non Falcon registers. */
    uint32_t FALCON_IRQSSET;
    uint32_t FALCON_IRQSCLR;
    uint32_t FALCON_IRQSTAT;
    uint32_t FALCON_IRQMODE;
    uint32_t FALCON_IRQMSET;
    uint32_t FALCON_IRQMCLR;
    uint32_t FALCON_IRQMASK;
    uint32_t FALCON_IRQDEST;
    uint8_t _0x1020[0x20];
    uint32_t FALCON_SCRATCH0;
    uint32_t FALCON_SCRATCH1;
    uint32_t FALCON_ITFEN;
    uint32_t FALCON_IDLESTATE;
    uint32_t FALCON_CURCTX;
    uint32_t FALCON_NXTCTX;
    uint8_t _0x1058[0x28];
    uint32_t FALCON_SCRATCH2;
    uint32_t FALCON_SCRATCH3;
    uint8_t _0x1088[0x18];
    uint32_t FALCON_CGCTL;
    uint32_t FALCON_ENGCTL;
    uint8_t _0x10A8[0x58];
    uint32_t FALCON_CPUCTL;
    uint32_t FALCON_BOOTVEC;
    uint32_t FALCON_HWCFG;
    uint32_t FALCON_DMACTL;
    uint32_t FALCON_DMATRFBASE;
    uint32_t FALCON_DMATRFMOFFS;
    uint32_t FALCON_DMATRFCMD;
    uint32_t FALCON_DMATRFFBOFFS;
    uint8_t _0x1120[0x10];
    uint32_t FALCON_CPUCTL_ALIAS;
    uint8_t _0x1134[0x20];
    uint32_t FALCON_IMFILLRNG1;
    uint32_t FALCON_IMFILLCTL;
    uint32_t _0x115C;
    uint32_t _0x1160;
    uint32_t _0x1164;
    uint32_t FALCON_EXTERRADDR;
    uint32_t FALCON_EXTERRSTAT;
    uint32_t _0x1170;
    uint32_t _0x1174;
    uint32_t _0x1178;
    uint32_t FALCON_CG2;
    uint32_t FALCON_CODE_INDEX;
    uint32_t FALCON_CODE;
    uint32_t FALCON_CODE_VIRT_ADDR;
    uint8_t _0x118C[0x34];
    uint32_t FALCON_DATA_INDEX0;
    uint32_t FALCON_DATA0;
    uint32_t FALCON_DATA_INDEX1;
    uint32_t FALCON_DATA1;
    uint32_t FALCON_DATA_INDEX2;
    uint32_t FALCON_DATA2;
    uint32_t FALCON_DATA_INDEX3;
    uint32_t FALCON_DATA3;
    uint32_t FALCON_DATA_INDEX4;
    uint32_t FALCON_DATA4;
    uint32_t FALCON_DATA_INDEX5;
    uint32_t FALCON_DATA5;
    uint32_t FALCON_DATA_INDEX6;
    uint32_t FALCON_DATA6;
    uint32_t FALCON_DATA_INDEX7;
    uint32_t FALCON_DATA7;
    uint32_t FALCON_ICD_CMD;
    uint32_t FALCON_ICD_ADDR;
    uint32_t FALCON_ICD_WDATA;
    uint32_t FALCON_ICD_RDATA;
    uint8_t _0x1210[0x30];
    uint32_t FALCON_SCTL;
    uint8_t _0x1244[0x5F8];     /* Ignore non Falcon registers. */
} tegra_tsec_t;

static inline volatile tegra_tsec_t *tsec_get_regs(void)
{
    return (volatile tegra_tsec_t *)TSEC_BASE;
}

void tsec_enable_clkrst();
void tsec_disable_clkrst();
int tsec_get_key(uint8_t *key, uint32_t rev, const void *tsec_fw, size_t tsec_fw_size);
int tsec_load_fw(const void *tsec_fw, size_t tsec_fw_size);
void tsec_run_fw();

#endif