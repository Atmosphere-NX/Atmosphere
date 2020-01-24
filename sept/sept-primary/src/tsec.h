/*
 * Copyright (c) 2018 naehrwert
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
 
#ifndef FUSEE_TSEC_H_
#define FUSEE_TSEC_H_

#include <string.h>
#include <stdbool.h>

#define TSEC_BASE 0x54500000
#define SOR1_BASE 0x54580000
#define KFUSE_BASE 0x7000FC00

#define SOR1_DP_HDCP_BKSV_LSB       MAKE_REG32(SOR1_BASE + 0x1E8)
#define SOR1_TMDS_HDCP_BKSV_LSB     MAKE_REG32(SOR1_BASE + 0x21C)
#define SOR1_TMDS_HDCP_CN_MSB       MAKE_REG32(SOR1_BASE + 0x208)
#define SOR1_TMDS_HDCP_CN_LSB       MAKE_REG32(SOR1_BASE + 0x20C)

#define KFUSE_FUSECTRL              MAKE_REG32(KFUSE_BASE + 0x00)
#define KFUSE_FUSEADDR              MAKE_REG32(KFUSE_BASE + 0x04)
#define KFUSE_FUSEDATA0             MAKE_REG32(KFUSE_BASE + 0x08)
#define KFUSE_FUSEWRDATA0           MAKE_REG32(KFUSE_BASE + 0x0C)
#define KFUSE_FUSETIME_RD1          MAKE_REG32(KFUSE_BASE + 0x10)
#define KFUSE_FUSETIME_RD2          MAKE_REG32(KFUSE_BASE + 0x14)
#define KFUSE_FUSETIME_PGM1         MAKE_REG32(KFUSE_BASE + 0x18)
#define KFUSE_FUSETIME_PGM2         MAKE_REG32(KFUSE_BASE + 0x1C)
#define KFUSE_REGULATOR             MAKE_REG32(KFUSE_BASE + 0x20)
#define KFUSE_PD                    MAKE_REG32(KFUSE_BASE + 0x24)
#define KFUSE_FUSETIME_RD3          MAKE_REG32(KFUSE_BASE + 0x28)
#define KFUSE_STATE                 MAKE_REG32(KFUSE_BASE + 0x80)
#define KFUSE_ERRCOUNT              MAKE_REG32(KFUSE_BASE + 0x84)
#define KFUSE_KEYADDR               MAKE_REG32(KFUSE_BASE + 0x88)
#define KFUSE_KEYS                  MAKE_REG32(KFUSE_BASE + 0x8C)

typedef struct {
    uint32_t TSEC_THI_INCR_SYNCPT;                  /* Tegra Host Interface registers. */
    uint32_t TSEC_THI_INCR_SYNCPT_CTRL;
    uint32_t TSEC_THI_INCR_SYNCPT_ERR;
    uint32_t TSEC_THI_CTXSW_INCR_SYNCPT;
    uint32_t _0x10[0x4];
    uint32_t TSEC_THI_CTXSW;
    uint32_t TSEC_THI_CTXSW_NEXT;
    uint32_t TSEC_THI_CONT_SYNCPT_EOF;
    uint32_t TSEC_THI_CONT_SYNCPT_L1;
    uint32_t TSEC_THI_STREAMID0;
    uint32_t TSEC_THI_STREAMID1;
    uint32_t TSEC_THI_THI_SEC;
    uint32_t _0x3C;
    uint32_t TSEC_THI_METHOD0;
    uint32_t TSEC_THI_METHOD1;
    uint32_t _0x48[0x6];
    uint32_t TSEC_THI_CONTEXT_SWITCH;
    uint32_t _0x64[0x5];
    uint32_t TSEC_THI_INT_STATUS;
    uint32_t TSEC_THI_INT_MASK;
    uint32_t TSEC_THI_CONFIG0;
    uint32_t TSEC_THI_DBG_MISC;
    uint32_t TSEC_THI_SLCG_OVERRIDE_HIGH_A;
    uint32_t TSEC_THI_SLCG_OVERRIDE_LOW_A;
    uint32_t _0x90[0x35C];
    uint32_t TSEC_THI_CLK_OVERRIDE;
    uint32_t _0xE04[0x7F];
    uint32_t TSEC_FALCON_IRQSSET;                   /* Falcon microcontroller registers. */
    uint32_t TSEC_FALCON_IRQSCLR;
    uint32_t TSEC_FALCON_IRQSTAT;
    uint32_t TSEC_FALCON_IRQMODE;
    uint32_t TSEC_FALCON_IRQMSET;
    uint32_t TSEC_FALCON_IRQMCLR;
    uint32_t TSEC_FALCON_IRQMASK;
    uint32_t TSEC_FALCON_IRQDEST;
    uint32_t TSEC_FALCON_GPTMRINT;
    uint32_t TSEC_FALCON_GPTMRVAL;
    uint32_t TSEC_FALCON_GPTMRCTL;
    uint32_t TSEC_FALCON_PTIMER0;
    uint32_t TSEC_FALCON_PTIMER1;
    uint32_t TSEC_FALCON_WDTMRVAL;
    uint32_t TSEC_FALCON_WDTMRCTL;
    uint32_t TSEC_FALCON_IRQDEST2;
    uint32_t TSEC_FALCON_MAILBOX0;
    uint32_t TSEC_FALCON_MAILBOX1;
    uint32_t TSEC_FALCON_ITFEN;
    uint32_t TSEC_FALCON_IDLESTATE;
    uint32_t TSEC_FALCON_CURCTX;
    uint32_t TSEC_FALCON_NXTCTX;
    uint32_t TSEC_FALCON_CTXACK;
    uint32_t TSEC_FALCON_FHSTATE;
    uint32_t TSEC_FALCON_PRIVSTATE;
    uint32_t TSEC_FALCON_MTHDDATA;
    uint32_t TSEC_FALCON_MTHDID;
    uint32_t TSEC_FALCON_MTHDWDAT;
    uint32_t TSEC_FALCON_MTHDCOUNT;
    uint32_t TSEC_FALCON_MTHDPOP;
    uint32_t TSEC_FALCON_MTHDRAMSZ;
    uint32_t TSEC_FALCON_SFTRESET;
    uint32_t TSEC_FALCON_OS;
    uint32_t TSEC_FALCON_RM;
    uint32_t TSEC_FALCON_SOFT_PM;
    uint32_t TSEC_FALCON_SOFT_MODE;
    uint32_t TSEC_FALCON_DEBUG1;
    uint32_t TSEC_FALCON_DEBUGINFO;
    uint32_t TSEC_FALCON_IBRKPT1;
    uint32_t TSEC_FALCON_IBRKPT2;
    uint32_t TSEC_FALCON_CGCTL;
    uint32_t TSEC_FALCON_ENGCTL;
    uint32_t TSEC_FALCON_PMM;
    uint32_t TSEC_FALCON_ADDR;
    uint32_t TSEC_FALCON_IBRKPT3;
    uint32_t TSEC_FALCON_IBRKPT4;
    uint32_t TSEC_FALCON_IBRKPT5;
    uint32_t _0x10BC[0x5];
    uint32_t TSEC_FALCON_EXCI;
    uint32_t TSEC_FALCON_SVEC_SPR;
    uint32_t TSEC_FALCON_RSTAT0;
    uint32_t TSEC_FALCON_RSTAT3;
    uint32_t _0x10E0[0x8];
    uint32_t TSEC_FALCON_CPUCTL;
    uint32_t TSEC_FALCON_BOOTVEC;
    uint32_t TSEC_FALCON_HWCFG;
    uint32_t TSEC_FALCON_DMACTL;
    uint32_t TSEC_FALCON_DMATRFBASE;
    uint32_t TSEC_FALCON_DMATRFMOFFS;
    uint32_t TSEC_FALCON_DMATRFCMD;
    uint32_t TSEC_FALCON_DMATRFFBOFFS;
    uint32_t TSEC_FALCON_DMAPOLL_FB;
    uint32_t TSEC_FALCON_DMAPOLL_CP;
    uint32_t TSEC_FALCON_DBG_STATE;
    uint32_t TSEC_FALCON_HWCFG1;
    uint32_t TSEC_FALCON_CPUCTL_ALIAS;
    uint32_t _0x1134;
    uint32_t TSEC_FALCON_STACKCFG;
    uint32_t _0x113C;
    uint32_t TSEC_FALCON_IMCTL;
    uint32_t TSEC_FALCON_IMSTAT;
    uint32_t TSEC_FALCON_TRACEIDX;
    uint32_t TSEC_FALCON_TRACEPC;
    uint32_t TSEC_FALCON_IMFILLRNG0;
    uint32_t TSEC_FALCON_IMFILLRNG1;
    uint32_t TSEC_FALCON_IMFILLCTL;
    uint32_t TSEC_FALCON_IMCTL_DEBUG;
    uint32_t TSEC_FALCON_CMEMBASE;
    uint32_t TSEC_FALCON_DMEMAPERT;
    uint32_t TSEC_FALCON_EXTERRADDR;
    uint32_t TSEC_FALCON_EXTERRSTAT;
    uint32_t _0x1170[0x3];
    uint32_t TSEC_FALCON_CG2;
    uint32_t TSEC_FALCON_IMEMC0;
    uint32_t TSEC_FALCON_IMEMD0;
    uint32_t TSEC_FALCON_IMEMT0;
    uint32_t _0x118C;
    uint32_t TSEC_FALCON_IMEMC1;
    uint32_t TSEC_FALCON_IMEMD1;
    uint32_t TSEC_FALCON_IMEMT1;
    uint32_t _0x119C;
    uint32_t TSEC_FALCON_IMEMC2;
    uint32_t TSEC_FALCON_IMEMD2;
    uint32_t TSEC_FALCON_IMEMT2;
    uint32_t _0x11AC;
    uint32_t TSEC_FALCON_IMEMC3;
    uint32_t TSEC_FALCON_IMEMD3;
    uint32_t TSEC_FALCON_IMEMT3;
    uint32_t _0x11BC;
    uint32_t TSEC_FALCON_DMEMC0;
    uint32_t TSEC_FALCON_DMEMD0;
    uint32_t TSEC_FALCON_DMEMC1;
    uint32_t TSEC_FALCON_DMEMD1;
    uint32_t TSEC_FALCON_DMEMC2;
    uint32_t TSEC_FALCON_DMEMD2;
    uint32_t TSEC_FALCON_DMEMC3;
    uint32_t TSEC_FALCON_DMEMD3;
    uint32_t TSEC_FALCON_DMEMC4;
    uint32_t TSEC_FALCON_DMEMD4;
    uint32_t TSEC_FALCON_DMEMC5;
    uint32_t TSEC_FALCON_DMEMD5;
    uint32_t TSEC_FALCON_DMEMC6;
    uint32_t TSEC_FALCON_DMEMD6;
    uint32_t TSEC_FALCON_DMEMC7;
    uint32_t TSEC_FALCON_DMEMD7;
    uint32_t TSEC_FALCON_ICD_CMD;
    uint32_t TSEC_FALCON_ICD_ADDR;
    uint32_t TSEC_FALCON_ICD_WDATA;
    uint32_t TSEC_FALCON_ICD_RDATA;
    uint32_t _0x1210[0xC];
    uint32_t TSEC_FALCON_SCTL;
    uint32_t TSEC_FALCON_SSTAT;
    uint32_t _0x1248[0xE];
    uint32_t TSEC_FALCON_SPROT_IMEM;
    uint32_t TSEC_FALCON_SPROT_DMEM;
    uint32_t TSEC_FALCON_SPROT_CPUCTL;
    uint32_t TSEC_FALCON_SPROT_MISC;
    uint32_t TSEC_FALCON_SPROT_IRQ;
    uint32_t TSEC_FALCON_SPROT_MTHD;
    uint32_t TSEC_FALCON_SPROT_SCTL;
    uint32_t TSEC_FALCON_SPROT_WDTMR;
    uint32_t _0x12A0[0x8];
    uint32_t TSEC_FALCON_DMAINFO_FINISHED_FBRD_LOW;
    uint32_t TSEC_FALCON_DMAINFO_FINISHED_FBRD_HIGH;
    uint32_t TSEC_FALCON_DMAINFO_FINISHED_FBWR_LOW;
    uint32_t TSEC_FALCON_DMAINFO_FINISHED_FBWR_HIGH;
    uint32_t TSEC_FALCON_DMAINFO_CURRENT_FBRD_LOW;
    uint32_t TSEC_FALCON_DMAINFO_CURRENT_FBRD_HIGH;
    uint32_t TSEC_FALCON_DMAINFO_CURRENT_FBWR_LOW;
    uint32_t TSEC_FALCON_DMAINFO_CURRENT_FBWR_HIGH;
    uint32_t TSEC_FALCON_DMAINFO_CTL;
    uint32_t _0x12E4[0x47];
    uint32_t TSEC_SCP_CTL0;                         /* Secure Co-processor registers. */
    uint32_t TSEC_SCP_CTL1;
    uint32_t TSEC_SCP_CTL_STAT;
    uint32_t TSEC_SCP_CTL_LOCK;
    uint32_t TSEC_SCP_CFG;
    uint32_t TSEC_SCP_CTL_SCP;
    uint32_t TSEC_SCP_CTL_PKEY;
    uint32_t TSEC_SCP_CTL_DBG;
    uint32_t TSEC_SCP_DBG0;
    uint32_t TSEC_SCP_DBG1;
    uint32_t TSEC_SCP_DBG2;
    uint32_t _0x142C;
    uint32_t TSEC_SCP_CMD;
    uint32_t _0x1434[0x7];
    uint32_t TSEC_SCP_STAT0;
    uint32_t TSEC_SCP_STAT1;
    uint32_t TSEC_SCP_STAT2;
    uint32_t _0x145C[0x5];
    uint32_t TSEC_SCP_RND_STAT0;
    uint32_t TSEC_SCP_RND_STAT1;
    uint32_t _0x1478[0x2];
    uint32_t TSEC_SCP_IRQSTAT;
    uint32_t TSEC_SCP_IRQMASK;
    uint32_t _0x1488[0x2];
    uint32_t TSEC_SCP_ACL_ERR;
    uint32_t TSEC_SCP_SEC_ERR;
    uint32_t TSEC_SCP_CMD_ERR;
    uint32_t _0x149C[0x19];
    uint32_t TSEC_RND_CTL0;                         /* Random Number Generator registers. */
    uint32_t TSEC_RND_CTL1;
    uint32_t TSEC_RND_CTL2;
    uint32_t TSEC_RND_CTL3;
    uint32_t TSEC_RND_CTL4;
    uint32_t TSEC_RND_CTL5;
    uint32_t TSEC_RND_CTL6;
    uint32_t TSEC_RND_CTL7;
    uint32_t TSEC_RND_CTL8;
    uint32_t TSEC_RND_CTL9;
    uint32_t TSEC_RND_CTL10;
    uint32_t TSEC_RND_CTL11;
    uint32_t _0x1530[0x34];
    uint32_t TSEC_TFBIF_CTL;                        /* Tegra Framebuffer Interface registers. */
    uint32_t TSEC_TFBIF_MCCIF_FIFOCTRL;
    uint32_t TSEC_TFBIF_THROTTLE;
    uint32_t TSEC_TFBIF_DBG_STAT0;
    uint32_t TSEC_TFBIF_DBG_STAT1;
    uint32_t TSEC_TFBIF_DBG_RDCOUNT_LO;
    uint32_t TSEC_TFBIF_DBG_RDCOUNT_HI;
    uint32_t TSEC_TFBIF_DBG_WRCOUNT_LO;
    uint32_t TSEC_TFBIF_DBG_WRCOUNT_HI;
    uint32_t TSEC_TFBIF_DBG_R32COUNT;
    uint32_t TSEC_TFBIF_DBG_R64COUNT;
    uint32_t TSEC_TFBIF_DBG_R128COUNT;
    uint32_t _0x1630;
    uint32_t TSEC_TFBIF_MCCIF_FIFOCTRL1;
    uint32_t TSEC_TFBIF_WRR_RDP;
    uint32_t _0x163C;
    uint32_t TSEC_TFBIF_SPROT_EMEM;
    uint32_t TSEC_TFBIF_TRANSCFG;
    uint32_t TSEC_TFBIF_REGIONCFG;
    uint32_t TSEC_TFBIF_ACTMON_ACTIVE_MASK;
    uint32_t TSEC_TFBIF_ACTMON_ACTIVE_BORPS;
    uint32_t TSEC_TFBIF_ACTMON_ACTIVE_WEIGHT;
    uint32_t _0x1658[0x2];
    uint32_t TSEC_TFBIF_ACTMON_MCB_MASK;
    uint32_t TSEC_TFBIF_ACTMON_MCB_BORPS;
    uint32_t TSEC_TFBIF_ACTMON_MCB_WEIGHT;
    uint32_t _0x166C;
    uint32_t TSEC_TFBIF_THI_TRANSPROP;
    uint32_t _0x1674[0x17];
    uint32_t TSEC_CG;                               /* Clock Gate registers. */
    uint32_t _0x16D4[0xB];
    uint32_t TSEC_BAR0_CTL;                         /* HOST1X device DMA registers. */
    uint32_t TSEC_BAR0_ADDR;
    uint32_t TSEC_BAR0_DATA;
    uint32_t TSEC_BAR0_TIMEOUT;
    uint32_t _0x1710[0x3C];
    uint32_t TSEC_TEGRA_FALCON_IP_VER;              /* Miscellaneous registers. */
    uint32_t _0x1804[0xD];
    uint32_t TSEC_TEGRA_CTL;
    uint32_t _0x183C[0x31];
} tegra_tsec_t;

static inline volatile tegra_tsec_t *tsec_get_regs(void)
{
    return (volatile tegra_tsec_t *)TSEC_BASE;
}

#endif