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

#include "mtc.h"
#include "mtc_b01.h"
#include "mtc_tables_b01.h"
#include "car.h"
#include "fuse.h"
#include "timers.h"
#include "../../../fusee/common/log.h"

/*
 * Macros.
 */
#define max(x, y) ({                            \
    typeof(x) _max1 = (x);                  \
    typeof(y) _max2 = (y);                  \
    (void) (&_max1 == &_max2);              \
    _max1 > _max2 ? _max1 : _max2; })

#define max_t(type, x, y) ({                    \
    type __max1 = (x);                      \
    type __max2 = (y);                      \
    __max1 > __max2 ? __max1: __max2; })

/*
 * PTFV defines - basically just indexes into the per table PTFV array.
 */
#define PTFV_CONFIG_CTRL_USE_PREVIOUS_EMA   (1 << 0)

/*
 * Do arithmetic in fixed point.
 */
#define MOVAVG_PRECISION_FACTOR     100

/*
 * The division portion of the average operation.
 */
#define __AVERAGE_PTFV(dev)                     \
    ({ dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_ ## dev = \
       dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_ ## dev / \
       dst_timing_tables->ptfv_list.ptfv_dvfs_samples; })

/*
 * The division portion of the average write operation.
 */
#define __AVERAGE_WRITE_PTFV(dev)                       \
    ({ dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_ ## dev = \
       dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_ ## dev / \
       dst_timing_tables->ptfv_list.ptfv_write_samples; })

/*
 * Convert val to fixed point and add it to the temporary average.
 */
#define __INCREMENT_PTFV(dev, val)                  \
    ({ dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_ ## dev += \
       ((val) * MOVAVG_PRECISION_FACTOR); })

/*
 * Convert a moving average back to integral form and return the value.
 */
#define __MOVAVG_AC(timing, dev)                    \
    ((timing)->ptfv_list.ptfv_dqsosc_movavg_ ## dev /    \
     MOVAVG_PRECISION_FACTOR)

/* Weighted update. */
#define __WEIGHTED_UPDATE_PTFV(dev, nval)               \
    do {                                \
                                    \
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_ ## dev =               \
            ((nval * MOVAVG_PRECISION_FACTOR) +     \
             (dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_ ## dev *         \
              dst_timing_tables->ptfv_list.ptfv_movavg_weight)) /         \
            (dst_timing_tables->ptfv_list.ptfv_movavg_weight + 1);        \
    } while (0)

/* Access a particular average. */
#define __MOVAVG(timing, dev)                      \
    ((timing)->ptfv_list.ptfv_dqsosc_movavg_ ## dev)


static int g_active_timing_table_idx = -1;
static uint32_t g_next_pll = 0;
static uint32_t g_program_pllm_ret = 0;
static uint32_t g_wrote_training_pattern = 0;
static uint32_t g_fsp_for_next_freq = 0;

static uint32_t g_periodic_timmer_compensation_intermediates[9 * 0x10] = {};

/* Register read/write helpers. */
static inline uint32_t mmio_read(volatile uint32_t *reg) {
    const uint32_t value = *reg;

    return value;
}

static inline void mmio_write(uint32_t val, volatile uint32_t *reg) {
    *reg = val;
}

static inline void mc_write(uint32_t val, uint32_t offset) {
    mmio_write(val, &(MAKE_MC_REG(offset)));
}

static inline uint32_t mc_read(uint32_t offset) {
    return mmio_read(&(MAKE_MC_REG(offset)));
}

static inline void emc_write(uint32_t val, uint32_t offset) {
    mmio_write(val, &(MAKE_EMC_REG(offset)));
}

static inline uint32_t emc_read(uint32_t offset) {
    return mmio_read(&(MAKE_EMC_REG(offset)));
}

static inline void emc0_write(uint32_t val, uint32_t offset) {
    mmio_write(val, &(MAKE_EMC0_REG(offset)));
}

static inline uint32_t emc0_read(uint32_t offset) {
    return mmio_read(&(MAKE_EMC0_REG(offset)));
}

static inline void emc1_write(uint32_t val, uint32_t offset) {
    mmio_write(val, &(MAKE_EMC1_REG(offset)));
}

static inline uint32_t emc1_read(uint32_t offset) {
    return mmio_read(&(MAKE_EMC1_REG(offset)));
}

static inline uint32_t car_read(volatile uint32_t *reg) {
    return mmio_read(reg);
}

static inline void car_write(uint32_t val, volatile uint32_t *reg) {
    mmio_write(val, reg);
}

static inline void emc_write_per_ch(uint32_t val, uint32_t addr, uint32_t fbio_cfg7) {
    if ((addr & 0xFFFFF000) == EMC0_BASE && (fbio_cfg7 & EMC_FBIO_CFG7_CH0_ENABLE)) {
        emc0_write(val, addr & 0xFFF);
    } else if ((addr & 0xFFFFF000) == EMC1_BASE && (fbio_cfg7 & EMC_FBIO_CFG7_CH1_ENABLE)) {
        emc1_write(val, addr & 0xFFF);
    }
}

/* Configure clock change sequence FIFO */
static void ccfifo_write(uint32_t ccfifo_addr, uint32_t ccfifo_data, uint32_t ccfifo_stall_cnt) {
    emc_write(ccfifo_data, EMC_CCFIFO_DATA);
    emc_write((ccfifo_addr & 0xFFFF) | ((ccfifo_stall_cnt & 0x7FFF) << 16) | 0x80000000, EMC_CCFIFO_ADDR);
}

static uint32_t g_burst_regs_addr[sizeof(t210b01_emc_burst_regs) / sizeof(uint32_t)] = {
    // TODO: By name instead of address
    EMC_BASE + 0x02C,
    EMC_BASE + 0x030,
    EMC_BASE + 0x590,
    EMC_BASE + 0x580,
    EMC_BASE + 0x0C0,
    EMC_BASE + 0x034,
    EMC_BASE + 0x038,
    EMC_BASE + 0x03C,
    EMC_BASE + 0x040,
    EMC_BASE + 0x044,
    EMC_BASE + 0x048,
    EMC_BASE + 0x144,
    EMC_BASE + 0x0AC,
    EMC_BASE + 0x0BC,
    EMC_BASE + 0x0F8,
    EMC_BASE + 0x0FC,
    EMC_BASE + 0x108,
    EMC_BASE + 0x10C,
    EMC_BASE + 0x5C0,
    EMC_BASE + 0x04C,
    EMC_BASE + 0x050,
    EMC_BASE + 0x054,
    EMC_BASE + 0x058,
    EMC_BASE + 0x0B8,
    EMC_BASE + 0x4E0,
    EMC_BASE + 0x05C,
    EMC_BASE + 0x498,
    EMC_BASE + 0x494,
    EMC_BASE + 0x2D0,
    EMC_BASE + 0x490,
    EMC_BASE + 0x48C,
    EMC_BASE + 0x060,
    EMC_BASE + 0x568,
    EMC_BASE + 0x468,
    EMC_BASE + 0x46C,
    EMC_BASE + 0x14C,
    EMC_BASE + 0x4A4,
    EMC_BASE + 0x150,
    EMC_BASE + 0x154,
    EMC_BASE + 0x56C,
    EMC_BASE + 0x064,
    EMC_BASE + 0x068,
    EMC_BASE + 0x06C,
    EMC_BASE + 0x2CC,
    EMC_BASE + 0x2D8,
    EMC_BASE + 0x2D4,
    EMC_BASE + 0x070,
    EMC_BASE + 0x074,
    EMC_BASE + 0x3DC,
    EMC_BASE + 0x078,
    EMC_BASE + 0x07C,
    EMC_BASE + 0x080,
    EMC_BASE + 0x084,
    EMC_BASE + 0x088,
    EMC_BASE + 0x08C,
    EMC_BASE + 0x11C,
    EMC_BASE + 0x118,
    EMC_BASE + 0x0B4,
    EMC_BASE + 0x090,
    EMC_BASE + 0x3E4,
    EMC_BASE + 0x094,
    EMC_BASE + 0x158,
    EMC_BASE + 0x15C,
    EMC_BASE + 0x098,
    EMC_BASE + 0x09C,
    EMC_BASE + 0x0A0,
    EMC_BASE + 0x0A4,
    EMC_BASE + 0x4A8,
    EMC_BASE + 0x0A8,
    EMC_BASE + 0x0B0,
    EMC_BASE + 0x104,
    EMC_BASE + 0x584,
    EMC_BASE + 0x2BC,
    EMC_BASE + 0x2C0,
    EMC_BASE + 0xCF4,
    EMC_BASE + 0x55C,
    EMC_BASE + 0x554,
    EMC_BASE + 0x610,
    EMC_BASE + 0x614,
    EMC_BASE + 0x630,
    EMC_BASE + 0x634,
    EMC_BASE + 0x4AC,
    EMC_BASE + 0x670,
    EMC_BASE + 0x674,
    EMC_BASE + 0x680,
    EMC_BASE + 0x684,
    EMC_BASE + 0x688,
    EMC_BASE + 0x68C,
    EMC_BASE + 0x690,
    EMC_BASE + 0x694,
    EMC_BASE + 0x6A0,
    EMC_BASE + 0x6A4,
    EMC_BASE + 0x6A8,
    EMC_BASE + 0x6AC,
    EMC_BASE + 0x6B0,
    EMC_BASE + 0x6B4,
    EMC_BASE + 0xC00,
    EMC_BASE + 0xC04,
    EMC_BASE + 0xC08,
    EMC_BASE + 0xC0C,
    EMC_BASE + 0xC10,
    EMC_BASE + 0xC20,
    EMC_BASE + 0xC24,
    EMC_BASE + 0xC28,
    EMC_BASE + 0x80C,
    EMC_BASE + 0x81C,
    EMC_BASE + 0x82C,
    EMC_BASE + 0x83C,
    EMC_BASE + 0x84C,
    EMC_BASE + 0x85C,
    EMC_BASE + 0x86C,
    EMC_BASE + 0x87C,
    EMC_BASE + 0x88C,
    EMC_BASE + 0x89C,
    EMC_BASE + 0x8AC,
    EMC_BASE + 0x8BC,
    EMC_BASE + 0x90C,
    EMC_BASE + 0x91C,
    EMC_BASE + 0x92C,
    EMC_BASE + 0x93C,
    EMC_BASE + 0x94C,
    EMC_BASE + 0x95C,
    EMC_BASE + 0x96C,
    EMC_BASE + 0x97C,
    EMC_BASE + 0x980,
    EMC_BASE + 0x984,
    EMC_BASE + 0x988,
    EMC_BASE + 0x98C,
    EMC_BASE + 0x990,
    EMC_BASE + 0x994,
    EMC_BASE + 0x998,
    EMC_BASE + 0x99C,
    EMC_BASE + 0x9A0,
    EMC_BASE + 0x9A4,
    EMC_BASE + 0x9A8,
    EMC_BASE + 0x9AC,
    EMC_BASE + 0x9B0,
    EMC_BASE + 0x9B4,
    EMC_BASE + 0x9B8,
    EMC_BASE + 0x9BC,
    EMC_BASE + 0x480,
    EMC_BASE + 0x310,
    EMC_BASE + 0x314,
    EMC_BASE + 0x100,
    EMC_BASE + 0x2E0,
    EMC_BASE + 0x2E4,
    EMC_BASE + 0x0C8,
    EMC_BASE + 0x0C4,
    EMC_BASE + 0x464,
    EMC_BASE + 0x5E4,
    EMC_BASE + 0x5E8,
    EMC_BASE + 0x5F8,
    EMC_BASE + 0xC78,
    EMC_BASE + 0xC44,
    EMC_BASE + 0x00C,
    EMC_BASE + 0x560,
    EMC_BASE + 0x3E0,
    EMC_BASE + 0x564,
    EMC_BASE + 0x594,
    EMC_BASE + 0x598,
    EMC_BASE + 0x5A4,
    EMC_BASE + 0x5A8,
    EMC_BASE + 0xC40,
    EMC_BASE + 0xC54,
    EMC_BASE + 0xC50,
    EMC_BASE + 0xC5C,
    EMC_BASE + 0xC58,
    EMC_BASE + 0xC60,
    EMC_BASE + 0xC64,
    EMC_BASE + 0xC34,
    EMC_BASE + 0xC38,
    EMC_BASE + 0xCF0,
    EMC_BASE + 0x330,
    EMC_BASE + 0x318,
    EMC_BASE + 0x334,
    EMC_BASE + 0x31C,
    EMC_BASE + 0xC3C,
    EMC_BASE + 0x49C,
    EMC_BASE + 0x720,
    EMC_BASE + 0x724,
    EMC_BASE + 0x728,
    EMC_BASE + 0x72C,
    EMC_BASE + 0x730,
    EMC_BASE + 0x734,
    EMC_BASE + 0x5F0,
    EMC_BASE + 0x740,
    EMC_BASE + 0x744,
    EMC_BASE + 0x748,
    EMC_BASE + 0x74C,
    EMC_BASE + 0x750,
    EMC_BASE + 0x754,
    EMC_BASE + 0x760,
    EMC_BASE + 0x770,
    EMC_BASE + 0x774,
    EMC_BASE + 0x778,
    EMC_BASE + 0x780,
    EMC_BASE + 0x784,
    EMC_BASE + 0x788,
    EMC_BASE + 0x110,
    EMC_BASE + 0x114,
    EMC_BASE + 0x3B4,
    EMC_BASE + 0x460,
    EMC_BASE + 0x3BC,
    EMC_BASE + 0x3C4,
    EMC_BASE + 0x3F4,
    EMC_BASE + 0x3F8,
    EMC_BASE + 0x4C4,
    EMC_BASE + 0x3FC,
    EMC_BASE + 0x400,
    EMC_BASE + 0xE04,
    EMC_BASE + 0xE44,
    EMC_BASE + 0xE6C,
    EMC_BASE + 0xE30,
    EMC_BASE + 0xE34,
    EMC_BASE + 0xE38,
    EMC_BASE + 0xE3C,
    EMC_BASE + 0xE0C,
    EMC_BASE + 0xE10,
    EMC_BASE + 0xE14,
    EMC_BASE + 0xED0,
    EMC_BASE + 0xE24,
    EMC_BASE + 0xE28,
    EMC_BASE + 0xE2C,
    EMC_BASE + 0xE18,
    EMC_BASE + 0xE1C,
    EMC_BASE + 0xE20,
    EMC_BASE + 0xE5C,
    EMC_BASE + 0x4D0,
};

static uint32_t g_burst_perch_regs_addr[sizeof(t210_emc_burst_reg_per_ch) / sizeof(uint32_t)] = {
    EMC0_BASE + EMC_MRW10,
    EMC1_BASE + EMC_MRW10,
    EMC0_BASE + EMC_MRW11,
    EMC1_BASE + EMC_MRW11,
    EMC0_BASE + EMC_MRW12,
    EMC1_BASE + EMC_MRW12,
    EMC0_BASE + EMC_MRW13,
    EMC1_BASE + EMC_MRW13,
};

static uint32_t g_vref_perch_regs_addr[sizeof(t210_emc_vref_perch_regs) / sizeof(uint32_t)] = {
    EMC0_BASE + EMC_TRAINING_OPT_DQS_IB_VREF_RANK0,
    EMC1_BASE + EMC_TRAINING_OPT_DQS_IB_VREF_RANK0,
    EMC0_BASE + EMC_TRAINING_OPT_DQS_IB_VREF_RANK1,
    EMC1_BASE + EMC_TRAINING_OPT_DQS_IB_VREF_RANK1,
};

static uint32_t g_training_mod_perch_regs_addr[sizeof(t210_emc_training_mod_regs) / sizeof(uint32_t)] = {
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_IB_BYTE0,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_IB_BYTE0,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_IB_BYTE1,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_IB_BYTE1,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_IB_BYTE2,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_IB_BYTE2,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_IB_BYTE3,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_IB_BYTE3,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_IB_MISC,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_IB_MISC,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_OB_BYTE0,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_OB_BYTE0,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_OB_BYTE1,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_OB_BYTE1,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_OB_BYTE2,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_OB_BYTE2,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_OB_BYTE3,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_OB_BYTE3,
    EMC0_BASE + EMC_TRAINING_RW_OFFSET_OB_MISC,
    EMC1_BASE + EMC_TRAINING_RW_OFFSET_OB_MISC,
};

static uint32_t g_trim_perch_regs_addr[sizeof(t210_emc_trim_perch_regs) / sizeof(uint32_t)] = {
    EMC0_BASE + EMC_CMD_BRLSHFT_0,
    EMC1_BASE + EMC_CMD_BRLSHFT_1,
    EMC0_BASE + EMC_DATA_BRLSHFT_0,
    EMC1_BASE + EMC_DATA_BRLSHFT_0,
    EMC0_BASE + EMC_DATA_BRLSHFT_1,
    EMC1_BASE + EMC_DATA_BRLSHFT_1,
    EMC0_BASE + EMC_QUSE_BRLSHFT_0,
    EMC1_BASE + EMC_QUSE_BRLSHFT_1,
    EMC0_BASE + EMC_QUSE_BRLSHFT_2,
    EMC1_BASE + EMC_QUSE_BRLSHFT_3,
};

static uint32_t g_trim_regs_addr[sizeof(t210_emc_trim_regs) / sizeof(uint32_t)] = {
    EMC_BASE + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3,
    EMC_BASE + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3,

    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2,

    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1,
    EMC_BASE + EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2,

    EMC_BASE + EMC_PMACRO_IB_VREF_DQS_0,
    EMC_BASE + EMC_PMACRO_IB_VREF_DQS_1,

    EMC_BASE + EMC_PMACRO_IB_VREF_DQ_0,
    EMC_BASE + EMC_PMACRO_IB_VREF_DQ_1,

    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3,

    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2,

    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2,

    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1,
    EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2,

    EMC_BASE + EMC_PMACRO_QUSE_DDLL_RANK0_0,
    EMC_BASE + EMC_PMACRO_QUSE_DDLL_RANK0_1,
    EMC_BASE + EMC_PMACRO_QUSE_DDLL_RANK0_2,
    EMC_BASE + EMC_PMACRO_QUSE_DDLL_RANK0_3,
    EMC_BASE + EMC_PMACRO_QUSE_DDLL_RANK1_0,
    EMC_BASE + EMC_PMACRO_QUSE_DDLL_RANK1_1,
    EMC_BASE + EMC_PMACRO_QUSE_DDLL_RANK1_2,
    EMC_BASE + EMC_PMACRO_QUSE_DDLL_RANK1_3,
};

static uint32_t g_burst_mc_regs_addr[sizeof(t210_emc_burst_mc_regs) / sizeof(uint32_t)] = {
    MC_BASE + MC_EMEM_ARB_CFG,
    MC_BASE + MC_EMEM_ARB_OUTSTANDING_REQ,
    MC_BASE + MC_EMEM_ARB_REFPB_HP_CTRL,
    MC_BASE + MC_EMEM_ARB_REFPB_BANK_CTRL,
    MC_BASE + MC_EMEM_ARB_TIMING_RCD,
    MC_BASE + MC_EMEM_ARB_TIMING_RP,
    MC_BASE + MC_EMEM_ARB_TIMING_RC,
    MC_BASE + MC_EMEM_ARB_TIMING_RAS,
    MC_BASE + MC_EMEM_ARB_TIMING_FAW,
    MC_BASE + MC_EMEM_ARB_TIMING_RRD,
    MC_BASE + MC_EMEM_ARB_TIMING_RAP2PRE,
    MC_BASE + MC_EMEM_ARB_TIMING_WAP2PRE,
    MC_BASE + MC_EMEM_ARB_TIMING_R2R,
    MC_BASE + MC_EMEM_ARB_TIMING_W2W,
    MC_BASE + MC_EMEM_ARB_TIMING_R2W,
    MC_BASE + MC_EMEM_ARB_TIMING_CCDMW,
    MC_BASE + MC_EMEM_ARB_TIMING_W2R,
    MC_BASE + MC_EMEM_ARB_TIMING_RFCPB,
    MC_BASE + MC_EMEM_ARB_DA_TURNS,
    MC_BASE + MC_EMEM_ARB_DA_COVERS,
    MC_BASE + MC_EMEM_ARB_MISC0,
    MC_BASE + MC_EMEM_ARB_MISC1,
    MC_BASE + MC_EMEM_ARB_MISC2,
    MC_BASE + MC_EMEM_ARB_RING1_THROTTLE,
    MC_BASE + MC_EMEM_ARB_DHYST_CTRL,
    MC_BASE + MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_0,
    MC_BASE + MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_1,
    MC_BASE + MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_2,
    MC_BASE + MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_3,
    MC_BASE + MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_4,
    MC_BASE + MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_5,
    MC_BASE + MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_6,
    MC_BASE + MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_7,
};

static uint32_t g_la_scale_regs_addr[sizeof(t210_emc_la_scale_regs) / sizeof(uint32_t)] = {
    MC_BASE + MC_MLL_MPCORER_PTSA_RATE,
    MC_BASE + MC_FTOP_PTSA_RATE,
    MC_BASE + MC_PTSA_GRANT_DECREMENT,
    MC_BASE + MC_LATENCY_ALLOWANCE_XUSB_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_XUSB_1,
    MC_BASE + MC_LATENCY_ALLOWANCE_TSEC_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_SDMMCA_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_SDMMCAA_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_SDMMC_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_SDMMCAB_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_PPCS_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_PPCS_1,
    MC_BASE + MC_LATENCY_ALLOWANCE_MPCORE_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_HC_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_HC_1,
    MC_BASE + MC_LATENCY_ALLOWANCE_AVPC_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_GPU_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_GPU2_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_NVENC_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_NVDEC_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_VIC_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_VI2_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_ISP2_0,
    MC_BASE + MC_LATENCY_ALLOWANCE_ISP2_1,
};

typedef struct {
    uint32_t dq[0x100];
    uint8_t dmi[0x100];
} tegra_b01_ram_pattern_t;
_Static_assert(sizeof(tegra_b01_ram_pattern_t) == 0x500);

static unsigned char g_ram_pattern_raw_data[sizeof(tegra_b01_ram_pattern_t) * 5] = {
    0x18, 0x18, 0x18, 0x18, 0x61, 0x61, 0x61, 0x61, 0x85, 0x85, 0x85, 0x85, 0x14, 0x14, 0x14, 0x14,
    0x51, 0x51, 0x51, 0x51, 0x47, 0x47, 0x47, 0x47, 0x1E, 0x1E, 0x1E, 0x1E, 0x79, 0x79, 0x79, 0x79,
    0xE5, 0xE5, 0xE5, 0xE5, 0x94, 0x94, 0x94, 0x94, 0x51, 0x51, 0x51, 0x51, 0x46, 0x46, 0x46, 0x46,
    0x19, 0x19, 0x19, 0x19, 0x67, 0x67, 0x67, 0x67, 0x9C, 0x9C, 0x9C, 0x9C, 0x71, 0x71, 0x71, 0x71,
    0xC5, 0xC5, 0xC5, 0xC5, 0x17, 0x17, 0x17, 0x17, 0x5F, 0x5F, 0x5F, 0x5F, 0x7E, 0x7E, 0x7E, 0x7E,
    0xFB, 0xFB, 0xFB, 0xFB, 0xED, 0xED, 0xED, 0xED, 0xB4, 0xB4, 0xB4, 0xB4, 0xD2, 0xD2, 0xD2, 0xD2,
    0x48, 0x48, 0x48, 0x48, 0x21, 0x21, 0x21, 0x21, 0x85, 0x85, 0x85, 0x85, 0x16, 0x16, 0x16, 0x16,
    0x59, 0x59, 0x59, 0x59, 0x66, 0x66, 0x66, 0x66, 0x9A, 0x9A, 0x9A, 0x9A, 0x69, 0x69, 0x69, 0x69,
    0xA4, 0xA4, 0xA4, 0xA4, 0x93, 0x93, 0x93, 0x93, 0x4F, 0x4F, 0x4F, 0x4F, 0x3F, 0x3F, 0x3F, 0x3F,
    0xFC, 0xFC, 0xFC, 0xFC, 0xF3, 0xF3, 0xF3, 0xF3, 0xCD, 0xCD, 0xCD, 0xCD, 0x37, 0x37, 0x37, 0x37,
    0xDC, 0xDC, 0xDC, 0xDC, 0x70, 0x70, 0x70, 0x70, 0xC3, 0xC3, 0xC3, 0xC3, 0x0F, 0x0F, 0x0F, 0x0F,
    0x3E, 0x3E, 0x3E, 0x3E, 0xFA, 0xFA, 0xFA, 0xFA, 0xEB, 0xEB, 0xEB, 0xEB, 0xAC, 0xAC, 0xAC, 0xAC,
    0xB3, 0xB3, 0xB3, 0xB3, 0xCC, 0xCC, 0xCC, 0xCC, 0x31, 0x31, 0x31, 0x31, 0xC5, 0xC5, 0xC5, 0xC5,
    0x15, 0x15, 0x15, 0x15, 0x57, 0x57, 0x57, 0x57, 0x5F, 0x5F, 0x5F, 0x5F, 0x7F, 0x7F, 0x7F, 0x7F,
    0xFD, 0xFD, 0xFD, 0xFD, 0xF4, 0xF4, 0xF4, 0xF4, 0xD0, 0xD0, 0xD0, 0xD0, 0x42, 0x42, 0x42, 0x42,
    0x08, 0x08, 0x08, 0x08, 0x23, 0x23, 0x23, 0x23, 0x8F, 0x8F, 0x8F, 0x8F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x18, 0x18, 0x18, 0x18, 0x61, 0x61, 0x61, 0x61, 0x85, 0x85, 0x85, 0x85, 0x14, 0x14, 0x14, 0x14,
    0x51, 0x51, 0x51, 0x51, 0x47, 0x47, 0x47, 0x47, 0x1E, 0x1E, 0x1E, 0x1E, 0x79, 0x79, 0x79, 0x79,
    0xE5, 0xE5, 0xE5, 0xE5, 0x94, 0x94, 0x94, 0x94, 0x51, 0x51, 0x51, 0x51, 0x46, 0x46, 0x46, 0x46,
    0x19, 0x19, 0x19, 0x19, 0x67, 0x67, 0x67, 0x67, 0x9C, 0x9C, 0x9C, 0x9C, 0x71, 0x71, 0x71, 0x71,
    0xC5, 0xC5, 0xC5, 0xC5, 0x17, 0x17, 0x17, 0x17, 0x5F, 0x5F, 0x5F, 0x5F, 0x7E, 0x7E, 0x7E, 0x7E,
    0xFB, 0xFB, 0xFB, 0xFB, 0xED, 0xED, 0xED, 0xED, 0xB4, 0xB4, 0xB4, 0xB4, 0xD2, 0xD2, 0xD2, 0xD2,
    0x48, 0x48, 0x48, 0x48, 0x21, 0x21, 0x21, 0x21, 0x85, 0x85, 0x85, 0x85, 0x16, 0x16, 0x16, 0x16,
    0x59, 0x59, 0x59, 0x59, 0x66, 0x66, 0x66, 0x66, 0x9A, 0x9A, 0x9A, 0x9A, 0x69, 0x69, 0x69, 0x69,
    0xA4, 0xA4, 0xA4, 0xA4, 0x93, 0x93, 0x93, 0x93, 0x4F, 0x4F, 0x4F, 0x4F, 0x3F, 0x3F, 0x3F, 0x3F,
    0xFC, 0xFC, 0xFC, 0xFC, 0xF3, 0xF3, 0xF3, 0xF3, 0xCD, 0xCD, 0xCD, 0xCD, 0x37, 0x37, 0x37, 0x37,
    0xDC, 0xDC, 0xDC, 0xDC, 0x70, 0x70, 0x70, 0x70, 0xC3, 0xC3, 0xC3, 0xC3, 0x0F, 0x0F, 0x0F, 0x0F,
    0x3E, 0x3E, 0x3E, 0x3E, 0xFA, 0xFA, 0xFA, 0xFA, 0xEB, 0xEB, 0xEB, 0xEB, 0xAC, 0xAC, 0xAC, 0xAC,
    0xB3, 0xB3, 0xB3, 0xB3, 0xCC, 0xCC, 0xCC, 0xCC, 0x31, 0x31, 0x31, 0x31, 0xC5, 0xC5, 0xC5, 0xC5,
    0x15, 0x15, 0x15, 0x15, 0x57, 0x57, 0x57, 0x57, 0x5F, 0x5F, 0x5F, 0x5F, 0x7F, 0x7F, 0x7F, 0x7F,
    0xFD, 0xFD, 0xFD, 0xFD, 0xF4, 0xF4, 0xF4, 0xF4, 0xD0, 0xD0, 0xD0, 0xD0, 0x42, 0x42, 0x42, 0x42,
    0x08, 0x08, 0x08, 0x08, 0x23, 0x23, 0x23, 0x23, 0x8F, 0x8F, 0x8F, 0x8F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x06, 0x06, 0x06, 0x06, 0x18, 0x18, 0x18, 0x18, 0x21, 0x21, 0x21, 0x21, 0x05, 0x05, 0x05, 0x05,
    0x14, 0x14, 0x14, 0x14, 0x11, 0x11, 0x11, 0x11, 0x07, 0x07, 0x07, 0x07, 0x1E, 0x1E, 0x1E, 0x1E,
    0x39, 0x39, 0x39, 0x39, 0x25, 0x25, 0x25, 0x25, 0x14, 0x14, 0x14, 0x14, 0x11, 0x11, 0x11, 0x11,
    0x06, 0x06, 0x06, 0x06, 0x19, 0x19, 0x19, 0x19, 0x27, 0x27, 0x27, 0x27, 0x1C, 0x1C, 0x1C, 0x1C,
    0x31, 0x31, 0x31, 0x31, 0x05, 0x05, 0x05, 0x05, 0x17, 0x17, 0x17, 0x17, 0x1F, 0x1F, 0x1F, 0x1F,
    0x3E, 0x3E, 0x3E, 0x3E, 0x3B, 0x3B, 0x3B, 0x3B, 0x2D, 0x2D, 0x2D, 0x2D, 0x34, 0x34, 0x34, 0x34,
    0x12, 0x12, 0x12, 0x12, 0x08, 0x08, 0x08, 0x08, 0x21, 0x21, 0x21, 0x21, 0x05, 0x05, 0x05, 0x05,
    0x16, 0x16, 0x16, 0x16, 0x19, 0x19, 0x19, 0x19, 0x26, 0x26, 0x26, 0x26, 0x1A, 0x1A, 0x1A, 0x1A,
    0x29, 0x29, 0x29, 0x29, 0x24, 0x24, 0x24, 0x24, 0x13, 0x13, 0x13, 0x13, 0x0F, 0x0F, 0x0F, 0x0F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3C, 0x3C, 0x3C, 0x3C, 0x33, 0x33, 0x33, 0x33, 0x0D, 0x0D, 0x0D, 0x0D,
    0x37, 0x37, 0x37, 0x37, 0x1C, 0x1C, 0x1C, 0x1C, 0x30, 0x30, 0x30, 0x30, 0x03, 0x03, 0x03, 0x03,
    0x0F, 0x0F, 0x0F, 0x0F, 0x3E, 0x3E, 0x3E, 0x3E, 0x3A, 0x3A, 0x3A, 0x3A, 0x2B, 0x2B, 0x2B, 0x2B,
    0x2C, 0x2C, 0x2C, 0x2C, 0x33, 0x33, 0x33, 0x33, 0x0C, 0x0C, 0x0C, 0x0C, 0x31, 0x31, 0x31, 0x31,
    0x05, 0x05, 0x05, 0x05, 0x15, 0x15, 0x15, 0x15, 0x17, 0x17, 0x17, 0x17, 0x1F, 0x1F, 0x1F, 0x1F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3D, 0x3D, 0x3D, 0x3D, 0x34, 0x34, 0x34, 0x34, 0x10, 0x10, 0x10, 0x10,
    0x02, 0x02, 0x02, 0x02, 0x08, 0x08, 0x08, 0x08, 0x23, 0x23, 0x23, 0x23, 0x0F, 0x0F, 0x0F, 0x0F,
    0x06, 0x06, 0x06, 0x06, 0x18, 0x18, 0x18, 0x18, 0x21, 0x21, 0x21, 0x21, 0x05, 0x05, 0x05, 0x05,
    0x14, 0x14, 0x14, 0x14, 0x11, 0x11, 0x11, 0x11, 0x07, 0x07, 0x07, 0x07, 0x1E, 0x1E, 0x1E, 0x1E,
    0x39, 0x39, 0x39, 0x39, 0x25, 0x25, 0x25, 0x25, 0x14, 0x14, 0x14, 0x14, 0x11, 0x11, 0x11, 0x11,
    0x06, 0x06, 0x06, 0x06, 0x19, 0x19, 0x19, 0x19, 0x27, 0x27, 0x27, 0x27, 0x1C, 0x1C, 0x1C, 0x1C,
    0x31, 0x31, 0x31, 0x31, 0x05, 0x05, 0x05, 0x05, 0x17, 0x17, 0x17, 0x17, 0x1F, 0x1F, 0x1F, 0x1F,
    0x3E, 0x3E, 0x3E, 0x3E, 0x3B, 0x3B, 0x3B, 0x3B, 0x2D, 0x2D, 0x2D, 0x2D, 0x34, 0x34, 0x34, 0x34,
    0x12, 0x12, 0x12, 0x12, 0x08, 0x08, 0x08, 0x08, 0x21, 0x21, 0x21, 0x21, 0x05, 0x05, 0x05, 0x05,
    0x16, 0x16, 0x16, 0x16, 0x19, 0x19, 0x19, 0x19, 0x26, 0x26, 0x26, 0x26, 0x1A, 0x1A, 0x1A, 0x1A,
    0x29, 0x29, 0x29, 0x29, 0x24, 0x24, 0x24, 0x24, 0x13, 0x13, 0x13, 0x13, 0x0F, 0x0F, 0x0F, 0x0F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3C, 0x3C, 0x3C, 0x3C, 0x33, 0x33, 0x33, 0x33, 0x0D, 0x0D, 0x0D, 0x0D,
    0x37, 0x37, 0x37, 0x37, 0x1C, 0x1C, 0x1C, 0x1C, 0x30, 0x30, 0x30, 0x30, 0x03, 0x03, 0x03, 0x03,
    0x0F, 0x0F, 0x0F, 0x0F, 0x3E, 0x3E, 0x3E, 0x3E, 0x3A, 0x3A, 0x3A, 0x3A, 0x2B, 0x2B, 0x2B, 0x2B,
    0x2C, 0x2C, 0x2C, 0x2C, 0x33, 0x33, 0x33, 0x33, 0x0C, 0x0C, 0x0C, 0x0C, 0x31, 0x31, 0x31, 0x31,
    0x05, 0x05, 0x05, 0x05, 0x15, 0x15, 0x15, 0x15, 0x17, 0x17, 0x17, 0x17, 0x1F, 0x1F, 0x1F, 0x1F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3D, 0x3D, 0x3D, 0x3D, 0x34, 0x34, 0x34, 0x34, 0x10, 0x10, 0x10, 0x10,
    0x02, 0x02, 0x02, 0x02, 0x08, 0x08, 0x08, 0x08, 0x23, 0x23, 0x23, 0x23, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x0F, 0x00, 0x0F, 0x0F,
    0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x0F, 0x00, 0x00, 0x0F, 0x00,
    0x0F, 0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00,
    0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x0F, 0x00, 0x0F, 0x0F,
    0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x0F, 0x00, 0x00, 0x0F, 0x00,
    0x0F, 0x0F, 0x0F, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x0F, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x00, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x0F, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x0F, 0x0F, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x00, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x40, 0x40, 0x40, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x20, 0x20, 0x20, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x40, 0x40, 0x40, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x20, 0x20, 0x20, 0x20, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00, 0x0F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xCC, 0xCC, 0xCC, 0xCC, 0x33, 0x33, 0x33, 0x33,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03,
    0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03, 0x0A, 0x05, 0x0C, 0x03
};

static int get_emc_dvfs_timing_table_index(uint32_t dram_id) {
    switch (dram_id) {
        case 0x0: /* DramId_EristaIcosaSamsung4gb */
            return 0;
        case 0x1: /* DramId_EristaIcosaHynix4gb */
            return 2;
        case 0x2: /* DramId_EristaIcosaMicron4gb */
            return 3;
        case 0x3: /* DramId_MarikoIowaHynix1y4gb */
            return 0x10;
        case 0x4: /* DramId_EristaIcosaSamsung6gb */
            return 1;
        case 0x5: /* DramId_MarikoHoagHynix1y4gb */
            return 0x10;
        case 0x6: /* DramId_MarikoAulaHynix1y4gb */
            return 0;
        case 0x7: /* DramId_MarikoIowax1x2Samsung4gb */
            return 0;
        case 0x8: /* DramId_MarikoIowaSamsung4gb */
            return 5;
        case 0x9: /* DramId_MarikoIowaSamsung8gb */
            return 6;
        case 0xA: /* DramId_MarikoIowaHynix4gb */
            return 7;
        case 0xB: /* DramId_MarikoIowaMicron4gb */
            return 8;
        case 0xC: /* DramId_MarikoHoagSamsung4gb */
            return 5;
        case 0xD: /* DramId_MarikoHoagSamsung8gb */
            return 6;
        case 0xE: /* DramId_MarikoHoagHynix4gb */
            return 7;
        case 0xF: /* DramId_MarikoHoagMicron4gb */
            return 8;
        case 0x10: /* DramId_MarikoIowaSamsung4gbY */
            return 9;
        case 0x11: /* DramId_MarikoIowaSamsung1y4gbX */
            return 0xC;
        case 0x12: /* DramId_MarikoIowaSamsung1y8gbX */
            return 0xD;
        case 0x13: /* DramId_MarikoHoagSamsung1y4gbX */
            return 0xC;
        case 0x14: /* DramId_MarikoIowaSamsung1y4gbY */
            return 0xA;
        case 0x15: /* DramId_MarikoIowaSamsung1y8gbY */
            return 0xB;
        case 0x16: /* DramId_MarikoAulaSamsung1y4gb */
            return 0xE;
        case 0x17: /* DramId_MarikoHoagSamsung1y8gbX */
            return 0xD;
        case 0x18: /* DramId_MarikoAulaSamsung1y4gbX */
            return 0xC;
        case 0x19: /* DramId_MarikoIowaMicron1y4gb */
            return 0xF;
        case 0x1A: /* DramId_MarikoHoagMicron1y4gb */
            return 0xF;
        case 0x1B: /* DramId_MarikoAulaMicron1y4gb */
            return 0xF;
        case 0x1C: /* DramId_MarikoAulaSamsung1y8gbX */
            return 0xD;
        default:
            return -1;
    }
}

static tegra_b01_emc_timing_t *get_t210_b01_emc_dvfs_timing_table(int32_t *out_num_entries) {
    switch (get_emc_dvfs_timing_table_index(fuse_get_dram_id())) {
        case 0:    /* SdevEmcDvfsTableS4gb01 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS4gb01;
        case 5:    /* SdevEmcDvfsTableS4gb03 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS4gb03;
        case 6:    /* SdevEmcDvfsTableS8gb03 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS8gb03;
        case 7:    /* SdevEmcDvfsTableH4gb03 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableH4gb03;
        case 8:    /* SdevEmcDvfsTableM4gb03 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableM4gb03;
        case 9:    /* SdevEmcDvfsTableS4gbY01 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS4gbY01;
        case 0xA:  /* SdevEmcDvfsTableS1y4gbY01 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS1y4gbY01;
        case 0xB:  /* SdevEmcDvfsTableS1y8gbY01 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS1y8gbY01;
        case 0xC:  /* SdevEmcDvfsTableS1y4gbX03 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS1y4gbX03;
        case 0xD:  /* SdevEmcDvfsTableS1y8gbX03 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS1y8gbX03;
        case 0xE:  /* SdevEmcDvfsTableS1y4gb01 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableS1y4gb01;
        case 0xF:  /* SdevEmcDvfsTableM1y4gb01 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableM1y4gb01;
        case 0x10: /* SdevEmcDvfsTableH1y4gb01 */
            if (out_num_entries != NULL) { *out_num_entries = 3; }
            return (tegra_b01_emc_timing_t *)T210b01SdevEmcDvfsTableH1y4gb01;
        default:
            return NULL;
    }
}


static void start_periodic_compensation() {
    uint32_t mpc_req = 0x4B;

    /* Write to EMC_MPC_0. */
    emc_write(mpc_req, EMC_MPC);

    /* Dummy read. */
    mpc_req = emc_read(EMC_MPC);
}

static uint32_t actual_osc_clocks(uint32_t in) {
    if (in < 0x40)
        return in * 16;
    else if (in < 0x80)
        return 2048;
    else if (in < 0xc0)
        return 4096;
    else
        return 8192;
}

static uint32_t emc_set_shadow_bypass(int set, uint32_t emc_dbg) {
    if (set)
        emc_dbg |= EMC_DBG_WRITE_MUX_ACTIVE;
    else
        emc_dbg &= ~EMC_DBG_WRITE_MUX_ACTIVE;

    return emc_dbg;
}

static uint32_t wait_for_update(uint32_t status_reg, uint32_t bit_mask, bool updated_state, uint32_t chan_cfg) {
    int result = 0;

    if (chan_cfg & EMC_FBIO_CFG7_CH0_ENABLE) {
        bool success = false;
        for (int i = 0; i < EMC_STATUS_UPDATE_TIMEOUT; ++i) {
            if (((emc0_read(status_reg) & bit_mask) != 0) == updated_state) {
                success = true;
                break;
            }
            udelay(1);
        }
        result |= success ? 0 : 4;
    }

    if (chan_cfg & EMC_FBIO_CFG7_CH1_ENABLE) {
        bool success = false;
        for (int i = 0; i < EMC_STATUS_UPDATE_TIMEOUT; ++i) {
            if (((emc1_read(status_reg) & bit_mask) != 0) == updated_state) {
                success = true;
                break;
            }
            udelay(1);
        }
        result |= success ? 0 : 4;
    }

    return result;
}

static void emc_timing_update(uint32_t fbio_cfg7) {
    /* Trigger the timing update event. */
    emc_write(0x1, EMC_TIMING_CONTROL);

    /* Wait for the update to finish. */
    wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_TIMING_UPDATE_STALLED, false, fbio_cfg7);
}

static uint32_t get_dll_state(tegra_b01_emc_timing_t *next_timing) {
    bool next_dll_enabled = !(next_timing->emc_emrs & 0x1);
    if (next_dll_enabled)
        return DLL_ON;
    else
        return DLL_OFF;
}

static uint32_t div_o3(uint32_t a, uint32_t b) {
    uint32_t result = a / b;

    if ((b * result) < a)
        return result + 1;
    else
        return result;
}

static uint32_t div_o3_f(uint32_t a, uint32_t b) {
    const float res = a / b;

    const uint32_t floor = (uint32_t)res;

    return floor + (((float)floor + 0.01 < res) ? 1 : 0);
}

static uint32_t pll_reprogram(uint32_t rate_khz_to, uint32_t clk_src_emc_to, uint32_t rate_khz_from, uint32_t clk_src_emc_from) {
    volatile tegra_car_t *car = car_get_regs();

    double post_div = 1.0;
    switch ((car_read(&car->clk_source_emc) >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT)) {
        case TEGRA_EMC_SRC_PLLM:
        case TEGRA_EMC_SRC_PLLM_UD:
            post_div = (double)(1 + ((car_read(&car->pllm_base) >> 20) & 1));
            break;
        case TEGRA_EMC_SRC_PLLMB_UD:
        case TEGRA_EMC_SRC_PLLMB:
            post_div = (double)(1 + ((car_read(&car->pllmb_base) >> 20) & 1));
            break;
    }

    const uint32_t emc_2x_clk_src_to   = (clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT);
    const uint32_t emc_2x_clk_src_from = (clk_src_emc_from >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT);
    const uint32_t divisor_to          = (emc_2x_clk_src_to != TEGRA_EMC_SRC_PLLM_UD && emc_2x_clk_src_to != TEGRA_EMC_SRC_PLLMB_UD) ? (clk_src_emc_to & 0xFF) : 0;
    const uint32_t divisor_from        = (emc_2x_clk_src_from != TEGRA_EMC_SRC_PLLM_UD && emc_2x_clk_src_from != TEGRA_EMC_SRC_PLLMB_UD) ? (clk_src_emc_from & 0xFF) : 0;

    if (emc_2x_clk_src_to != emc_2x_clk_src_from && ((emc_2x_clk_src_to | 4) != 4 || (emc_2x_clk_src_from | 4) != 4)) {
        return 1;
    }

    const float val_to   = ((double)(divisor_to & 1) * 0.5 + (double)((divisor_to >> 1) + 1)) * ((double)rate_khz_to) * post_div;
    const float val_from = ((double)(divisor_from & 1) * 0.5 + (double)((divisor_from >> 1) + 1)) * ((double)rate_khz_from) * post_div;
    const float ratio    = val_from / val_to;

    return ratio > 1.01 || ratio < 0.99;
}

static uint32_t program_pllm(uint32_t rate_khz_to, uint32_t rate_khz_from, uint32_t clk_src_emc_to, uint32_t clk_src_emc_from, bool is_pllmb, tegra_b01_emc_timing_t *timing_tables_to) {
    volatile tegra_car_t *car = car_get_regs();

    g_program_pllm_ret = clk_src_emc_from;

    const uint32_t base = ((timing_tables_to->pllmb_divm & 0xFF) | ((timing_tables_to->pllmb_divn & 0xFF) << 8) | ((timing_tables_to->pllmb_divp & 1) << 20));
    if (is_pllmb) {
        car_write(base, &car->pllmb_base);
        car_read(&car->pllmb_base);

        car_write(car_read(&car->pllmb_misc1) | 0x10000000, &car->pllmb_misc1);

        if (timing_tables_to->pll_en_ssc & 1) {
            car_write(timing_tables_to->pllmb_ss_cfg,   &car->pllmb_ss_cfg);
            car_write(timing_tables_to->pllmb_ss_ctrl1, &car->pllmb_ss_ctrl1);
            car_write(timing_tables_to->pllmb_ss_ctrl2, &car->pllmb_ss_ctrl2);
        } else {
            car_write(timing_tables_to->pllmb_ss_cfg   & 0xBFFFFFFF, &car->pllmb_ss_cfg);
            car_write(timing_tables_to->pllmb_ss_ctrl2 & 0x0000FFFF, &car->pllmb_ss_ctrl2);
        }

        car_write(car_read(&car->pllmb_base) | 0x40000000, &car->pllmb_base);

        switch (g_program_pllm_ret >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) {
            case TEGRA_EMC_SRC_PLLM:
                g_program_pllm_ret = ((g_program_pllm_ret & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLMB << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
                break;
            case TEGRA_EMC_SRC_PLLM_UD:
                g_program_pllm_ret = ((g_program_pllm_ret & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLMB_UD << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
                break;
        }

        while ((car_read(&car->pllmb_base) & 0x8000000) == 0) { /* ... */ }

        return g_program_pllm_ret;
    } else {
        car_write(base, &car->pllm_base);
        car_read(&car->pllm_base);

        car_write(car_read(&car->pllm_misc2) | 0x10, &car->pllm_misc2);

        if (timing_tables_to->pll_en_ssc & 1) {
            car_write(timing_tables_to->pllm_ss_cfg,   &car->pllm_ss_cfg);
            car_write(timing_tables_to->pllm_ss_ctrl1, &car->pllm_ss_ctrl1);
            car_write(timing_tables_to->pllm_ss_ctrl2, &car->pllm_ss_ctrl2);
        } else {
            car_write(timing_tables_to->pllm_ss_cfg   & 0xBFFFFFFF, &car->pllm_ss_cfg);
            car_write(timing_tables_to->pllm_ss_ctrl2 & 0x0000FFFF, &car->pllm_ss_ctrl2);
        }

        car_write(car_read(&car->pllm_base) | 0x40000000, &car->pllm_base);

        switch (g_program_pllm_ret >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) {
            case TEGRA_EMC_SRC_PLLM:
                g_program_pllm_ret = ((g_program_pllm_ret & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLM << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
                break;
            case TEGRA_EMC_SRC_PLLM_UD:
                g_program_pllm_ret = ((g_program_pllm_ret & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLM_UD << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
                break;
        }

        while ((car_read(&car->pllm_base) & 0x8000000) == 0) { /* ... */ }

        return g_program_pllm_ret;
    }
}

static uint32_t apply_periodic_compensation_trimmer(tegra_b01_emc_timing_t *timing_tables, uint32_t trim_reg) {
    /* Initialize variables. */
    uint32_t rate_mhz = timing_tables->rate_khz / 1000;
    uint32_t adj[0x10] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };

    int tree_delta[4] = {0};
    int tree_delta_taps[4] = {0};

    /* Generate the intermediate array. */
    #define SET_TRIM_INTERMEDIATE(_arr_, _emc_, _rank_, _byte_)                                                             \
    ({                                                                                                                      \
        const uint32_t shft = timing_tables->trim_perch_regs.emc##  _emc_ ##_data_brlshft_## _rank_;                        \
        const uint32_t base = ((shft >> (3 * _byte_)) & 7) << 6;                                                            \
        const uint32_t val0 = timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank ## _rank_ ## _byte ## _byte_ ## _0; \
        const uint32_t val1 = timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank ## _rank_ ## _byte ## _byte_ ## _1; \
        const uint32_t val2 = timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank ## _rank_ ## _byte ## _byte_ ## _2; \
        _arr_[9 * (8 * _rank_ + _byte_) + 0] = base + ((val0 >>  0) & 0xFF);                                                \
        _arr_[9 * (8 * _rank_ + _byte_) + 1] = base + ((val0 >>  8) & 0xFF);                                                \
        _arr_[9 * (8 * _rank_ + _byte_) + 2] = base + ((val0 >> 16) & 0xFF);                                                \
        _arr_[9 * (8 * _rank_ + _byte_) + 3] = base + ((val0 >> 24) & 0xFF);                                                \
        _arr_[9 * (8 * _rank_ + _byte_) + 4] = base + ((val1 >>  0) & 0xFF);                                                \
        _arr_[9 * (8 * _rank_ + _byte_) + 5] = base + ((val1 >>  8) & 0xFF);                                                \
        _arr_[9 * (8 * _rank_ + _byte_) + 6] = base + ((val1 >> 16) & 0xFF);                                                \
        _arr_[9 * (8 * _rank_ + _byte_) + 7] = base + ((val1 >> 24) & 0xFF);                                                \
        _arr_[9 * (8 * _rank_ + _byte_) + 8] = base + ((val2 >>  0) & 0xFF);                                                \
    })
    {
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 0, 0);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 0, 1);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 0, 2);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 0, 3);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 0, 4);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 0, 5);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 0, 6);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 0, 7);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 1, 0);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 1, 1);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 1, 2);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 0, 1, 3);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 1, 4);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 1, 5);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 1, 6);
        SET_TRIM_INTERMEDIATE(g_periodic_timmer_compensation_intermediates, 1, 1, 7);
    }
    #undef SET_TRIM_INTERMEDIATE

    switch (trim_reg) {
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2:
        case EMC0_BASE + EMC_DATA_BRLSHFT_0:
        case EMC1_BASE + EMC_DATA_BRLSHFT_0:
            {
                tree_delta[0] = 128 * (timing_tables->current_dram_clktree_c0d0u0 - timing_tables->trained_dram_clktree_c0d0u0);
                tree_delta[1] = 128 * (timing_tables->current_dram_clktree_c0d0u1 - timing_tables->trained_dram_clktree_c0d0u1);
                tree_delta[2] = 128 * (timing_tables->current_dram_clktree_c1d0u0 - timing_tables->trained_dram_clktree_c1d0u0);
                tree_delta[3] = 128 * (timing_tables->current_dram_clktree_c1d0u1 - timing_tables->trained_dram_clktree_c1d0u1);
                tree_delta_taps[0] = (tree_delta[0] * (int)rate_mhz) / 1000000;
                tree_delta_taps[1] = (tree_delta[1] * (int)rate_mhz) / 1000000;
                tree_delta_taps[2] = (tree_delta[2] * (int)rate_mhz) / 1000000;
                tree_delta_taps[3] = (tree_delta[3] * (int)rate_mhz) / 1000000;

                for (int i = 0; i < 4; ++i) {
                    const uint32_t sum = (tree_delta_taps[i] <= timing_tables->tree_margin) ? 0 : tree_delta_taps[i];

                    for (int j = 0; j < 18; ++j) {
                        const uint32_t v = (g_periodic_timmer_compensation_intermediates[18 * i + j] += sum);
                        if (v < (adj[2 * i + (j < 9)] << 6)) {
                            adj[2 * i + (j < 9)] = v >> 6;
                        }
                    }
                    for (int j = 0; j < 18; ++j) {
                        g_periodic_timmer_compensation_intermediates[18 * i + j] -= (adj[2 * i + (j < 9)] << 6);
                    }
                }
            }
            break;
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1:
        case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2:
        case EMC0_BASE + EMC_DATA_BRLSHFT_1:
        case EMC1_BASE + EMC_DATA_BRLSHFT_1:
            {
                tree_delta[0] = 128 * (timing_tables->current_dram_clktree_c0d1u0 - timing_tables->trained_dram_clktree_c0d1u0);
                tree_delta[1] = 128 * (timing_tables->current_dram_clktree_c0d1u1 - timing_tables->trained_dram_clktree_c0d1u1);
                tree_delta[2] = 128 * (timing_tables->current_dram_clktree_c1d1u0 - timing_tables->trained_dram_clktree_c1d1u0);
                tree_delta[3] = 128 * (timing_tables->current_dram_clktree_c1d1u1 - timing_tables->trained_dram_clktree_c1d1u1);
                tree_delta_taps[0] = (tree_delta[0] * (int)rate_mhz) / 1000000;
                tree_delta_taps[1] = (tree_delta[1] * (int)rate_mhz) / 1000000;
                tree_delta_taps[2] = (tree_delta[2] * (int)rate_mhz) / 1000000;
                tree_delta_taps[3] = (tree_delta[3] * (int)rate_mhz) / 1000000;

                for (int i = 0; i < 4; ++i) {
                    const uint32_t sum = (tree_delta_taps[i] <= timing_tables->tree_margin) ? 0 : tree_delta_taps[i];

                    for (int j = 0; j < 18; ++j) {
                        const uint32_t v = (g_periodic_timmer_compensation_intermediates[72 + 18 * i + j] += sum);
                        if (v < (adj[8 + 2 * i + (j < 9)] << 6)) {
                            adj[8 + 2 * i + (j < 9)] = v >> 6;
                        }
                    }
                    for (int j = 0; j < 18; ++j) {
                        g_periodic_timmer_compensation_intermediates[72 + 18 * i + j] -= (adj[8 + 2 * i + (j < 9)] << 6);
                    }
                }
            }
            break;
    }

    uint32_t result = 0;
    switch (trim_reg) {
        case EMC0_BASE + EMC_DATA_BRLSHFT_0:
            result = ((adj[ 0] & 7) <<  0) | ((adj[ 1] & 7) <<  3) | ((adj[ 2] & 7) <<  6) | ((adj[ 3] & 7) <<  9);
            break;
        case EMC1_BASE + EMC_DATA_BRLSHFT_0:
            result = ((adj[ 4] & 7) << 12) | ((adj[ 5] & 7) << 15) | ((adj[ 6] & 7) << 18) | ((adj[ 7] & 7) << 21);
            break;
        case EMC0_BASE + EMC_DATA_BRLSHFT_1:
            result = ((adj[ 8] & 7) <<  0) | ((adj[ 9] & 7) <<  3) | ((adj[10] & 7) <<  6) | ((adj[11] & 7) <<  9);
            break;
        case EMC1_BASE + EMC_DATA_BRLSHFT_1:
            result = ((adj[12] & 7) << 12) | ((adj[13] & 7) << 15) | ((adj[14] & 7) << 18) | ((adj[15] & 7) << 21);
            break;
        #define ADD_TRIM_CASE(_ARR_, _RANK_, _BYTE_)                                                                                                                                                                                                  \
            case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK ## _RANK_ ## _BYTE ## _BYTE_ ##_0:                                                                                                                                                       \
                result = ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 0] & 0xFF) << 0) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 1] & 0xFF) << 8) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 2] & 0xFF) << 16) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 3] & 0xFF) << 24); \
                break;                                                                                                                                                                                                                                \
            case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK ## _RANK_ ## _BYTE ## _BYTE_ ##_1:                                                                                                                                                       \
                result = ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 4] & 0xFF) << 0) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 5] & 0xFF) << 8) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 6] & 0xFF) << 16) | ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 7] & 0xFF) << 24); \
                break;                                                                                                                                                                                                                                \
            case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK ## _RANK_ ## _BYTE ## _BYTE_ ##_2:                                                                                                                                                       \
                result = ((_ARR_[9 * (8 * _RANK_ + _BYTE_) + 8] & 0xFF) << 0);                                                                                                                                                                        \
                break;
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 0);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 1);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 2);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 3);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 4);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 5);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 6);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 0, 7);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 0);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 1);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 2);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 3);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 4);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 5);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 6);
        ADD_TRIM_CASE(g_periodic_timmer_compensation_intermediates, 1, 7);
        #undef ADD_TRIM_CASE
    }

    return result;
}

static uint32_t update_clock_tree_delay(tegra_b01_emc_timing_t *src_timing_tables, tegra_b01_emc_timing_t *dst_timing_tables, uint32_t dram_dev_num, uint32_t mode, uint32_t type) {
    uint32_t mrr_req = 0, mrr_data = 0;
    uint32_t temp0_0 = 0, temp0_1 = 0, temp1_0 = 0, temp1_1 = 0;
    int tdel = 0, tmdel = 0, adel = 0;

    uint32_t current_timing_rate_mhz = src_timing_tables->rate_khz / 1000;
    uint32_t next_timing_rate_mhz    = dst_timing_tables->rate_khz / 1000;
    uint32_t fbio_cfg7               = dst_timing_tables->emc_fbio_cfg7;

    bool dvfs_pt1                 = (type == DVFS_PT1);
    bool training_pt1             = (type == TRAINING_PT1);
    bool dvfs_update              = (type == DVFS_UPDATE);
    bool training_update          = (type == TRAINING_UPDATE);
    bool periodic_training_update = (type == PERIODIC_TRAINING_UPDATE);

    /* Dev0 MSB. */
    if (dvfs_pt1 || training_pt1 || periodic_training_update) {
        mrr_req = ((2 << EMC_MRR_DEV_SEL_SHIFT) | (19 << EMC_MRR_MA_SHIFT));
        emc_write(mrr_req, EMC_MRR);

        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, fbio_cfg7);

        if (fbio_cfg7 & EMC_FBIO_CFG7_CH0_ENABLE) {
            mrr_data = ((emc0_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

            temp0_0 = ((mrr_data & 0xff) << 8);
            temp0_1 = (mrr_data & 0xff00);
        } else {
            temp0_0 = temp0_1 = 0;
        }
        if (fbio_cfg7 & EMC_FBIO_CFG7_CH1_ENABLE) {
            mrr_data = ((emc1_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

            temp1_0 = ((mrr_data & 0xff) << 8);
            temp1_1 = (mrr_data & 0xff00);
        } else {
            temp1_0 = temp1_1 = 0;
        }

        /* Dev0 LSB. */
        mrr_req = ((mrr_req & ~EMC_MRR_MA_MASK) | (18 << EMC_MRR_MA_SHIFT));
        emc_write(mrr_req, EMC_MRR);

        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, fbio_cfg7);

        if (fbio_cfg7 & EMC_FBIO_CFG7_CH0_ENABLE) {
            mrr_data = ((emc0_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

            temp0_0 |= (mrr_data & 0xff);
            temp0_1 |= (mrr_data & 0xff00) >> 8;
        }
        if (fbio_cfg7 & EMC_FBIO_CFG7_CH1_ENABLE) {
            mrr_data = ((emc1_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

            temp1_0 |= (mrr_data & 0xff);
            temp1_1 |= (mrr_data & 0xff00) >> 8;
        }
    }

    #define CVAL(v) ((uint32_t)((1000 * ((1000 * actual_osc_clocks(src_timing_tables->run_clocks)) / current_timing_rate_mhz)) / (2 * v)))

    if (fbio_cfg7 & EMC_FBIO_CFG7_CH0_ENABLE) {
        if (dvfs_pt1 || training_pt1)
            __INCREMENT_PTFV(c0d0u0, CVAL(temp0_0));
        else if (dvfs_update)
            __AVERAGE_PTFV(c0d0u0);
        else if (training_update)
            __AVERAGE_WRITE_PTFV(c0d0u0);
        else if (periodic_training_update)
            __WEIGHTED_UPDATE_PTFV(c0d0u0, CVAL(temp0_0));

        if (dvfs_update || training_update || periodic_training_update) {
            tdel = (dst_timing_tables->current_dram_clktree_c0d0u0 - __MOVAVG_AC(dst_timing_tables, c0d0u0));
            tmdel = (tdel < 0) ? ~tdel : tdel;
            adel = tmdel;
            if (mode == 1 || ((adel * 128 * next_timing_rate_mhz) / 1000000) > dst_timing_tables->tree_margin)
                dst_timing_tables->current_dram_clktree_c0d0u0 = __MOVAVG_AC(dst_timing_tables, c0d0u0);
        }

        if (dvfs_pt1 || training_pt1)
            __INCREMENT_PTFV(c0d0u1, CVAL(temp0_1));
        else if (dvfs_update)
            __AVERAGE_PTFV(c0d0u1);
        else if (training_update)
            __AVERAGE_WRITE_PTFV(c0d0u1);
        else if (periodic_training_update)
            __WEIGHTED_UPDATE_PTFV(c0d0u1, CVAL(temp0_1));

        if (dvfs_update || training_update || periodic_training_update) {
            tdel = (dst_timing_tables->current_dram_clktree_c0d0u1 - __MOVAVG_AC(dst_timing_tables, c0d0u1));
            tmdel = (tdel < 0) ? -1 * tdel : tdel;

            if (tmdel > adel)
                adel = tmdel;

            if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing_tables->tree_margin)
                dst_timing_tables->current_dram_clktree_c0d0u1 = __MOVAVG_AC(dst_timing_tables, c0d0u1);
        }
    } else {
        adel = 0;
    }

    if (fbio_cfg7 & EMC_FBIO_CFG7_CH1_ENABLE) {
        if (dvfs_pt1 || training_pt1)
            __INCREMENT_PTFV(c1d0u0, CVAL(temp1_0));
        else if (dvfs_update)
            __AVERAGE_PTFV(c1d0u0);
        else if (training_update)
            __AVERAGE_WRITE_PTFV(c1d0u0);
        else if (periodic_training_update)
            __WEIGHTED_UPDATE_PTFV(c1d0u0, CVAL(temp1_0));

        if (dvfs_update || training_update || periodic_training_update) {
            tdel = (dst_timing_tables->current_dram_clktree_c1d0u0 - __MOVAVG_AC(dst_timing_tables, c1d0u0));
            tmdel = (tdel < 0) ? -1 * tdel : tdel;

            if (tmdel > adel)
                adel = tmdel;

            if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing_tables->tree_margin)
                dst_timing_tables->current_dram_clktree_c1d0u0 = __MOVAVG_AC(dst_timing_tables, c1d0u0);
        }

        if (dvfs_pt1 || training_pt1)
            __INCREMENT_PTFV(c1d0u1, CVAL(temp1_1));
        else if (dvfs_update)
            __AVERAGE_PTFV(c1d0u1);
        else if (training_update)
            __AVERAGE_WRITE_PTFV(c1d0u1);
        else if (periodic_training_update)
            __WEIGHTED_UPDATE_PTFV(c1d0u1, CVAL(temp1_1));

        if (dvfs_update || training_update || periodic_training_update) {
            tdel = (dst_timing_tables->current_dram_clktree_c1d0u1 - __MOVAVG_AC(dst_timing_tables, c1d0u1));
            tmdel = (tdel < 0) ? -1 * tdel : tdel;

            if (tmdel > adel)
                adel = tmdel;

            if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing_tables->tree_margin)
                dst_timing_tables->current_dram_clktree_c1d0u1 = __MOVAVG_AC(dst_timing_tables, c1d0u1);
        }
    }

    if (dram_dev_num == TWO_RANK) {
        /* Dev1 MSB. */
        if (dvfs_pt1 || training_pt1 || periodic_training_update) {
            mrr_req = ((1 << EMC_MRR_DEV_SEL_SHIFT) | (19 << EMC_MRR_MA_SHIFT));
            emc_write(mrr_req, EMC_MRR);

            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, fbio_cfg7);

            if (fbio_cfg7 & EMC_FBIO_CFG7_CH0_ENABLE) {
                mrr_data = ((emc0_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

                temp0_0 = ((mrr_data & 0xff) << 8);
                temp0_1 = (mrr_data & 0xff00);
            }
            if (fbio_cfg7 & EMC_FBIO_CFG7_CH1_ENABLE) {
                mrr_data = ((emc1_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

                temp1_0 = ((mrr_data & 0xff) << 8);
                temp1_1 = (mrr_data & 0xff00);
            }

            /* Dev1 LSB. */
            mrr_req = ((mrr_req & ~EMC_MRR_MA_MASK) | (18 << EMC_MRR_MA_SHIFT));
            emc_write(mrr_req, EMC_MRR);

            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, fbio_cfg7);

            if (fbio_cfg7 & EMC_FBIO_CFG7_CH0_ENABLE) {
                mrr_data = ((emc0_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

                temp0_0 |= ((mrr_data & 0xff) << 8);
                temp0_1 |= (mrr_data & 0xff00);
            }
            if (fbio_cfg7 & EMC_FBIO_CFG7_CH1_ENABLE) {
                mrr_data = ((emc1_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

                temp1_0 |= ((mrr_data & 0xff) << 8);
                temp1_1 |= (mrr_data & 0xff00);
            }
        }

        if (fbio_cfg7 & EMC_FBIO_CFG7_CH0_ENABLE) {
            if (dvfs_pt1 || training_pt1)
                __INCREMENT_PTFV(c0d1u0, CVAL(temp0_0));
            else if (dvfs_update)
                __AVERAGE_PTFV(c0d1u0);
            else if (training_update)
                __AVERAGE_WRITE_PTFV(c0d1u0);
            else if (periodic_training_update)
                __WEIGHTED_UPDATE_PTFV(c0d1u0, CVAL(temp0_0));

            if (dvfs_update || training_update || periodic_training_update) {
                tdel = (dst_timing_tables->current_dram_clktree_c0d1u0 - __MOVAVG_AC(dst_timing_tables, c0d1u0));
                tmdel = (tdel < 0) ? -1 * tdel : tdel;

                if (tmdel > adel)
                    adel = tmdel;

                if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing_tables->tree_margin)
                    dst_timing_tables->current_dram_clktree_c0d1u0 = __MOVAVG_AC(dst_timing_tables, c0d1u0);
            }

            if (dvfs_pt1 || training_pt1)
                __INCREMENT_PTFV(c0d1u1, CVAL(temp0_1));
            else if (dvfs_update)
                __AVERAGE_PTFV(c0d1u1);
            else if (training_update)
                __AVERAGE_WRITE_PTFV(c0d1u1);
            else if (periodic_training_update)
                __WEIGHTED_UPDATE_PTFV(c0d1u1, CVAL(temp0_1));

            if (dvfs_update || training_update || periodic_training_update) {
                tdel = (dst_timing_tables->current_dram_clktree_c0d1u1 - __MOVAVG_AC(dst_timing_tables, c0d1u1));
                tmdel = (tdel < 0) ? -1 * tdel : tdel;

                if (tmdel > adel)
                    adel = tmdel;

                if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing_tables->tree_margin)
                    dst_timing_tables->current_dram_clktree_c0d1u1 = __MOVAVG_AC(dst_timing_tables, c0d1u1);
            }
        }

        if (fbio_cfg7 & EMC_FBIO_CFG7_CH1_ENABLE) {
            if (dvfs_pt1 || training_pt1)
                __INCREMENT_PTFV(c1d1u0, CVAL(temp1_0));
            else if (dvfs_update)
                __AVERAGE_PTFV(c1d1u0);
            else if (training_update)
                __AVERAGE_WRITE_PTFV(c1d1u0);
            else if (periodic_training_update)
                __WEIGHTED_UPDATE_PTFV(c1d1u0, CVAL(temp1_0));

            if (dvfs_update || training_update || periodic_training_update) {
                tdel = (dst_timing_tables->current_dram_clktree_c1d1u0 - __MOVAVG_AC(dst_timing_tables, c1d1u0));
                tmdel = (tdel < 0) ? -1 * tdel : tdel;

                if (tmdel > adel)
                    adel = tmdel;

                if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing_tables->tree_margin)
                    dst_timing_tables->current_dram_clktree_c1d1u0 = __MOVAVG_AC(dst_timing_tables, c1d1u0);
            }

            if (dvfs_pt1 || training_pt1)
                __INCREMENT_PTFV(c1d1u1, CVAL(temp1_1));
            else if (dvfs_update)
                __AVERAGE_PTFV(c1d1u1);
            else if (training_update)
                __AVERAGE_WRITE_PTFV(c1d1u1);
            else if (periodic_training_update)
                __WEIGHTED_UPDATE_PTFV(c1d1u1, CVAL(temp1_1));

            if (dvfs_update || training_update || periodic_training_update) {
                tdel = (dst_timing_tables->current_dram_clktree_c1d1u1 - __MOVAVG_AC(dst_timing_tables, c1d1u1));
                tmdel = (tdel < 0) ? -1 * tdel : tdel;

                if (tmdel > adel)
                    adel = tmdel;

                if (mode == 1 || (tmdel * 128 * next_timing_rate_mhz / 1000000) > dst_timing_tables->tree_margin)
                    dst_timing_tables->current_dram_clktree_c1d1u1 = __MOVAVG_AC(dst_timing_tables, c1d1u1);
            }
        }
    }

    #undef CVAL

    if (mode == 1) {
        dst_timing_tables->trained_dram_clktree_c0d0u0 = dst_timing_tables->current_dram_clktree_c0d0u0;
        dst_timing_tables->trained_dram_clktree_c0d0u1 = dst_timing_tables->current_dram_clktree_c0d0u1;
        dst_timing_tables->trained_dram_clktree_c0d1u0 = dst_timing_tables->current_dram_clktree_c0d1u0;
        dst_timing_tables->trained_dram_clktree_c0d1u1 = dst_timing_tables->current_dram_clktree_c0d1u1;
        dst_timing_tables->trained_dram_clktree_c1d0u0 = dst_timing_tables->current_dram_clktree_c1d0u0;
        dst_timing_tables->trained_dram_clktree_c1d0u1 = dst_timing_tables->current_dram_clktree_c1d0u1;
        dst_timing_tables->trained_dram_clktree_c1d1u0 = dst_timing_tables->current_dram_clktree_c1d1u0;
        dst_timing_tables->trained_dram_clktree_c1d1u1 = dst_timing_tables->current_dram_clktree_c1d1u1;
    }

    return adel;
}

static uint32_t periodic_compensation_handler(int type, uint32_t dram_dev_num, tegra_b01_emc_timing_t *src_timing_tables, tegra_b01_emc_timing_t *dst_timing_tables) {
    if (!dst_timing_tables->periodic_training) {
        return 0;
    }

    uint32_t adel          = 0;
    uint32_t samples       = dst_timing_tables->ptfv_list.ptfv_dvfs_samples;
    uint32_t samples_write = dst_timing_tables->ptfv_list.ptfv_write_samples;
    uint32_t delay         = 2 + (1000 * actual_osc_clocks(src_timing_tables->run_clocks) / src_timing_tables->rate_khz);

    if (type == DVFS_SEQUENCE) {
        if (src_timing_tables->periodic_training && (dst_timing_tables->ptfv_list.ptfv_config_ctrl & 1)) {
            /* If the previous frequency was using periodic calibration then we can reuse the previous frequencies EMA data. */
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u0 = src_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u0 * samples;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u1 = src_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u1 * samples;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u0 = src_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u0 * samples;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u1 = src_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u1 * samples;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u0 = src_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u0 * samples;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u1 = src_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u1 * samples;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u0 = src_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u0 * samples;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u1 = src_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u1 * samples;
        } else {
            /* Reset the EMA. */
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u0 = 0;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u1 = 0;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u0 = 0;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u1 = 0;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u0 = 0;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u1 = 0;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u0 = 0;
            dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u1 = 0;

            for (uint32_t i = 0; i < samples; ++i) {
                start_periodic_compensation();

                udelay(delay);

                /* Generate next sample of data. */
                adel = update_clock_tree_delay(src_timing_tables, dst_timing_tables, dram_dev_num, 0, DVFS_PT1);
            }
        }

        adel = update_clock_tree_delay(src_timing_tables, dst_timing_tables, dram_dev_num, 0, DVFS_UPDATE);
    } else if (type == WRITE_TRAINING_SEQUENCE) {
        /* Reset the EMA. */
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u0 = 0;
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u1 = 0;
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u0 = 0;
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u1 = 0;
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u0 = 0;
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u1 = 0;
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u0 = 0;
        dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u1 = 0;

        for (uint32_t i = 0; i < samples_write; ++i) {
            start_periodic_compensation();

            udelay(delay);

            /* Generate next sample of data. */
            adel = update_clock_tree_delay(src_timing_tables, dst_timing_tables, dram_dev_num, 1, TRAINING_PT1);
        }

        adel = update_clock_tree_delay(src_timing_tables, dst_timing_tables, dram_dev_num, 1, TRAINING_UPDATE);
    } else if (type == PERIODIC_TRAINING_SEQUENCE) {
        start_periodic_compensation();

        udelay(delay);

        adel = update_clock_tree_delay(src_timing_tables, dst_timing_tables, dram_dev_num, 0, PERIODIC_TRAINING_UPDATE);
    }

    return adel;
}

static void change_dll_src(tegra_b01_emc_timing_t *timing_tables, uint32_t dst_clk_src) {
    volatile tegra_car_t *car = car_get_regs();

    uint32_t val = ((dst_clk_src & 0xE00000FF) | (timing_tables->dll_clk_src & 0x1FFFFF00)) & 0xFFFFF3FF;
    switch (dst_clk_src >> 29) {
        case TEGRA_EMC_SRC_PLLMB_UD:
            val |= 0x400;
            break;
        case TEGRA_EMC_SRC_PLLM_UD:
            break;
        default:
            val |= 0x800;
            break;
    }

    /* Set EMC_DLL_CLK_SRC, DDLL_CLK_SEL and EMC_DLL_CLK_DIVISOR */
    car_write(val, &car->clk_source_emc_dll);

    /* Update CLK_ENB_EMC_DLL */
    car_write((car_read(&car->clk_out_enb_x) & 0xFFFFBFFF) | ((timing_tables->clk_out_enb_x_0_clk_enb_emc_dll & 1) << 14), &car->clk_out_enb_x);
    car_read(&car->clk_out_enb_x) ;

}

static void dll_prelock(tegra_b01_emc_timing_t *dst_timing_tables, tegra_b01_emc_timing_t *src_timing_tables, bool training_enabled, uint32_t dst_clk_src) {
    /* Update EMC_CFG_DIG_DLL */
    emc_write((emc_read(EMC_CFG_DIG_DLL) & 0xFFFFFFE4) | 0x00000008, EMC_CFG_DIG_DLL);

    /* Request a timing update event */
    emc_timing_update(dst_timing_tables->emc_fbio_cfg7);

    /* Update EMC_CFG_DIG_DLL */
    emc_write((emc_read(EMC_CFG_DIG_DLL) & 0xFFFFF824) | 0x000003C8, EMC_CFG_DIG_DLL);

    /* Request a timing update event */
    emc_timing_update(dst_timing_tables->emc_fbio_cfg7);

    /* Wait until CFG_DLL_EN is cleared. */
    wait_for_update(EMC_CFG_DIG_DLL, EMC_CFG_DIG_DLL_CFG_DLL_EN, false, dst_timing_tables->emc_fbio_cfg7);

    /* Configure PMACRO_DLL_CFG */
    emc_write(dst_timing_tables->burst_regs.emc_pmacro_dll_cfg_0, EMC_PMACRO_DLL_CFG_0);
    emc_read(EMC_PMACRO_DLL_CFG_1);

    emc_write((dst_timing_tables->burst_regs.emc_pmacro_dll_cfg_1 & 0xFFFFDFFF) | (emc_read(EMC_PMACRO_DLL_CFG_1) & 0x00002000), EMC_PMACRO_DLL_CFG_1);

    /* Request a timing update event */
    emc_timing_update(dst_timing_tables->emc_fbio_cfg7);

    /* Change the dll clock source. */
    change_dll_src(dst_timing_tables, dst_clk_src);

    /* Wait 2 us. */
    udelay(2);

    /* Enable dll. */
    emc_write(emc_read(EMC_CFG_DIG_DLL) | 0x00000001, EMC_CFG_DIG_DLL);

    /* Request a timing update event */
    emc_timing_update(dst_timing_tables->emc_fbio_cfg7);

    /* Wait until CFG_DLL_EN is set. */
    wait_for_update(EMC_CFG_DIG_DLL, EMC_CFG_DIG_DLL_CFG_DLL_EN, true, dst_timing_tables->emc_fbio_cfg7);

    /* Wait for DLL_LOCK to be set */
    wait_for_update(EMC_DIG_DLL_STATUS, EMC_DIG_DLL_STATUS_DLL_LOCK_B01, true, dst_timing_tables->emc_fbio_cfg7);

    if (training_enabled) {
        /* Disable dll. */
        emc_write(emc_read(EMC_DBG) | 0x00000002, EMC_DBG);
        emc_write(emc_read(EMC_CFG_DIG_DLL) & 0xFFFFFFFE, EMC_CFG_DIG_DLL);
        emc_write(emc_read(EMC_DBG) & 0xFFFFFFFD, EMC_DBG);

        /* Wait until CFG_DLL_EN is cleared. */
        wait_for_update(EMC_CFG_DIG_DLL, EMC_CFG_DIG_DLL_CFG_DLL_EN, false, dst_timing_tables->emc_fbio_cfg7);
    }

    emc_read(0xD20);
}

static void dll_disable(uint32_t fbio_cfg7) {
    /* Disable dll. */
    emc_write(emc_read(EMC_CFG_DIG_DLL) & 0xFFFFFFFE, EMC_CFG_DIG_DLL);

    /* Request a timing update event */
    emc_timing_update(fbio_cfg7);

    /* Wait until CFG_DLL_EN is cleared. */
    wait_for_update(EMC_CFG_DIG_DLL, EMC_CFG_DIG_DLL_CFG_DLL_EN, false, fbio_cfg7);
}

static void pll_disable(uint32_t dst_clk_src) {
    volatile tegra_car_t *car = car_get_regs();

    switch (dst_clk_src >> 29) {
        case TEGRA_EMC_SRC_PLLM:
        case TEGRA_EMC_SRC_PLLM_UD:
            car_write(car_read(&car->pllmb_base) & 0xBFFFFFFF, &car->pllmb_base);
            break;
        case TEGRA_EMC_SRC_PLLMB_UD:
        case TEGRA_EMC_SRC_PLLMB:
            car_write(car_read(&car->pllm_base) & 0xBFFFFFFF, &car->pllm_base);
            break;
        case TEGRA_EMC_SRC_PLLC:
        case TEGRA_EMC_SRC_PLLP:
        case TEGRA_EMC_SRC_CLKM:
        case TEGRA_EMC_SRC_PLLP_UD:
            car_write(car_read(&car->pllm_base) & 0xBFFFFFFF, &car->pllm_base);
            car_write(car_read(&car->pllmb_base) & 0xBFFFFFFF, &car->pllmb_base);
            break;
    }
}

static void dvfs_power_ramp_down(tegra_b01_emc_timing_t *src_timing_tables, tegra_b01_emc_timing_t *dst_timing_tables, bool flip_backward, uint32_t vtt_vdda_channel) {
    tegra_b01_emc_timing_t *from_table = (flip_backward ? dst_timing_tables : src_timing_tables);
    tegra_b01_emc_timing_t *to_table   = (flip_backward ? src_timing_tables : dst_timing_tables);

    uint32_t from_rate_khz = from_table->rate_khz;
    uint32_t from_period   = 1000000000 / from_rate_khz;

    uint32_t to_rate_khz   = to_table->rate_khz;
    uint32_t clk_div       = 1000 * dst_timing_tables->src_clock_div;

    uint32_t delay = div_o3(clk_div, from_period);

    if (from_rate_khz >= 407997 || to_rate_khz <= 407996) {
        if (from_rate_khz >= 407997 && to_rate_khz <= 407996) {
            uint32_t pmacro_vttgen_ctrl_1 = emc_read(EMC_PMACRO_VTTGEN_CTRL_1);

            if (dst_timing_tables->vtt_vdda_dual_channel) {
                if (vtt_vdda_channel != 1) {
                    return;
                }

                ccfifo_write(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((to_table->vtt_vdda_ctrl_4 & 0x3F) << 10), delay);
                ccfifo_write(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((to_table->vtt_vdda_ctrl_0 & 0x3F) << 10), delay * 2);
            } else {
                ccfifo_write(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing_tables->vtt_vdda_ctrl_0 & 0x3F) << 10), 0);
            }
        }
    } else {
        uint32_t pmacro_vttgen_ctrl_1 = emc_read(EMC_PMACRO_VTTGEN_CTRL_1);

        if (dst_timing_tables->vtt_vdda_dual_channel) {
            if (vtt_vdda_channel == 1) {
                ccfifo_write(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing_tables->vtt_vdda_ctrl_3 & 0x3F) << 10), delay);
                ccfifo_write(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing_tables->vtt_vdda_ctrl_0 & 0x3F) << 10), delay);
            } else if (vtt_vdda_channel == 0) {
                ccfifo_write(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing_tables->vtt_vdda_ctrl_1 & 0x3F) << 10), delay);
                ccfifo_write(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing_tables->vtt_vdda_ctrl_2 & 0x3F) << 10), delay);
            }
        } else {
            ccfifo_write(EMC_PMACRO_VTTGEN_CTRL_1, (pmacro_vttgen_ctrl_1 & 0xFFFF03FF) | ((dst_timing_tables->vtt_vdda_ctrl_0 & 0x3F) << 10), 0);
        }
    }
}

static uint32_t dvfs_power_ramp_up(uint32_t dst_clock_period, bool flip_backward, tegra_b01_emc_timing_t *src_timing_tables, tegra_b01_emc_timing_t *dst_timing_tables, uint32_t needs_training) {
    uint32_t misc_cfg_1 = flip_backward ? src_timing_tables->misc_cfg_1 : dst_timing_tables->misc_cfg_1;

    uint32_t emc_pmacro_cmd_pad_tx_ctrl, emc_pmacro_brick_ctrl_rfu1, emc_fbio_cfg5;
    if (flip_backward) {
        emc_pmacro_cmd_pad_tx_ctrl = src_timing_tables->burst_regs.emc_pmacro_cmd_pad_tx_ctrl;
        emc_pmacro_brick_ctrl_rfu1 = src_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1;
        emc_fbio_cfg5              = src_timing_tables->burst_regs.emc_fbio_cfg5;
    } else if (needs_training & 3) {
        emc_pmacro_cmd_pad_tx_ctrl = dst_timing_tables->shadow_regs_ca_train.emc_pmacro_cmd_pad_tx_ctrl;
        emc_pmacro_brick_ctrl_rfu1 = dst_timing_tables->shadow_regs_ca_train.emc_pmacro_brick_ctrl_rfu1;
        emc_fbio_cfg5              = dst_timing_tables->shadow_regs_ca_train.emc_fbio_cfg5;
    } else if (needs_training & 0xF0) {
        emc_pmacro_cmd_pad_tx_ctrl = dst_timing_tables->shadow_regs_rdwr_train.emc_pmacro_cmd_pad_tx_ctrl;
        emc_pmacro_brick_ctrl_rfu1 = dst_timing_tables->shadow_regs_rdwr_train.emc_pmacro_brick_ctrl_rfu1;
        emc_fbio_cfg5              = dst_timing_tables->shadow_regs_rdwr_train.emc_fbio_cfg5;
    } else {
        emc_pmacro_cmd_pad_tx_ctrl = dst_timing_tables->burst_regs.emc_pmacro_cmd_pad_tx_ctrl;
        emc_pmacro_brick_ctrl_rfu1 = dst_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1;
        emc_fbio_cfg5              = dst_timing_tables->burst_regs.emc_fbio_cfg5;
    }

    bool misc_flag = (misc_cfg_1 & 3) == 3;
    uint32_t timescale = 100000 << ((misc_cfg_1 >> 2) & 7);
    uint32_t delay     = (timescale / dst_clock_period);

    if (dst_clock_period < 869 || misc_flag) {
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 & 0xFE40FE40, delay + 1);
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 & 0xFEEDFEED, delay + 1);
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 & 0xFFFFFFFF, delay + 1);

        ccfifo_write(EMC_FBIO_CFG5, emc_fbio_cfg5 & 0xFFFFFEFF, delay + 10);

        ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, emc_pmacro_cmd_pad_tx_ctrl & 0xFBFFFFFF, 5);

        return timescale + 10 * dst_clock_period + 3 * timescale;
    } else if (dst_clock_period > 1665) {
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 | 0x00000600, 0);

        ccfifo_write(EMC_FBIO_CFG5, emc_fbio_cfg5 & 0xFFFFFEFF, 12);

        ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, emc_pmacro_cmd_pad_tx_ctrl & 0xFBFFFFFF, 5);

        return 12 * dst_clock_period;
    } else {
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1 | 0x06000600, delay + 1);

        ccfifo_write(EMC_FBIO_CFG5, emc_fbio_cfg5 & 0xFFFFFEFF, delay + 10);

        ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, emc_pmacro_cmd_pad_tx_ctrl & 0xFBFFFFFF, 5);

        return timescale + 10 * dst_clock_period;
    }
}

static void freq_change(tegra_b01_emc_timing_t *src_timing_tables, tegra_b01_emc_timing_t *dst_timing_tables, uint32_t needs_training, uint32_t dst_clk_src, uint32_t unk) {
    volatile tegra_car_t *car = car_get_regs();

    /* Extract training values */
    bool train_ca           = (needs_training & 0x001);
    bool train_ca_vref      = (needs_training & 0x002);
    // bool train_quse        = (needs_training & 0x004);
    // bool train_quse_vref   = (needs_training & 0x008);
    bool train_wr           = (needs_training & 0x010);
    bool train_wr_vref      = (needs_training & 0x020);
    bool train_rd           = (needs_training & 0x040);
    bool train_rd_vref      = (needs_training & 0x080);
    bool train_swap_rank    = (needs_training & 0x100);
    bool train_self_refresh = (needs_training & 0x200);

    /* Check if we should do training. */
    uint32_t training_enabled_mask = (needs_training & 0xF3);

    uint32_t dst_emc_fbio_cfg7 = dst_timing_tables->emc_fbio_cfg7;
    uint32_t dst_misc_cfg_0    = dst_timing_tables->misc_cfg_0;
    uint32_t dst_misc_cfg_1    = dst_timing_tables->misc_cfg_1;
    uint32_t dst_misc_cfg_2    = dst_timing_tables->misc_cfg_2;
    uint32_t src_misc_cfg_0    = src_timing_tables->misc_cfg_0;
    uint32_t src_misc_cfg_1    = src_timing_tables->misc_cfg_1;
    uint32_t src_t_rp          = src_timing_tables->dram_timings.t_rp;
    uint32_t src_t_rfc         = src_timing_tables->dram_timings.t_rfc;
    uint32_t dst_t_pdex        = dst_timing_tables->dram_timings.t_pdex;
    uint32_t dst_t_fc_lpddr4   = dst_timing_tables->dram_timings.t_fc_lpddr4;

    g_fsp_for_next_freq = !g_fsp_for_next_freq;

    int dram_type                  = emc_read(EMC_FBIO_CFG5) & EMC_FBIO_CFG5_DRAM_TYPE_MASK >> EMC_FBIO_CFG5_DRAM_TYPE_SHIFT;
    uint32_t src_emc_zcal_wait_cnt = src_timing_tables->burst_regs.emc_zcal_wait_cnt;

    bool shared_zq_resistor = (src_emc_zcal_wait_cnt >> 31) & 1;

    bool opt_zcal_en_cc = (dst_timing_tables->burst_regs.emc_zcal_interval && !src_timing_tables->burst_regs.emc_zcal_interval) || (dram_type == DRAM_TYPE_LPDDR4);

    uint32_t dst_t_fc_lpddr4_hz = 1000 * dst_t_fc_lpddr4;

    bool is_lpddr2 = (dram_type == DRAM_TYPE_LPDDR2);
    bool is_lpddr3 = is_lpddr2 && ((dst_timing_tables->burst_regs.emc_fbio_cfg5 >> 25) & 1);
    uint32_t opt_dll_mode = (dram_type == DRAM_TYPE_DDR3) ? get_dll_state(dst_timing_tables) : DLL_OFF;

    int dram_dev_num = ((mc_read(MC_EMEM_ADR_CFG) & 1) + 1);

    uint32_t tZQCAL_lpddr4              = dst_timing_tables->tZQCAL_lpddr4;
    uint32_t zqcal_before_cc_cutoff     = dst_timing_tables->zqcal_before_cc_cutoff;
    uint32_t opt_cc_short_zcal          = dst_timing_tables->opt_cc_short_zcal;
    uint32_t opt_short_zcal             = dst_timing_tables->opt_short_zcal;
    uint32_t opt_do_sw_qrst             = dst_timing_tables->opt_do_sw_qrst;
    uint32_t save_restore_clkstop_pd    = dst_timing_tables->save_restore_clkstop_pd;
    // uint32_t opt_E90                    = dst_timing_tables->opt_E90;
    uint32_t cya_allow_ref_cc           = dst_timing_tables->cya_allow_ref_cc;
    uint32_t ref_b4_sref_en             = dst_timing_tables->ref_b4_sref_en;
    uint32_t cya_issue_pc_ref           = dst_timing_tables->cya_issue_pc_ref;

    uint32_t src_rate_khz = src_timing_tables->rate_khz;
    uint32_t dst_rate_khz = dst_timing_tables->rate_khz;

    uint32_t src_clock_period = 1000000000 / src_rate_khz;
    uint32_t dst_clock_period = 1000000000 / dst_rate_khz;

    uint32_t emc_auto_cal_config = emc_read(EMC_AUTO_CAL_CONFIG);

    uint32_t adj_dst_t_fc_lpddr4 = (dst_clock_period <= zqcal_before_cc_cutoff) ? dst_t_fc_lpddr4_hz : 0;

    uint32_t emc_dbg_o = emc_read(EMC_DBG);
    uint32_t emc_pin_o = emc_read(EMC_PIN);
    uint32_t emc_cfg_pipe_clk_o = emc_read(EMC_CFG_PIPE_CLK);
    uint32_t emc_dbg = emc_dbg_o;

    uint32_t emc_cfg = dst_timing_tables->burst_regs.emc_cfg;
    uint32_t emc_sel_dpd_ctrl = dst_timing_tables->emc_sel_dpd_ctrl;

    uint32_t next_push, next_dq_e_ivref, next_dqs_e_ivref;

    /* Step 1:
     *   Pre DVFS SW sequence.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 1\n");
    //mdelay(500);

    /* Step 1.1: Disable DLL. */
    uint32_t tmp = emc_read(EMC_CFG_DIG_DLL);
    tmp &= ~EMC_CFG_DIG_DLL_CFG_DLL_EN;
    emc_write(tmp, EMC_CFG_DIG_DLL);

    /* Calculate 14000 / dst_period. */
    uint32_t div_14000_by_dst_period = max(div_o3(14000, dst_clock_period), (uint32_t)10);

    /* Request a timing update. */
    emc_timing_update(dst_emc_fbio_cfg7);

    /* Wait for DLL to be disabled. */
    wait_for_update(EMC_CFG_DIG_DLL, EMC_CFG_DIG_DLL_CFG_DLL_EN, false, dst_emc_fbio_cfg7);

    /* Step 1.2: Disable AUTOCAL. */
    emc_auto_cal_config = (dst_timing_tables->emc_auto_cal_config & 0x7FFFF9FF) | 0x600;
    emc_write(emc_auto_cal_config, EMC_AUTO_CAL_CONFIG);
    emc_read(EMC_AUTO_CAL_CONFIG);

    /* Step 1.3: Disable other power features. */
    emc_dbg = emc_set_shadow_bypass(ACTIVE, emc_dbg_o);
    emc_write(emc_dbg, EMC_DBG);
    emc_write(emc_cfg & ~(EMC_CFG_DYN_SELF_REF | EMC_CFG_DRAM_ACPD | EMC_CFG_DRAM_CLKSTOP_SR | EMC_CFG_DRAM_CLKSTOP_PD), EMC_CFG);
    emc_write(emc_sel_dpd_ctrl & ~(EMC_SEL_DPD_CTRL_CLK_SEL_DPD_EN | EMC_SEL_DPD_CTRL_CA_SEL_DPD_EN | EMC_SEL_DPD_CTRL_RESET_SEL_DPD_EN | EMC_SEL_DPD_CTRL_ODT_SEL_DPD_EN | EMC_SEL_DPD_CTRL_DATA_SEL_DPD_EN), EMC_SEL_DPD_CTRL);
    emc_write(emc_dbg_o, EMC_DBG);

    /* Skip this if dvfs_with_training is set. */
    bool compensate_trimmer_applicable = false;
    uint32_t adel = 0;
    if (!training_enabled_mask && dst_timing_tables->periodic_training) {
        /* Wait for DRAM to get out of power down. */
        wait_for_update(EMC_EMC_STATUS, dram_dev_num == TWO_RANK ? 0x30 : 0x10, false, dst_emc_fbio_cfg7);

        /* Wait for DRAM to get out of self refresh. */
        wait_for_update(EMC_EMC_STATUS, 0x300, false, dst_emc_fbio_cfg7);

        if (dst_timing_tables->periodic_training) {
            /* Reset all clock tree values. */
            dst_timing_tables->current_dram_clktree_c0d0u0 = dst_timing_tables->trained_dram_clktree_c0d0u0;
            dst_timing_tables->current_dram_clktree_c0d0u1 = dst_timing_tables->trained_dram_clktree_c0d0u1;
            dst_timing_tables->current_dram_clktree_c0d1u0 = dst_timing_tables->trained_dram_clktree_c0d1u0;
            dst_timing_tables->current_dram_clktree_c0d1u1 = dst_timing_tables->trained_dram_clktree_c0d1u1;
            dst_timing_tables->current_dram_clktree_c1d0u0 = dst_timing_tables->trained_dram_clktree_c1d0u0;
            dst_timing_tables->current_dram_clktree_c1d0u1 = dst_timing_tables->trained_dram_clktree_c1d0u1;
            dst_timing_tables->current_dram_clktree_c1d1u0 = dst_timing_tables->trained_dram_clktree_c1d1u0;
            dst_timing_tables->current_dram_clktree_c1d1u1 = dst_timing_tables->trained_dram_clktree_c1d1u1;

            /* Do DVFS_SEQUENCE. */
            adel = periodic_compensation_handler(DVFS_SEQUENCE, dram_dev_num, src_timing_tables, dst_timing_tables);

            /* Check if we should use compensate trimmer. */
            compensate_trimmer_applicable = dst_timing_tables->periodic_training && ((adel * 128 * (dst_rate_khz / 1000)) / 1000000) > dst_timing_tables->tree_margin;
        }
    }

    emc_write(EMC_INTSTATUS_CLKCHANGE_COMPLETE, EMC_INTSTATUS);

    emc_dbg = emc_set_shadow_bypass(ACTIVE, emc_dbg);
    emc_write(emc_dbg, EMC_DBG);
    emc_write(emc_cfg & ~(EMC_CFG_DYN_SELF_REF | EMC_CFG_DRAM_ACPD | EMC_CFG_DRAM_CLKSTOP_SR | EMC_CFG_DRAM_CLKSTOP_PD), EMC_CFG);
    emc_write(emc_sel_dpd_ctrl & ~(EMC_SEL_DPD_CTRL_CLK_SEL_DPD_EN | EMC_SEL_DPD_CTRL_CA_SEL_DPD_EN | EMC_SEL_DPD_CTRL_RESET_SEL_DPD_EN | EMC_SEL_DPD_CTRL_ODT_SEL_DPD_EN | EMC_SEL_DPD_CTRL_DATA_SEL_DPD_EN), EMC_SEL_DPD_CTRL);
    emc_write(emc_cfg_pipe_clk_o | EMC_CFG_PIPE_CLK_CLK_ALWAYS_ON, EMC_CFG_PIPE_CLK);
    emc_write(dst_timing_tables->emc_fdpd_ctrl_cmd_no_ramp & ~EMC_FDPD_CTRL_CMD_NO_RAMP_CMD_DPD_NO_RAMP_ENABLE, EMC_FDPD_CTRL_CMD_NO_RAMP);

    /* Adjust pllm_misc1 as needed. */
    if (dst_timing_tables->pllm_misc1_0_pllm_clamp_ph90) {
        car_write(car_read(&car->pllm_misc1) & 0x7FFFFFFF, &car->pllm_misc1);
    }

    /* Check if we need to turn on VREF generator. */
    if ((((!src_timing_tables->burst_regs.emc_pmacro_data_pad_tx_ctrl &
           EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF)) &&
         ((dst_timing_tables->burst_regs.emc_pmacro_data_pad_tx_ctrl &
           EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF))) ||
        ((!(src_timing_tables->burst_regs.emc_pmacro_data_pad_tx_ctrl &
           EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF_B01)) &&
         ((dst_timing_tables->burst_regs.emc_pmacro_data_pad_tx_ctrl &
           EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF_B01))))
    {
        uint32_t pad_tx_ctrl      = dst_timing_tables->burst_regs.emc_pmacro_data_pad_tx_ctrl;
        uint32_t last_pad_tx_ctrl = src_timing_tables->burst_regs.emc_pmacro_data_pad_tx_ctrl;

        next_dqs_e_ivref = pad_tx_ctrl & EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF_B01;
        next_dq_e_ivref = pad_tx_ctrl & EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF;
        next_push = (last_pad_tx_ctrl & ~EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF & ~EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF_B01) | next_dq_e_ivref | next_dqs_e_ivref;
        emc_write(next_push, EMC_PMACRO_DATA_PAD_TX_CTRL);

        emc_write(emc_dbg_o, EMC_DBG);
        udelay(1);
    } else {
        emc_write(emc_dbg_o, EMC_DBG);
    }

    /* Check if we need to fixup xm2comppadctrl */
    if ((dst_misc_cfg_1 & 0x20) == 0) {
        emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

        uint32_t xm2comppadctrl = emc_read(EMC_XM2COMPPADCTRL);

        emc_write(xm2comppadctrl | 0x08000000, EMC_XM2COMPPADCTRL);
        udelay(1);
        emc_write(xm2comppadctrl | 0x18000000, EMC_XM2COMPPADCTRL);
        udelay(1);
        emc_write(xm2comppadctrl | 0x38000000, EMC_XM2COMPPADCTRL);
        udelay(1);

        emc_write(emc_set_shadow_bypass(ASSEMBLY, emc_dbg_o), EMC_DBG);
    }

    /* Step 2:
     *   Prelock the DLL.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 2\n");
    //mdelay(500);

    if (dst_timing_tables->burst_regs.emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN) {
        emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

        emc_write(emc_read(EMC_PMACRO_DLL_CFG_1) & 0xFFFFDFFF, EMC_PMACRO_DLL_CFG_1);
        emc_read(EMC_PMACRO_DLL_CFG_1);

        emc_write(emc_set_shadow_bypass(ASSEMBLY, emc_dbg_o), EMC_DBG);
        emc_read(EMC_DBG);

        dll_prelock(dst_timing_tables, src_timing_tables, training_enabled_mask, dst_clk_src);
    } else {
        dll_disable(dst_emc_fbio_cfg7);
    }

    /* Step 3:
     *   Prepare autocal for the clock change.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 3\n");
    //mdelay(500);

    /* Disable AUTOCAL. */
    emc_auto_cal_config = (dst_timing_tables->emc_auto_cal_config & 0x7FFFF9FF) | 0x600;
    emc_write(emc_auto_cal_config, EMC_AUTO_CAL_CONFIG);

    emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

    emc_write(dst_timing_tables->emc_auto_cal_config2, EMC_AUTO_CAL_CONFIG2);

    if (dst_emc_fbio_cfg7 & (EMC_FBIO_CFG7_CH0_ENABLE | EMC_FBIO_CFG7_CH1_ENABLE)) {
        emc0_write(dst_timing_tables->emc_auto_cal_config3, EMC_AUTO_CAL_CONFIG3);
        emc0_write(dst_timing_tables->emc_auto_cal_config4, EMC_AUTO_CAL_CONFIG4);
        emc0_write(dst_timing_tables->emc_auto_cal_config5, EMC_AUTO_CAL_CONFIG5);
        emc0_write(dst_timing_tables->emc_auto_cal_config6, EMC_AUTO_CAL_CONFIG6);
        emc0_write(dst_timing_tables->emc_auto_cal_config7, EMC_AUTO_CAL_CONFIG7);
        emc0_write(dst_timing_tables->emc_auto_cal_config8, EMC_AUTO_CAL_CONFIG8);
    }

    emc_write(emc_dbg_o, EMC_DBG);

    emc_auto_cal_config = (dst_timing_tables->emc_auto_cal_config & 0x7FFFF9FE) | 0x601;
    emc_write(emc_auto_cal_config, EMC_AUTO_CAL_CONFIG);

    /* Step 4:
     *   Update EMC_CFG.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 4\n");
    //mdelay(500);

    emc_write(emc_read(EMC_CFG) & 0xEFFFFFFF, EMC_CFG);
    emc_write(dst_timing_tables->emc_cfg_2, EMC_CFG_2);

    /* Step 5:
     *   Prepare reference variables for ZQCAL regs.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 5\n");
    //mdelay(500);

    uint32_t emc_zcal_interval = src_timing_tables->burst_regs.emc_zcal_interval & 0xFF000000;
    uint32_t dst_emc_zcal_wait_cnt = dst_timing_tables->burst_regs.emc_zcal_wait_cnt;

    uint32_t zq_wait_long, zq_wait_short;

    if (dram_type == DRAM_TYPE_LPDDR4) {
        zq_wait_long  = max(div_o3(1000000, dst_clock_period), (uint32_t)1);
        zq_wait_short = max(div_o3(30000, dst_clock_period), (uint32_t)8) + 1;
    } else if (is_lpddr2 || is_lpddr3) {
        zq_wait_long  = max(div_o3(360000, dst_clock_period), dst_timing_tables->min_mrs_wait) + 4;
        zq_wait_short = 0;
    } else if (dram_type == DRAM_TYPE_DDR3) {
        zq_wait_long  = max(div_o3(320000, dst_clock_period), (uint32_t)256);
        zq_wait_short = 0;
    } else {
        zq_wait_long  = 0;
        zq_wait_short = 0;
    }

    /* Step 6:
     *   Training code.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 6\n");
    //mdelay(500);

    {
        uint32_t pintemp = emc_read(EMC_PIN);
        if ((train_ca || train_ca_vref) && (dram_dev_num == TWO_RANK)) {
            emc_write(pintemp | 0x7, EMC_PIN);
        }
    }

    /* Step 7:
     *   Program FSP reference registers and send MRWs to new FSPWR.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 7\n");
    //mdelay(500);

    uint32_t mr13_flip_fspop, mr13_flip_fspwr;
    if (!g_fsp_for_next_freq) {
        mr13_flip_fspwr = (dst_timing_tables->emc_mrw3 & 0xffffff3f) | 0x80;
        mr13_flip_fspop = (dst_timing_tables->emc_mrw3 & 0xffffff3f) | 0x00;
    } else {
        mr13_flip_fspwr = (dst_timing_tables->emc_mrw3 & 0xffffff3f) | 0x40;
        mr13_flip_fspop = (dst_timing_tables->emc_mrw3 & 0xffffff3f) | 0xc0;
    }

    uint32_t mr13_catr_enable = mr13_flip_fspwr | 1;

    if (dram_dev_num == TWO_RANK) {
        if (train_ca || train_ca_vref) {
            mr13_flip_fspop  = (mr13_flip_fspop & 0x3FFFFFFF) | (train_swap_rank ? 0x80000000 : 0x40000000);
        }

        mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF) | (train_swap_rank ? 0x40000000 : 0x80000000);
    }

    if (dram_type == DRAM_TYPE_LPDDR4) {
        emc_write(mr13_flip_fspwr, EMC_MRW3);
        emc_write(dst_timing_tables->emc_mrw, EMC_MRW);
        emc_write(dst_timing_tables->emc_mrw2, EMC_MRW2);
    }

    /* Step 8:
     *   Program the shadow registers.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 8\n");
    //mdelay(500);

    /* Set burst registers. */
    {
        uint32_t pmacro_vttgen_ctrl_1 = emc_read(EMC_PMACRO_VTTGEN_CTRL_1);
        uint32_t xm2comppadctrl       = emc_read(EMC_XM2COMPPADCTRL);
        for (int i = 0; i < dst_timing_tables->num_burst; ++i) {
            uint32_t reg_addr = g_burst_regs_addr[i];

            uint32_t wval;
            if (train_ca || train_ca_vref) {
                wval = dst_timing_tables->shadow_regs_ca_train_arr[i];
            } else if (train_wr || train_wr_vref || train_rd || train_rd_vref) {
                wval = dst_timing_tables->shadow_regs_rdwr_train_arr[i];
            } else {
                wval = dst_timing_tables->burst_regs_arr[i];
            }

            /* Adjust the value to write. */
            switch (reg_addr) {
                case EMC_BASE + EMC_CFG:
                    wval &= (dram_type == DRAM_TYPE_LPDDR4) ? 0x0FFFFFFF : 0xCFFFFFFF;
                    break;
                case EMC_BASE + EMC_MRS_WAIT_CNT:
                    if (opt_zcal_en_cc && is_lpddr2 && (opt_cc_short_zcal == 0) && (opt_short_zcal != 0)) {
                        wval = (wval & 0xFFFFFC00) | (zq_wait_long & 0x3FF);
                    }
                    break;
                case EMC_BASE + EMC_ZCAL_WAIT_CNT:
                    if ((opt_short_zcal != 0) && opt_zcal_en_cc && (opt_cc_short_zcal == 0) && (dram_type == DRAM_TYPE_DDR3)) {
                        wval = (wval & 0xFFFFF800) | (zq_wait_long & 0x7FF);
                    }
                    break;
                case EMC_BASE + EMC_ZCAL_INTERVAL:
                    if (opt_zcal_en_cc) {
                        wval = 0;
                    }
                    break;
                case EMC_BASE + EMC_PMACRO_BRICK_CTRL_RFU1:
                    wval &= 0xF800F800;
                    break;
                case EMC_BASE + EMC_PMACRO_CMD_PAD_TX_CTRL:
                    wval |= 0x04000000;
                    break;
                case EMC_BASE + EMC_PMACRO_AUTOCAL_CFG_COMMON:
                    wval |= 0x00010000;
                    break;
                case EMC_BASE + EMC_TRAINING_CTRL:
                    if (train_swap_rank) {
                        wval |= 0x4000;
                    }
                    break;
                case EMC_BASE + EMC_REFRESH:
                case EMC_BASE + EMC_TREFBW:
                    wval >>= (unk <= 2) ? unk : 0;
                    break;
                case EMC_BASE + EMC_XM2COMPPADCTRL:
                    if ((dst_misc_cfg_1 & 0x20) == 0) {
                        wval = (wval & 0x00FFFFFF) | (xm2comppadctrl & 0xFF000000);
                    }
                    break;
                case EMC_BASE + EMC_DLL_CFG_1:
                    wval = (wval & 0xFFFFDFFF) | (emc_read(EMC_PMACRO_DLL_CFG_1) & 0x00002000);
                    break;
                case EMC_BASE + EMC_PMACRO_VTTGEN_CTRL_1:
                    wval = (wval & 0xFFFF03FF) | (pmacro_vttgen_ctrl_1 & 0xFC00);
                    break;
                case EMC_BASE + EMC_MRW6:
                case EMC_BASE + EMC_MRW7:
                case EMC_BASE + EMC_MRW8:
                case EMC_BASE + EMC_MRW9:
                case EMC_BASE + EMC_MRW14:
                case EMC_BASE + EMC_MRW15:
                case EMC0_BASE + EMC_MRW10:
                case EMC0_BASE + EMC_MRW11:
                case EMC0_BASE + EMC_MRW12:
                case EMC0_BASE + EMC_MRW13:
                case EMC1_BASE + EMC_MRW10:
                case EMC1_BASE + EMC_MRW11:
                case EMC1_BASE + EMC_MRW12:
                case EMC1_BASE + EMC_MRW13:
                    if (dram_type != DRAM_TYPE_LPDDR4) {
                        continue;
                    }
                    break;
            }

            /* Write the value. */
            mmio_write(wval, (volatile uint32_t *)reg_addr);
        }
    }

    if (dram_type == DRAM_TYPE_LPDDR4) {
        /* Use the current timing when training. */
        uint32_t mrw_req;
        if (training_enabled_mask)
            mrw_req = (23 << EMC_MRW_MRW_MA_SHIFT) | (src_timing_tables->run_clocks & EMC_MRW_MRW_OP_MASK);
        else
            mrw_req = (23 << EMC_MRW_MRW_MA_SHIFT) | (dst_timing_tables->run_clocks & EMC_MRW_MRW_OP_MASK);

        emc_write(mrw_req, EMC_MRW);
    }

    /* Per channel burst registers. */
    if (dram_type == DRAM_TYPE_LPDDR4) {
        for (int i = 0; i < dst_timing_tables->num_burst_per_ch; ++i) {
            emc_write_per_ch(dst_timing_tables->burst_reg_per_ch_arr[i], g_burst_perch_regs_addr[i], dst_emc_fbio_cfg7);
        }
    }

    /* Vref regs. */
    for (int i = 0; i < dst_timing_tables->vref_num; ++i) {
        emc_write_per_ch(dst_timing_tables->vref_perch_regs_arr[i], g_vref_perch_regs_addr[i], dst_emc_fbio_cfg7);
    }

    /* Training regs. */
    if (training_enabled_mask) {
        for (int i = 0; i < dst_timing_tables->training_mod_num; ++i) {
            emc_write_per_ch(dst_timing_tables->training_mod_regs_arr[i], g_training_mod_perch_regs_addr[i], dst_emc_fbio_cfg7);
        }
    }

    /* Per channel trimmers. */
    for (int i = 0; i < dst_timing_tables->num_trim_per_ch; ++i) {
        uint32_t reg_addr = g_trim_perch_regs_addr[i];
        uint32_t wval = dst_timing_tables->trim_perch_regs_arr[i];

        if (compensate_trimmer_applicable) {
            switch (reg_addr) {
                case EMC0_BASE + EMC_DATA_BRLSHFT_0:
                case EMC1_BASE + EMC_DATA_BRLSHFT_0:
                case EMC0_BASE + EMC_DATA_BRLSHFT_1:
                case EMC1_BASE + EMC_DATA_BRLSHFT_1:
                    wval = apply_periodic_compensation_trimmer(dst_timing_tables, reg_addr);
                    break;
            }
        }

        emc_write_per_ch(wval, reg_addr, dst_emc_fbio_cfg7);
    }

    /* Trimmers. */
    for (int i = 0; i < dst_timing_tables->num_trim; ++i) {
        uint32_t reg_addr = g_trim_regs_addr[i];
        uint32_t wval = dst_timing_tables->trim_regs_arr[i];

        if (compensate_trimmer_applicable) {
            switch (reg_addr) {
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1:
                case EMC_BASE + EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2:
                    wval = apply_periodic_compensation_trimmer(dst_timing_tables, reg_addr);
                    break;
            }
        }

            mmio_write(wval, (volatile uint32_t *)reg_addr);
    }

    if (training_enabled_mask) {
        if (train_wr && dst_timing_tables->periodic_training) {
            periodic_compensation_handler(WRITE_TRAINING_SEQUENCE, dram_dev_num, src_timing_tables, dst_timing_tables);
        }
    } else {
        /* Write burst_mc_regs. */
        for (int i = 0; i < dst_timing_tables->num_mc_regs; ++i) {
            mmio_write(dst_timing_tables->burst_mc_regs_arr[i], (volatile uint32_t *)g_burst_mc_regs_addr[i]);
        }

        /* Registers to be programmed on the faster clock. */
        if (dst_timing_tables->rate_khz < src_timing_tables->rate_khz) {
            for (int i = 0; i < dst_timing_tables->num_up_down; ++i) {
                mmio_write(dst_timing_tables->la_scale_regs_arr[i], (volatile uint32_t *)g_la_scale_regs_addr[i]);
            }
        }
    }

    if ((dst_misc_cfg_1 & 2) != 0 && (dst_misc_cfg_1 & 1) == 0) {
        emc_write(dst_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1, EMC_PMACRO_BRICK_CTRL_RFU1);
        emc_write(dst_timing_tables->burst_regs.emc_pmacro_cmd_pad_tx_ctrl, EMC_PMACRO_CMD_PAD_TX_CTRL);
    }

    emc_write(EMC_CFG_PIPE_CLK_CLK_ALWAYS_ON, EMC_CFG_PIPE_CLK);
    emc_write(dst_timing_tables->emc_fdpd_ctrl_cmd_no_ramp & ~EMC_FDPD_CTRL_CMD_NO_RAMP_CMD_DPD_NO_RAMP_ENABLE, EMC_FDPD_CTRL_CMD_NO_RAMP);


    /* Step 9:
     *   LPDDR4 section A.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 9/10\n");
    //mdelay(500);

    uint32_t emc_dbg_write_active = emc_dbg_o;
    if (dram_type == DRAM_TYPE_LPDDR4) {
        emc_write(emc_zcal_interval, EMC_ZCAL_INTERVAL);
        emc_write((dst_emc_zcal_wait_cnt & 0xFFFFF800) | 1, EMC_ZCAL_WAIT_CNT);
        emc_write(emc_dbg_o | (EMC_DBG_WRITE_MUX_ACTIVE | EMC_DBG_WRITE_ACTIVE_ONLY), EMC_DBG);
        emc_write(emc_zcal_interval, EMC_ZCAL_INTERVAL);
        emc_write(emc_dbg_o, EMC_DBG);

        if (training_enabled_mask) {
            emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);
            emc_write(dst_timing_tables->burst_regs.emc_pmacro_autocal_cfg_common | EMC_PMACRO_AUTOCAL_CFG_COMMON_E_CAL_BYPASS_DVFS, EMC_PMACRO_AUTOCAL_CFG_COMMON);

            if (train_ca || train_ca_vref) {
                emc_write(src_timing_tables->burst_regs.emc_fbio_cfg5 | EMC_FBIO_CFG5_CMD_BUS_RETURN_TO_ZERO, EMC_FBIO_CFG5);
            }

            emc_write(emc_dbg_o, EMC_DBG);

            if (dst_emc_fbio_cfg7 == (EMC_FBIO_CFG7_CH0_ENABLE | EMC_FBIO_CFG7_CH1_ENABLE)) {
                ccfifo_write(EMC_CFG_SYNC, 1, 0);
            }

            /* Change CFG_SWAP. */
            ccfifo_write(EMC_DBG, ((emc_dbg_o & 0xF3FFFFFF) | 0x4000000), 0);
        }

        ccfifo_write(EMC_SELF_REF, 0x101, 0);

        if (!(train_ca || train_ca_vref) && dst_clock_period <= zqcal_before_cc_cutoff) {
            ccfifo_write(EMC_MRW3, mr13_flip_fspwr ^ 0x40, 0);
            ccfifo_write(EMC_MRW6, (src_timing_tables->burst_regs.emc_mrw6 & 0x0000C0C0) | (dst_timing_tables->burst_regs.emc_mrw6 & 0xFFFF3F3F), 0);
            ccfifo_write(EMC_MRW14, (src_timing_tables->burst_regs.emc_mrw14 & 0x00003838) | (dst_timing_tables->burst_regs.emc_mrw14 & 0xFFFF0707), 0);

            if (dram_dev_num == TWO_RANK) {
                ccfifo_write(EMC_MRW7, (src_timing_tables->burst_regs.emc_mrw7 & 0x0000C0C0) | (dst_timing_tables->burst_regs.emc_mrw7 & 0xFFFF3F3F), 0);
                ccfifo_write(EMC_MRW15, (src_timing_tables->burst_regs.emc_mrw15 & 0x00003838) | (dst_timing_tables->burst_regs.emc_mrw15 & 0xFFFF0707), 0);
            }

            if (opt_zcal_en_cc) {
                if (dram_dev_num == ONE_RANK || shared_zq_resistor) {
                    ccfifo_write(EMC_ZQ_CAL, (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                } else {
                    ccfifo_write(EMC_ZQ_CAL, EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                }
            }
        }

        if (training_enabled_mask) {
            emc_dbg_write_active = (emc_dbg_o & 0xF7FFFFFF) | 0x4000000 | EMC_DBG_WRITE_ACTIVE_ONLY;
            ccfifo_write(EMC_DBG, emc_dbg_write_active, 0);
        }

        if (train_ca || train_ca_vref) {
            ccfifo_write(EMC_PMACRO_DATA_RX_TERM_MODE, src_timing_tables->burst_regs.emc_pmacro_data_rx_term_mode & 0xFFFFFCCC, 0);

            if (dram_dev_num == TWO_RANK && train_swap_rank) {
                ccfifo_write(EMC_MRW3, mr13_flip_fspop | 0x8, (1000 * src_t_rp) / src_clock_period);
                ccfifo_write(EMC_MRW3, mr13_catr_enable | 0x8, 0);
            } else {
                ccfifo_write(EMC_MRW3, mr13_catr_enable | 0x8, (1000 * src_t_rp) / src_clock_period);
            }

            ccfifo_write(EMC_TR_CTRL_0, (dst_timing_tables->emc_tr_ctrl_0 & 0x3F1000) | 0x100012A, 0);

            ccfifo_write(EMC_INTSTATUS, 0, 1000000 / src_clock_period);
        } else {
            ccfifo_write(EMC_MRW3, mr13_flip_fspop | 0x8, (1000 * src_t_rp) / src_clock_period);

            ccfifo_write(EMC_INTSTATUS, 0, max(div_o3_f(dst_t_fc_lpddr4_hz, src_clock_period), max(div_o3_f(14000, src_clock_period), (uint32_t)10)));
        }

        uint32_t t = 30 + (cya_allow_ref_cc ? ((4000 * src_t_rfc + 1000 * src_t_rp) / src_clock_period) : 0);
        ccfifo_write(EMC_PIN, emc_pin_o & ~(EMC_PIN_PIN_CKE_PER_DEV | EMC_PIN_PIN_CKEB | EMC_PIN_PIN_CKE), t);
    } else {
        ccfifo_write(EMC_SELF_REF, 0x1, 0);
    }

    uint32_t ref_delay_mult = 1;
    if (ref_b4_sref_en) ++ref_delay_mult;
    if (cya_allow_ref_cc) ++ref_delay_mult;
    if (cya_issue_pc_ref) ++ref_delay_mult;

    uint32_t ref_delay = 20 + ref_delay_mult * (((1000 * src_t_rfc) / src_clock_period) + ((1000 * src_t_rp) / src_clock_period));

    /* Step 11:
     *   Ramp down.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 11\n");
    //mdelay(500);

    ccfifo_write(EMC_CFG_SYNC, 1, (dram_type == DRAM_TYPE_LPDDR4) ? 0 : ref_delay);

    bool do_ramp_up = (dst_misc_cfg_1 & 2) == 0 || (dst_misc_cfg_1 & 1) != 0;
    if (!do_ramp_up) {
        ccfifo_write(EMC_FBIO_CFG5, emc_read(EMC_FBIO_CFG5) & 0xF7FFFFFF, 12);
    }

    ccfifo_write(EMC_DBG, emc_set_shadow_bypass(ACTIVE, emc_dbg_write_active & 0xBFFFFFFF), 0);

    if ((dst_misc_cfg_2 & 0x10) == 0) {
        dvfs_power_ramp_down(src_timing_tables, dst_timing_tables, false, 0);
    }

    ccfifo_write(EMC_DBG, emc_set_shadow_bypass(ACTIVE, emc_dbg_write_active | 0x40000000), 0);

    uint32_t ramp_down_wait = 0;
    if (do_ramp_up) {
        ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, src_timing_tables->burst_regs.emc_pmacro_cmd_pad_tx_ctrl | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_DRVFORCEON, 0);
        ccfifo_write(EMC_FBIO_CFG5, src_timing_tables->burst_regs.emc_fbio_cfg5 | 0x100, 12);

        bool misc_flag = (dst_misc_cfg_1 & 3) == 3;

        uint32_t timescale = (100000 << ((dst_misc_cfg_1 >> 2) & 7));
        uint32_t delay     = (timescale / src_clock_period);

        if (src_rate_khz > 1150747 || misc_flag) {
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, src_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xFEEDFEED, delay + 1);
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, src_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xFE40FE40, delay + 1);
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, src_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xF800F800, delay + 1);


            ramp_down_wait = 3 * timescale + 12 * src_clock_period;
        } else {
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, src_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xF800F800, delay + 20);

            ramp_down_wait = timescale + 32 * src_clock_period;
        }

        if (src_rate_khz > 600240 || misc_flag) {
            ccfifo_write(EMC_INTSTATUS, 0, delay + 1);

            ramp_down_wait += 2 * timescale;
        }
    }

    /* Step 12:
     *   Trigger the clock change.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 12\n");
    //mdelay(500);

    ccfifo_write(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
    ccfifo_write(EMC_INTSTATUS, 0, dst_timing_tables->clkchange_delay);

    ccfifo_write(EMC_DBG, emc_set_shadow_bypass(ACTIVE, emc_dbg_write_active & 0xBFFFFFFF), 0);

    if ((dst_misc_cfg_2 & 0x10) == 0) {
        dvfs_power_ramp_down(src_timing_tables, dst_timing_tables, false, 1);
    }

    if (training_enabled_mask) {
        ccfifo_write(EMC_DBG, emc_set_shadow_bypass(ACTIVE, emc_dbg_write_active | 0x40000000), 0);
    }

    /* Step 13:
     *   Ramp up.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 13\n");
    //mdelay(500);

    uint32_t ramp_up_wait = 0;
    if (do_ramp_up) {
        ramp_up_wait = dvfs_power_ramp_up(dst_clock_period, false, src_timing_tables, dst_timing_tables, needs_training);
    }

    if (ramp_up_wait < 1001000 && src_timing_tables->ramp_wait != dst_timing_tables->ramp_wait) {
        ccfifo_write(EMC_INTSTATUS, 0, 1 + ((1001000 - ramp_up_wait) / dst_clock_period));
    }

    if (do_ramp_up) {
        ccfifo_write(EMC_DBG, emc_dbg_write_active, 0);
    } else {
        ccfifo_write(EMC_FBIO_CFG5, emc_read(EMC_FBIO_CFG5), 0);
        ccfifo_write(EMC_DBG, emc_dbg_write_active, 12);
    }

    /* Step 14:
     *   Bringup CKE pins.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 14\n");
    //mdelay(500);

    if (dram_type == DRAM_TYPE_LPDDR4) {
        uint32_t pin_val;
        if (train_ca || train_ca_vref) {
            pin_val = (dst_misc_cfg_0 & 1) ? ((emc_pin_o & 0xFFFCFFF8) | (((dst_misc_cfg_0 >> 1) & 3) << 16)) : emc_pin_o;
            pin_val &= 0xFFFFFFF8;
            if (dram_dev_num == TWO_RANK) {
                if (train_swap_rank) {
                    pin_val |= 5;
                } else {
                    pin_val |= 6;
                }
            }
        } else {
            pin_val = (dst_misc_cfg_0 & 1) ? ((emc_pin_o & 0xFFFCFFFF) | (((dst_misc_cfg_0 >> 1) & 3) << 16)) : emc_pin_o;
            pin_val &= 0xFFFFFFF8;
            if (dram_dev_num == TWO_RANK) {
                pin_val |= 7;
            } else {
                pin_val |= 1;
            }
        }

        ccfifo_write(EMC_PIN, pin_val, 0);
    }

    /* Step 15:
     *   Calculate zqlatch wait time; has dependency on ramping times.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 15\n");
    //mdelay(500);

    uint32_t dst_t_pdex_hz = 1000 * dst_t_pdex;

    uint32_t zq_latch_dvfs_wait_time;
    if (dst_clock_period <= zqcal_before_cc_cutoff) {
        zq_latch_dvfs_wait_time = (ramp_up_wait + ramp_down_wait) / dst_clock_period;
    } else {
        zq_latch_dvfs_wait_time = div_o3(dst_t_pdex_hz, dst_clock_period);
    }

    if (!(train_ca || train_ca_vref) && (dram_type == DRAM_TYPE_LPDDR4) && opt_zcal_en_cc) {
        int offset = (int)((tZQCAL_lpddr4 - adj_dst_t_fc_lpddr4) / dst_clock_period) - (int)zq_latch_dvfs_wait_time;
        int addl_wait = (int)div_o3(dst_t_pdex_hz, dst_clock_period);

        if (dram_dev_num == TWO_RANK) {
            if (shared_zq_resistor) {
                if (dst_clock_period > zqcal_before_cc_cutoff) {
                    ccfifo_write(EMC_ZQ_CAL, (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD, max_t(int, 0, addl_wait));
                }

                ccfifo_write(EMC_ZQ_CAL, (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_LATCH_CMD, max_t(int, 0, offset + addl_wait));
                ccfifo_write(EMC_ZQ_CAL, (1 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);

                if (!training_enabled_mask) {
                    ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xFFFFFFF7) | 0xC000000 | (dst_misc_cfg_2 & 8), 0);
                    ccfifo_write(EMC_SELF_REF, 0, 0);
                    ccfifo_write(EMC_REF, 0, 0);
                }

                ccfifo_write(EMC_ZQ_CAL, (1 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_LATCH_CMD, zq_wait_short + div_14000_by_dst_period + (tZQCAL_lpddr4 / dst_clock_period));
            } else {
                if (dst_clock_period > zqcal_before_cc_cutoff) {
                    ccfifo_write(EMC_ZQ_CAL, (0 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD, max_t(int, 0, addl_wait));
                }

                if (!training_enabled_mask) {
                    ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xFFFFFFF7) | 0xC000000 | (dst_misc_cfg_2 & 8), max_t(int, 0, addl_wait));
                    ccfifo_write(EMC_SELF_REF, 0, 0);
                    ccfifo_write(EMC_REF, 0, 0);
                }

                ccfifo_write(EMC_ZQ_CAL, (0 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_LATCH_CMD, div_14000_by_dst_period + max_t(int, 0, offset));
            }
        } else {
            if (dst_clock_period > zqcal_before_cc_cutoff) {
                ccfifo_write(EMC_ZQ_CAL, (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD, max_t(int, 0, addl_wait));
            }

            if (!training_enabled_mask) {
                ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xFFFFFFF7) | 0xC000000 | (dst_misc_cfg_2 & 8), max_t(int, 0, addl_wait));
                ccfifo_write(EMC_SELF_REF, 0, 0);
                ccfifo_write(EMC_REF, 0x80000000, 0);
            }

            ccfifo_write(EMC_ZQ_CAL, (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_LATCH_CMD, div_14000_by_dst_period + max_t(int, 0, offset));
        }
    }

    /* WAR: delay for zqlatch */
    ccfifo_write(EMC_INTSTATUS, 0, 10);

    /* Step 16:
     *   LPDDR4 Conditional Training Kickoff.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 16\n");
    //mdelay(500);

    if (training_enabled_mask && dram_type == DRAM_TYPE_LPDDR4) {
        if (opt_do_sw_qrst) {
            ccfifo_write(EMC_ISSUE_QRST, 1, 0);
            ccfifo_write(EMC_ISSUE_QRST, 0, 2);
        }

        ccfifo_write(EMC_INTSTATUS, 0, (1020000 / dst_clock_period));

        {
            uint32_t train_cmd = 0;
            if (train_ca) {
                train_cmd |= (1 << 1);  /* CA */
            }
            if (train_ca_vref) {
                train_cmd |= (1 << 5);  /* CA_VREF */
            }
            if (train_wr) {
                train_cmd |= (1 << 3);  /* WR */
            }
            if (train_wr_vref) {
                train_cmd |= (1 << 6);  /* WR_VREF */
            }
            if (train_rd) {
                train_cmd |= (1 << 2);  /* RD */
            }
            if (train_rd_vref) {
                train_cmd |= (1 << 7);  /* RD_VREF */
            }

            train_cmd |= (1 << 31);     /* GO */

            ccfifo_write(EMC_TRAINING_CMD, train_cmd, 0);
        }

        ccfifo_write(EMC_SWITCH_BACK_CTRL, 1, 0);

        if (!(train_ca || train_ca_vref) || train_swap_rank) {
            ccfifo_write(EMC_MRW3, mr13_flip_fspop ^ 0xC0, 0);
            ccfifo_write(EMC_INTSTATUS, 0, (1000000 / dst_clock_period));
        }

        {
            uint32_t pin_val = (dst_misc_cfg_0 & 1) ? ((emc_pin_o & 0xFFFCFFFF) | (((dst_misc_cfg_0 >> 1) & 3) << 16)) : emc_pin_o;
            ccfifo_write(EMC_PIN, pin_val & 0xFFFFFFF8, 0);
        }

        ccfifo_write(EMC_CFG_SYNC, 1, 0);

        if ((src_misc_cfg_1 & 3) != 2) {
            ccfifo_write(EMC_DBG, emc_set_shadow_bypass(ACTIVE, emc_dbg_write_active | 0x40000000), 0);

            ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, dst_timing_tables->burst_regs.emc_pmacro_cmd_pad_tx_ctrl | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_DRVFORCEON, 0);
            ccfifo_write(EMC_FBIO_CFG5, dst_timing_tables->burst_regs.emc_fbio_cfg5 | 0x100, 12);

            bool misc_flag = (src_misc_cfg_1 & 3) == 3;

            uint32_t timescale = (100000 << ((src_misc_cfg_1 >> 2) & 7));
            uint32_t delay     = (timescale / dst_clock_period);

            if (dst_rate_khz > 1150747 || misc_flag) {
                ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xFEEDFEED, delay + 1);
                ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xFE40FE40, delay + 1);
                ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xF800F800, delay + 1);
            } else {
                ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, dst_timing_tables->burst_regs.emc_pmacro_brick_ctrl_rfu1 & 0xF800F800, delay + 20);
            }

            if (dst_rate_khz > 600240 || misc_flag) {
                ccfifo_write(EMC_INTSTATUS, 0, delay + 1);
            }
        } else {
            ccfifo_write(EMC_FBIO_CFG5, emc_read(EMC_FBIO_CFG5) & 0xF7FFFFFF, 12);
            ccfifo_write(EMC_DBG, emc_set_shadow_bypass(ACTIVE, emc_dbg_write_active | 0x40000000), 0);
        }

        ccfifo_write(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
        ccfifo_write(EMC_DBG, emc_set_shadow_bypass(ACTIVE, emc_dbg_write_active & 0xBFFFFFFF), 0);

        if ((dst_misc_cfg_2 & 0x10) == 0) {
            dvfs_power_ramp_down(src_timing_tables, dst_timing_tables, true, 1);
        }

        if ((src_misc_cfg_1 & 3) != 2) {
            dvfs_power_ramp_up(src_clock_period, true, src_timing_tables, dst_timing_tables, needs_training);

            if (ramp_up_wait < 1001000 && src_timing_tables->ramp_wait != dst_timing_tables->ramp_wait) {
                ccfifo_write(EMC_INTSTATUS, 0, 1 + ((1001000 - ramp_up_wait) / dst_clock_period));
            }

            ccfifo_write(EMC_DBG, emc_dbg_write_active, 0);
        } else {
            if (ramp_up_wait < 1001000 && src_timing_tables->ramp_wait != dst_timing_tables->ramp_wait) {
                ccfifo_write(EMC_INTSTATUS, 0, 1 + ((1001000 - ramp_up_wait) / dst_clock_period));
            }

            ccfifo_write(EMC_FBIO_CFG5, emc_read(EMC_FBIO_CFG5), 0);
            ccfifo_write(EMC_DBG, emc_dbg_write_active, 12);
        }

        {
            uint32_t pin_val = (src_misc_cfg_0 & 1) ? ((emc_pin_o & 0xFFFCFFFF) | (((src_misc_cfg_0 >> 1) & 3) << 16)) : emc_pin_o;
            pin_val &= 0xFFFFFFF8;
            pin_val |= (dram_dev_num == TWO_RANK) ? 7 : 1;
            ccfifo_write(EMC_PIN, pin_val, 0);
        }

        if (train_ca || train_ca_vref) {
            ccfifo_write(EMC_TR_CTRL_0, 0x2A, (200000 / src_clock_period));
            ccfifo_write(EMC_TR_CTRL_0, 0x20, (1000000 / src_clock_period));
            ccfifo_write(EMC_MRW3, mr13_catr_enable & 0xFFFFFFFE, 0);
            ccfifo_write(EMC_INTSTATUS, 0, (1000000 / src_clock_period));
            ccfifo_write(EMC_PMACRO_DATA_RX_TERM_MODE, src_timing_tables->burst_regs.emc_pmacro_data_rx_term_mode, 0);
        }

        ccfifo_write(EMC_DBG, emc_dbg_o, 0);

        if (opt_zcal_en_cc) {
            if (shared_zq_resistor) {
                ccfifo_write(EMC_ZQ_CAL, (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                ccfifo_write(EMC_ZQ_CAL, (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_LATCH_CMD, 1 + div_o3(1000000, src_clock_period));

                if ((!(train_ca || train_ca_vref) || train_swap_rank) && dram_dev_num == TWO_RANK) {
                    ccfifo_write(EMC_ZQ_CAL, (1 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                    ccfifo_write(EMC_ZQ_CAL, (1 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_LATCH_CMD, 1 + div_o3(1000000, src_clock_period));
                }
            } else {
                ccfifo_write(EMC_ZQ_CAL, (dram_dev_num == ONE_RANK ? (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) : (0 << EMC_ZQ_CAL_DEV_SEL_SHIFT)) | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                ccfifo_write(EMC_ZQ_CAL, (dram_dev_num == ONE_RANK ? (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) : (0 << EMC_ZQ_CAL_DEV_SEL_SHIFT)) | EMC_ZQ_CAL_ZQ_LATCH_CMD, 1 + div_o3(1000000, src_clock_period));
            }
        }

        if (!(train_ca || train_ca_vref)) {
            ccfifo_write(EMC_MRW3, ((mr13_flip_fspop & 0xF3FFFFF7) | (dst_misc_cfg_2 & 8)) ^ 0x0C0000C0, 0);
        }

        ccfifo_write(EMC_SELF_REF, 0x0, 0);
    }

    /* Step 17:
     *   exit self refresh.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 17\n");
    //mdelay(500);

    if (dram_type != DRAM_TYPE_LPDDR4) {
        ccfifo_write(EMC_SELF_REF, 0, 0);
    }

    /* Step 18:
     *   Send MRWs to LPDDR3/DDR3.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 18\n");
    //mdelay(500);

    if (is_lpddr2) {
        ccfifo_write(EMC_MRW2, dst_timing_tables->emc_mrw2, 0);
        ccfifo_write(EMC_MRW, dst_timing_tables->emc_mrw, 0);

        if (is_lpddr3) {
            ccfifo_write(EMC_MRW4, dst_timing_tables->emc_mrw4, 0);
        }
    } else if (dram_type == DRAM_TYPE_DDR3) {
        if (opt_dll_mode) {
            ccfifo_write(EMC_EMRS, dst_timing_tables->emc_emrs & ~EMC_EMRS_USE_EMRS_LONG_CNT, 0);
        }
        ccfifo_write(EMC_EMRS2, dst_timing_tables->emc_emrs2 & ~EMC_EMRS2_USE_EMRS2_LONG_CNT, 0);
        ccfifo_write(EMC_MRS, dst_timing_tables->emc_mrs | EMC_EMRS_USE_EMRS_LONG_CNT, 0);
    }

    /* Step 19:
     *   ZQCAL for LPDDR3/DDR3
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 19\n");
    //mdelay(500);

    if (opt_zcal_en_cc) {
        if (is_lpddr2) {
            uint32_t zq_op = opt_cc_short_zcal ? dst_timing_tables->zq_op_cc_short_zcal : dst_timing_tables->zq_op_cc_long_zcal;
            uint32_t zcal_wait_time_ps = opt_cc_short_zcal ? dst_timing_tables->zcal_wait_time_ps_cc_short_zcal : dst_timing_tables->zcal_wait_time_ps_cc_long_zcal;
            uint32_t zcal_wait_time_clocks = div_o3(zcal_wait_time_ps, dst_clock_period);

            ccfifo_write(EMC_MRS_WAIT_CNT2, (zcal_wait_time_clocks & 0x3FF) | ((zcal_wait_time_clocks & 0x7FF) << 16), 0);
            ccfifo_write(EMC_MRW, (zq_op | 0x880C0000) - 0x20000, 0);

            if (dram_dev_num == TWO_RANK) {
                ccfifo_write(EMC_MRW, zq_op | 0x480A0000, 0);
            }
        } else if (dram_type == DRAM_TYPE_DDR3) {
            ccfifo_write(EMC_ZQ_CAL, (2 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD | (opt_cc_short_zcal ? 0 : EMC_ZQ_CAL_LONG), 0);
            if (dram_dev_num == TWO_RANK) {
                ccfifo_write(EMC_ZQ_CAL, (1 << EMC_ZQ_CAL_DEV_SEL_SHIFT) | EMC_ZQ_CAL_ZQ_CAL_CMD | (opt_cc_short_zcal ? 0 : EMC_ZQ_CAL_LONG), 0);
            }
        }
    }

    /* Step 20:
     *   Issue ref and optional QRST.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 20\n");
    //mdelay(500);

    if (training_enabled_mask || dram_type != DRAM_TYPE_LPDDR4) {
        ccfifo_write(EMC_REF, dram_dev_num == ONE_RANK ? 0x80000000 : 0x00000000, 0);
    }

    if (opt_do_sw_qrst) {
        ccfifo_write(EMC_ISSUE_QRST, 1, 0);
        ccfifo_write(EMC_ISSUE_QRST, 0, 2);
    }

    /* Step 21:
     *   Restore ZCAL and ZCAL interval.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 21\n");
    //mdelay(500);

    if (save_restore_clkstop_pd || opt_zcal_en_cc || (training_enabled_mask && dram_type == DRAM_TYPE_LPDDR4)) {
        ccfifo_write(EMC_DBG, emc_set_shadow_bypass(ACTIVE, emc_dbg_o), 0);

        if (opt_zcal_en_cc) {
            if (training_enabled_mask) {
                ccfifo_write(EMC_ZCAL_INTERVAL, (dst_misc_cfg_2 & 2) ? 0 : src_timing_tables->burst_regs.emc_zcal_interval, 0);
            } else if (dram_type != DRAM_TYPE_LPDDR4) {
                ccfifo_write(EMC_ZCAL_INTERVAL, (dst_misc_cfg_2 & 2) ? 0 : dst_timing_tables->burst_regs.emc_zcal_interval, 0);
            }
        }

        if (save_restore_clkstop_pd || (training_enabled_mask && dram_type == DRAM_TYPE_LPDDR4)) {
            ccfifo_write(EMC_CFG, dst_timing_tables->burst_regs.emc_cfg & ~EMC_CFG_DYN_SELF_REF, 0);
        }

        if (training_enabled_mask && dram_type == DRAM_TYPE_LPDDR4) {
            ccfifo_write(EMC_SEL_DPD_CTRL, src_timing_tables->emc_sel_dpd_ctrl, 0);
        }

        ccfifo_write(EMC_DBG, emc_dbg_o, 0);
    }

    /* Step 22:
     *   Restore EMC_CFG_PIPE_CLK.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 22\n");
    //mdelay(500);

    ccfifo_write(EMC_CFG_PIPE_CLK, emc_cfg_pipe_clk_o, 0);
    ccfifo_write(EMC_INTSTATUS, 0, dst_timing_tables->pipe_clk_delay);

    /* Step 23:
     *   Do clock change.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 23\n");
    //mdelay(500);

    if (training_enabled_mask) {
        uint32_t clk_source_emc;
        if (dst_emc_fbio_cfg7 & (EMC_FBIO_CFG7_CH0_ENABLE | EMC_FBIO_CFG7_CH1_ENABLE)) {
            clk_source_emc = car_read(&car->clk_source_emc);
            car_write(clk_source_emc, &car->clk_source_emc_safe);
        } else {
            clk_source_emc = 0;
        }

        change_dll_src(src_timing_tables, clk_source_emc);
    }

    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 23b\n");
    //mdelay(500);

    {
        uint32_t dig_dll = (emc_read(EMC_CFG_DIG_DLL) & 0xFFFFFFE4) | 8;
        if ((dst_misc_cfg_2 & 1) == 0) {
            dig_dll = (dig_dll & 0xFFFFFF3F) | 0x80;
        }
        emc_write(dig_dll, EMC_CFG_DIG_DLL);
        emc_read(EMC_CFG_DIG_DLL);
    }

    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 23c\n");
    //mdelay(500);

    mc_read(MC_EMEM_ADR_CFG);
    emc_read(EMC_INTSTATUS);

    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 23d (%08x)\n", dst_clk_src);
    //mdelay(500);

    if (dst_emc_fbio_cfg7 & (EMC_FBIO_CFG7_CH0_ENABLE | EMC_FBIO_CFG7_CH1_ENABLE)) {
        car_write(dst_clk_src, &car->clk_source_emc);
        car_read(&car->clk_source_emc);
    }

    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 23e\n");

    if (wait_for_update(EMC_INTSTATUS, EMC_INTSTATUS_CLKCHANGE_COMPLETE, true, dst_emc_fbio_cfg7)) {
        //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: wait for clock change complete failed\n");
        //mdelay(500);
        return;
    }

    /* Step 24:
     *   Save training results.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 24\n");

    if (training_enabled_mask) {
        uint32_t tmp_emem_numdev = mc_read(MC_EMEM_ADR_CFG) & 1;

        uint32_t emc_dbg_tmp = emc_read(EMC_DBG);
        emc_write(emc_dbg_tmp | 1, EMC_DBG);       /* Set READ_MUX to ASSEMBLY. */

        uint32_t tmp_dram_dev_num = 1 + tmp_emem_numdev;

        /* Save CA results. */
        if (train_ca) {
            dst_timing_tables->trim_perch_regs.emc_cmd_brlshft_0 = emc0_read(EMC_CMD_BRLSHFT_0);
            dst_timing_tables->trim_perch_regs.emc_cmd_brlshft_1 = emc1_read(EMC_CMD_BRLSHFT_1);
            dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4 = emc0_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4);
            dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5 = emc1_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5);

            if (train_self_refresh) {
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2);
            }
        }

        /* Save CA_VREF results. */
        if (train_ca_vref) {
            uint32_t emc0_training_opt_ca_vref = emc0_read(EMC_TRAINING_OPT_CA_VREF);
            uint32_t emc1_training_opt_ca_vref = emc1_read(EMC_TRAINING_OPT_CA_VREF);

            uint32_t rank_mask = tmp_dram_dev_num == TWO_RANK ? 0x480C0000 : 0xC80C0000;

            dst_timing_tables->burst_reg_per_ch.emc0_mrw10 = (emc0_training_opt_ca_vref & 0xFFFF) | 0x880C0000;
            dst_timing_tables->burst_reg_per_ch.emc1_mrw10 = (emc1_training_opt_ca_vref & 0xFFFF)  | 0x880C0000;
            dst_timing_tables->burst_reg_per_ch.emc0_mrw11 = (rank_mask & 0xFFFFFF00) | ((emc0_training_opt_ca_vref >> 16) & 0xFFFF);
            dst_timing_tables->burst_reg_per_ch.emc1_mrw11 = (rank_mask & 0xFFFFFF00) | ((emc1_training_opt_ca_vref >> 16) & 0xFFFF);
        }

        /* Save RD results. */
        if (train_rd) {
            dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_0 = emc0_read(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0);
            dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_1 = emc0_read(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1);
            dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_2 = emc1_read(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2);
            dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_3 = emc1_read(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3);

            if (tmp_dram_dev_num == TWO_RANK) {
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_0 = emc0_read(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_1 = emc0_read(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_2 = emc1_read(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_3 = emc1_read(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3);
            }

            if (train_self_refresh) {
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_0 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_1 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_2 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_0 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_1 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_2 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_0 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_1 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_2 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_0 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_1 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_2 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_0 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_1 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_2 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_0 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_1 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_2 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_0 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_1 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_2 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_0 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_1 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1);
                dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_2 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2);

                if (tmp_dram_dev_num == TWO_RANK) {
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_0 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_1 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_2 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_0 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_1 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_2 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_0 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_1 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_2 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_0 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_1 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_2 = emc0_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_0 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_1 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_2 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_0 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_1 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_2 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_0 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_1 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_2 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_0 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_1 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_2 = emc1_read(EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2);
                }
            }

            /* Save RD_VREF results. */
            if (train_rd_vref) {
                uint32_t emc_pmacro_ib_vref_dq_0 = emc0_read(EMC_PMACRO_IB_VREF_DQ_0);
                uint32_t emc_pmacro_ib_vref_dq_1 = emc0_read(EMC_PMACRO_IB_VREF_DQ_1);

                #define GET_SAVE_RESTORE_MOD_REG(n) ((dst_timing_tables->save_restore_mod_regs[n] & 0x80000000) ? (~dst_timing_tables->save_restore_mod_regs[n]) : (dst_timing_tables->save_restore_mod_regs[n]))

                uint8_t ib_vref_dq_byte0_icr = ((emc_pmacro_ib_vref_dq_0 >>  0) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(0) & 0x7F);
                uint8_t ib_vref_dq_byte1_icr = ((emc_pmacro_ib_vref_dq_0 >>  8) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(1) & 0x7F);
                uint8_t ib_vref_dq_byte2_icr = ((emc_pmacro_ib_vref_dq_0 >> 16) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(2) & 0x7F);
                uint8_t ib_vref_dq_byte3_icr = ((emc_pmacro_ib_vref_dq_0 >> 24) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(3) & 0x7F);
                uint8_t ib_vref_dq_byte4_icr = ((emc_pmacro_ib_vref_dq_1 >>  0) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(4) & 0x7F);
                uint8_t ib_vref_dq_byte5_icr = ((emc_pmacro_ib_vref_dq_1 >>  8) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(5) & 0x7F);
                uint8_t ib_vref_dq_byte6_icr = ((emc_pmacro_ib_vref_dq_1 >> 16) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(6) & 0x7F);
                uint8_t ib_vref_dq_byte7_icr = ((emc_pmacro_ib_vref_dq_1 >> 24) & 0x7F) + (GET_SAVE_RESTORE_MOD_REG(7) & 0x7F);

                dst_timing_tables->trim_regs.emc_pmacro_ib_vref_dq_0 = ((ib_vref_dq_byte0_icr & 0x7F) | (ib_vref_dq_byte1_icr & 0x7F) << 8) | ((ib_vref_dq_byte2_icr & 0x7F) << 16) | ((ib_vref_dq_byte3_icr & 0x7F) << 24);


                dst_timing_tables->trim_regs.emc_pmacro_ib_vref_dq_1 = ((ib_vref_dq_byte4_icr & 0x7F) | (ib_vref_dq_byte5_icr & 0x7F) << 8) | ((ib_vref_dq_byte6_icr & 0x7F) << 16) | ((ib_vref_dq_byte7_icr & 0x7F) << 24);

                #undef GET_SAVE_RESTORE_MOD_REG
            }
        }

        /* Save WR results. */
        if (train_wr) {
            dst_timing_tables->trim_perch_regs.emc0_data_brlshft_0 = emc0_read(EMC_DATA_BRLSHFT_0);
            dst_timing_tables->trim_perch_regs.emc1_data_brlshft_0 = emc1_read(EMC_DATA_BRLSHFT_0);

            if (tmp_dram_dev_num == TWO_RANK) {
                dst_timing_tables->trim_perch_regs.emc0_data_brlshft_1 = emc0_read(EMC_DATA_BRLSHFT_1);
                dst_timing_tables->trim_perch_regs.emc1_data_brlshft_1 = emc1_read(EMC_DATA_BRLSHFT_1);
            }

            dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0 = emc0_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0);
            dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1 = emc0_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1);
            dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2 = emc1_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2);
            dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3 = emc1_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3);

            if (tmp_dram_dev_num == TWO_RANK) {
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0 = emc0_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1 = emc0_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2 = emc1_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3 = emc1_read(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3);
            }

            if (train_self_refresh) {
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_0 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_1 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_2 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_0 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_1 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_2 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_0 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_1 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_2 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_0 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_1 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1);
                dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_2 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2);

                if (tmp_dram_dev_num == TWO_RANK) {
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_0 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_1 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_2 = emc0_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_0 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_1 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_2 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_0 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_1 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_2 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_0 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_1 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_2 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_0 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_1 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1);
                    dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_2 = emc1_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2);
                }
            }

            /* Save WR_VREF results. */
            if (train_wr_vref) {
                uint32_t emc0_training_opt_dq_ob_vref = emc0_read(EMC_TRAINING_OPT_DQ_OB_VREF);
                uint32_t emc1_training_opt_dq_ob_vref = emc1_read(EMC_TRAINING_OPT_DQ_OB_VREF);

                #define GET_SAVE_RESTORE_MOD_REG(n) ((dst_timing_tables->save_restore_mod_regs[n] & 0x80000000) ? (~dst_timing_tables->save_restore_mod_regs[n]) : (dst_timing_tables->save_restore_mod_regs[n]))
                uint8_t mod_reg_8  = GET_SAVE_RESTORE_MOD_REG( 8);
                uint8_t mod_reg_9  = GET_SAVE_RESTORE_MOD_REG( 9);
                uint8_t mod_reg_10 = GET_SAVE_RESTORE_MOD_REG(10);
                uint8_t mod_reg_11 = GET_SAVE_RESTORE_MOD_REG(11);
                #undef GET_SAVE_RESTORE_MOD_REG

                uint32_t rank_mask = tmp_dram_dev_num == TWO_RANK ? 0x480E0000 : 0xC80E0000;

                uint8_t emc0_mrw12_byte0 = (mod_reg_8  + ((emc0_training_opt_dq_ob_vref >>  0) & 0xFF));
                uint8_t emc0_mrw12_byte1 = (mod_reg_9  + ((emc0_training_opt_dq_ob_vref >>  8) & 0xFF));
                uint8_t emc0_mrw13_byte0 = (mod_reg_8  + ((emc0_training_opt_dq_ob_vref >> 16) & 0xFF));
                uint8_t emc0_mrw13_byte1 = (mod_reg_9  + ((emc0_training_opt_dq_ob_vref >> 24) & 0xFF));
                uint8_t emc1_mrw12_byte0 = (mod_reg_10 + ((emc1_training_opt_dq_ob_vref >>  0) & 0xFF));
                uint8_t emc1_mrw12_byte1 = (mod_reg_11 + ((emc1_training_opt_dq_ob_vref >>  8) & 0xFF));
                uint8_t emc1_mrw13_byte0 = (mod_reg_10 + ((emc1_training_opt_dq_ob_vref >> 16) & 0xFF));
                uint8_t emc1_mrw13_byte1 = (mod_reg_11 + ((emc1_training_opt_dq_ob_vref >> 24) & 0xFF));

                dst_timing_tables->burst_reg_per_ch.emc0_mrw12 = (emc0_mrw12_byte0 << 0) | (emc0_mrw12_byte1 << 8) | 0x880E0000;
                dst_timing_tables->burst_reg_per_ch.emc1_mrw12 = (emc1_mrw12_byte0 << 0) | (emc1_mrw12_byte1 << 8) | 0x880E0000;
                dst_timing_tables->burst_reg_per_ch.emc1_mrw13 = (emc1_mrw13_byte0 << 0) | (emc1_mrw13_byte1 << 8) | rank_mask;
                dst_timing_tables->burst_reg_per_ch.emc0_mrw13 = (emc0_mrw13_byte0 << 0) | (emc0_mrw13_byte1 << 8) | rank_mask;
            }
        }

        emc_write(emc_dbg_tmp, EMC_DBG);
    }

    /* Step 25:
     *   Program MC updown registers.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: freq_change - step 25\n");

    if (dst_timing_tables->rate_khz > src_timing_tables->rate_khz && !training_enabled_mask) {
        for (int i = 0; i < dst_timing_tables->num_up_down; ++i) {
            mmio_write(dst_timing_tables->la_scale_regs_arr[i], (volatile uint32_t *)(g_la_scale_regs_addr[i]));
        }

        /* Request a timing update. */
        emc_timing_update(dst_emc_fbio_cfg7);
    }

    /* Step 26:
     *   Restore ZCAL registers.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 26\n");

    if (dram_type == DRAM_TYPE_LPDDR4 && !training_enabled_mask) {
        emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

        emc_write(dst_timing_tables->burst_regs.emc_zcal_wait_cnt, EMC_ZCAL_WAIT_CNT);
        emc_write((dst_misc_cfg_2 & 2) ? 0 : dst_timing_tables->burst_regs.emc_zcal_interval, EMC_ZCAL_INTERVAL);

        emc_write(emc_dbg_o, EMC_DBG);
    }

    if (dram_type != DRAM_TYPE_LPDDR4 && opt_cc_short_zcal && opt_zcal_en_cc && !opt_short_zcal) {
        udelay(2);

        if (is_lpddr2) {
            emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

            emc_write(dst_timing_tables->burst_regs.emc_mrs_wait_cnt, EMC_MRS_WAIT_CNT);

            emc_write(emc_dbg_o, EMC_DBG);
        } else if (dram_type == DRAM_TYPE_DDR3) {
            emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

            emc_write(dst_timing_tables->burst_regs.emc_zcal_wait_cnt, EMC_ZCAL_WAIT_CNT);

            emc_write(emc_dbg_o, EMC_DBG);
        }
    }

    if (training_enabled_mask) {
        if (!src_timing_tables->pllm_misc1_0_pllm_clamp_ph90) {
            car_write(car_read(&car->pllm_misc1) | 0x80000000, &car->pllm_misc1);
        }
    } else {
        if (!dst_timing_tables->pllm_misc1_0_pllm_clamp_ph90) {
            car_write(car_read(&car->pllm_misc1) | 0x80000000, &car->pllm_misc1);
        }
    }

    /* Step 27:
     *   Restore EMC_CFG, FDPD registers.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 27\n");

    emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);
    emc_write(dst_timing_tables->burst_regs.emc_cfg, EMC_CFG);
    emc_write(emc_dbg_o, EMC_DBG);

    emc_write(dst_timing_tables->emc_fdpd_ctrl_cmd_no_ramp, EMC_FDPD_CTRL_CMD_NO_RAMP);
    emc_write(dst_timing_tables->emc_sel_dpd_ctrl, EMC_SEL_DPD_CTRL);

    /* Step 28:
     *   Training recover.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 28\n");

    if (training_enabled_mask && dram_type == DRAM_TYPE_LPDDR4) {
        emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

        emc_write(dst_timing_tables->burst_regs.emc_cfg, EMC_CFG);
        emc_write(dst_timing_tables->emc_sel_dpd_ctrl, EMC_SEL_DPD_CTRL);
        emc_write(src_timing_tables->burst_regs.emc_zcal_wait_cnt, EMC_ZCAL_WAIT_CNT);
        emc_write((dst_misc_cfg_2 & 2) ? 0 : dst_timing_tables->burst_regs.emc_zcal_interval, EMC_ZCAL_INTERVAL);
        emc_write(dst_timing_tables->emc_auto_cal_config2, EMC_AUTO_CAL_CONFIG2);
        if (dst_emc_fbio_cfg7 & (EMC_FBIO_CFG7_CH0_ENABLE | EMC_FBIO_CFG7_CH1_ENABLE)) {
            emc0_write(dst_timing_tables->emc_auto_cal_config3, EMC_AUTO_CAL_CONFIG3);
            emc0_write(dst_timing_tables->emc_auto_cal_config4, EMC_AUTO_CAL_CONFIG4);
            emc0_write(dst_timing_tables->emc_auto_cal_config5, EMC_AUTO_CAL_CONFIG5);
            emc0_write(dst_timing_tables->emc_auto_cal_config6, EMC_AUTO_CAL_CONFIG6);
            emc0_write(dst_timing_tables->emc_auto_cal_config7, EMC_AUTO_CAL_CONFIG7);
            emc0_write(dst_timing_tables->emc_auto_cal_config8, EMC_AUTO_CAL_CONFIG8);
        }

        emc_write(emc_dbg_o, EMC_DBG);

        emc_write(dst_timing_tables->burst_regs.emc_tr_dvfs & ~EMC_TR_DVFS_TRAINING_DVFS, EMC_TR_DVFS);
    }

    /* Step 29:
     *   Power fix WAR.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 29\n");

    emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

    emc_write(dst_timing_tables->burst_regs.emc_pmacro_autocal_cfg_common, EMC_PMACRO_AUTOCAL_CFG_COMMON);

    emc_write(emc_dbg_o, EMC_DBG);

    emc_write(EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE0 |
           EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE1 |
           EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE2 |
           EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE3 |
           EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE4 |
           EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE5 |
           EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE6 |
           EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE7,
           EMC_PMACRO_CFG_PM_GLOBAL_0);
    emc_write(EMC_PMACRO_TRAINING_CTRL_0_CH0_TRAINING_E_WRPTR, EMC_PMACRO_TRAINING_CTRL_0);
    emc_write(EMC_PMACRO_TRAINING_CTRL_1_CH1_TRAINING_E_WRPTR, EMC_PMACRO_TRAINING_CTRL_1);
    emc_write(0, EMC_PMACRO_CFG_PM_GLOBAL_0);

    emc_write(emc_set_shadow_bypass(ACTIVE, emc_dbg_o), EMC_DBG);

    /* Fixup xm2comppadctrl */
    if ((dst_misc_cfg_1 & 0x20) == 0) {
        uint32_t xm2comppadctrl = emc_read(EMC_XM2COMPPADCTRL);

        emc_write(xm2comppadctrl & 0x1CFFFFFF, EMC_XM2COMPPADCTRL);
        udelay(1);
        emc_write(xm2comppadctrl & 0x0CFFFFFF, EMC_XM2COMPPADCTRL);
        udelay(1);
        emc_write(xm2comppadctrl & 0x04FFFFFF, EMC_XM2COMPPADCTRL);
        udelay(1);
    }

    emc_write(emc_read(EMC_PMACRO_DLL_CFG_1) | 0x2000, EMC_PMACRO_DLL_CFG_1);
    emc_read(EMC_PMACRO_DLL_CFG_1);
    udelay(2);

    /* Select EMC DLL clock source. */
    {
        car_write(car_read(&car->clk_source_emc_dll) | 0xC00, &car->clk_source_emc_dll);
        car_read(&car->clk_source_emc_dll);
    }

    emc_write(emc_set_shadow_bypass(ASSEMBLY, emc_dbg_o), EMC_DBG);
    emc_read(EMC_DBG);

    /* Step 30:
     *   Re-enable autocal.
     */
    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 30\n");

    if (!training_enabled_mask) {
        if (dst_timing_tables->burst_regs.emc_cfg_dig_dll & 1) {
            uint32_t dig_dll = (emc_read(EMC_CFG_DIG_DLL) & 0xFFFFFFE4) | 9;
            if ((dst_misc_cfg_2 & 1) == 0) {
                dig_dll = (dig_dll & 0xFFFFFF3F) | 0x80;
            }
            emc_write(dig_dll, EMC_CFG_DIG_DLL);

            /* Request a timing update. */
            emc_timing_update(dst_emc_fbio_cfg7);
        }
    }

    if (!(training_enabled_mask && dram_type == DRAM_TYPE_LPDDR4)) {
        emc_write(training_enabled_mask ? src_timing_tables->emc_auto_cal_config : dst_timing_tables->emc_auto_cal_config, EMC_AUTO_CAL_CONFIG);
    }

    if (training_enabled_mask) {
        g_fsp_for_next_freq = !g_fsp_for_next_freq;
    }

    if (src_timing_tables->periodic_training) {
        /* Reset all clock tree values. */
        src_timing_tables->current_dram_clktree_c0d0u0 = src_timing_tables->trained_dram_clktree_c0d0u0;
        src_timing_tables->current_dram_clktree_c0d0u1 = src_timing_tables->trained_dram_clktree_c0d0u1;
        src_timing_tables->current_dram_clktree_c0d1u0 = src_timing_tables->trained_dram_clktree_c0d1u0;
        src_timing_tables->current_dram_clktree_c0d1u1 = src_timing_tables->trained_dram_clktree_c0d1u1;
        src_timing_tables->current_dram_clktree_c1d0u0 = src_timing_tables->trained_dram_clktree_c1d0u0;
        src_timing_tables->current_dram_clktree_c1d0u1 = src_timing_tables->trained_dram_clktree_c1d0u1;
        src_timing_tables->current_dram_clktree_c1d1u0 = src_timing_tables->trained_dram_clktree_c1d1u0;
        src_timing_tables->current_dram_clktree_c1d1u1 = src_timing_tables->trained_dram_clktree_c1d1u1;
    }

    /* Disable pll. */
    pll_disable(dst_clk_src);

    //mdelay(500);
}

static void update_timing_tables(tegra_b01_emc_timing_t *tmp_timing_tables, tegra_b01_emc_timing_t *dst_timing_tables, uint32_t training_param) {
    if (training_param & 0x1) {
        tmp_timing_tables->trim_perch_regs.emc_cmd_brlshft_0 = dst_timing_tables->trim_perch_regs.emc_cmd_brlshft_0;
        tmp_timing_tables->trim_perch_regs.emc_cmd_brlshft_1 = dst_timing_tables->trim_perch_regs.emc_cmd_brlshft_1;
    }
    if (training_param & 0x10) {
        tmp_timing_tables->trim_perch_regs.emc0_data_brlshft_0 = dst_timing_tables->trim_perch_regs.emc0_data_brlshft_0;
        tmp_timing_tables->trim_perch_regs.emc1_data_brlshft_0 = dst_timing_tables->trim_perch_regs.emc1_data_brlshft_0;
        tmp_timing_tables->trim_perch_regs.emc0_data_brlshft_1 = dst_timing_tables->trim_perch_regs.emc0_data_brlshft_1;
        tmp_timing_tables->trim_perch_regs.emc1_data_brlshft_1 = dst_timing_tables->trim_perch_regs.emc1_data_brlshft_1;
    }
    if (training_param & 0x2) {
        tmp_timing_tables->burst_reg_per_ch.emc0_mrw10 = dst_timing_tables->burst_reg_per_ch.emc0_mrw10;
        tmp_timing_tables->burst_reg_per_ch.emc1_mrw10 = dst_timing_tables->burst_reg_per_ch.emc1_mrw10;
        tmp_timing_tables->burst_reg_per_ch.emc0_mrw11 = dst_timing_tables->burst_reg_per_ch.emc0_mrw11;
        tmp_timing_tables->burst_reg_per_ch.emc1_mrw11 = dst_timing_tables->burst_reg_per_ch.emc1_mrw11;
    }
    if (training_param & 0x20) {
        tmp_timing_tables->burst_reg_per_ch.emc0_mrw12 = dst_timing_tables->burst_reg_per_ch.emc0_mrw12;
        tmp_timing_tables->burst_reg_per_ch.emc1_mrw12 = dst_timing_tables->burst_reg_per_ch.emc1_mrw12;
        tmp_timing_tables->burst_reg_per_ch.emc0_mrw13 = dst_timing_tables->burst_reg_per_ch.emc0_mrw13;
        tmp_timing_tables->burst_reg_per_ch.emc1_mrw13 = dst_timing_tables->burst_reg_per_ch.emc1_mrw13;
    }

    if (training_param & 0x1) {
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_4;
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_5;

        if (training_param & 0x200) {
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd0_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd1_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd2_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_cmd3_2;
        }
    }

    if (training_param & 0x40) {
        tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_0;
        tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_1;
        tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_2;
        tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_3 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank0_3;
        tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_0;
        tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_1;
        tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_2;
        tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_3 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_long_dqs_rank1_3;

        if (training_param & 0x200) {
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte0_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte1_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte2_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte3_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte4_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte5_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte6_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank0_byte7_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte0_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte1_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte2_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte3_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte4_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte5_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte6_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_2 = dst_timing_tables->trim_regs.emc_pmacro_ib_ddll_short_dq_rank1_byte7_2;
        }
    }

    if (training_param & 0x80) {
        tmp_timing_tables->trim_regs.emc_pmacro_ib_vref_dq_0 = dst_timing_tables->trim_regs.emc_pmacro_ib_vref_dq_0;
        tmp_timing_tables->trim_regs.emc_pmacro_ib_vref_dq_1 = dst_timing_tables->trim_regs.emc_pmacro_ib_vref_dq_1;
    }

    if (training_param & 0x10) {
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_0;
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_1;
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_2;
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank0_3;
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_0;
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_1;
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_2;
        tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_long_dq_rank1_3;

        if (training_param & 0x200) {
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte0_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte1_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte2_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte3_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte4_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte5_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte6_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank0_byte7_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte0_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte1_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte2_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte3_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte4_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte5_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte6_2;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_0 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_0;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_1 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_1;
            tmp_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_2 = dst_timing_tables->trim_regs.emc_pmacro_ob_ddll_short_dq_rank1_byte7_2;
        }
    }

    tmp_timing_tables->ptfv_list.ptfv_write_samples         = dst_timing_tables->ptfv_list.ptfv_write_samples;
    tmp_timing_tables->ptfv_list.ptfv_dvfs_samples          = dst_timing_tables->ptfv_list.ptfv_dvfs_samples;
    tmp_timing_tables->ptfv_list.ptfv_movavg_weight         = dst_timing_tables->ptfv_list.ptfv_movavg_weight;
    tmp_timing_tables->ptfv_list.ptfv_config_ctrl           = dst_timing_tables->ptfv_list.ptfv_config_ctrl;
    tmp_timing_tables->current_dram_clktree_c0d0u0          = dst_timing_tables->current_dram_clktree_c0d0u0;
    tmp_timing_tables->current_dram_clktree_c0d0u1          = dst_timing_tables->current_dram_clktree_c0d0u1;
    tmp_timing_tables->trained_dram_clktree_c0d0u0          = dst_timing_tables->trained_dram_clktree_c0d0u0;
    tmp_timing_tables->trained_dram_clktree_c0d0u1          = dst_timing_tables->trained_dram_clktree_c0d0u1;
    tmp_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u0  = dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u0;
    tmp_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u1  = dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d0u1;
    tmp_timing_tables->current_dram_clktree_c0d1u0          = dst_timing_tables->current_dram_clktree_c0d1u0;
    tmp_timing_tables->current_dram_clktree_c0d1u1          = dst_timing_tables->current_dram_clktree_c0d1u1;
    tmp_timing_tables->trained_dram_clktree_c0d1u0          = dst_timing_tables->trained_dram_clktree_c0d1u0;
    tmp_timing_tables->trained_dram_clktree_c0d1u1          = dst_timing_tables->trained_dram_clktree_c0d1u1;
    tmp_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u0  = dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u0;
    tmp_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u1  = dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c0d1u1;
    tmp_timing_tables->current_dram_clktree_c1d0u0          = dst_timing_tables->current_dram_clktree_c1d0u0;
    tmp_timing_tables->current_dram_clktree_c1d0u1          = dst_timing_tables->current_dram_clktree_c1d0u1;
    tmp_timing_tables->trained_dram_clktree_c1d0u0          = dst_timing_tables->trained_dram_clktree_c1d0u0;
    tmp_timing_tables->trained_dram_clktree_c1d0u1          = dst_timing_tables->trained_dram_clktree_c1d0u1;
    tmp_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u0  = dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u0;
    tmp_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u1  = dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d0u1;
    tmp_timing_tables->current_dram_clktree_c1d1u0          = dst_timing_tables->current_dram_clktree_c1d1u0;
    tmp_timing_tables->current_dram_clktree_c1d1u1          = dst_timing_tables->current_dram_clktree_c1d1u1;
    tmp_timing_tables->trained_dram_clktree_c1d1u0          = dst_timing_tables->trained_dram_clktree_c1d1u0;
    tmp_timing_tables->trained_dram_clktree_c1d1u1          = dst_timing_tables->trained_dram_clktree_c1d1u1;
    tmp_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u0  = dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u0;
    tmp_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u1  = dst_timing_tables->ptfv_list.ptfv_dqsosc_movavg_c1d1u1;
}

static void train_freq(tegra_b01_emc_timing_t *src_timing_tables, tegra_b01_emc_timing_t *tmp_timing_tables, tegra_b01_emc_timing_t *dst_timing_tables, bool update_clk, uint32_t dst_clk_src) {
    /* Read from MC_EMEM_ADR_CFG */
    uint32_t dram_dev_num = mc_read(MC_EMEM_ADR_CFG);

    /* Write training patterns. */
    if (g_wrote_training_pattern == 0) {
        //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Writing training patterns...\n");

        tegra_b01_ram_pattern_t *ram_patterns = (tegra_b01_ram_pattern_t *)g_ram_pattern_raw_data;

        for (uint32_t i = 0; i < 0x100; ++i) {
            /* Write the DQ data into pattern RAM */
            emc_write(ram_patterns[dst_timing_tables->training_pattern].dq[i], EMC_TRAINING_PATRAM_DQ);

            /* Write the DMI data into pattern RAM */
            emc_write(ram_patterns[dst_timing_tables->training_pattern].dmi[i] & 0xF, EMC_TRAINING_PATRAM_DMI);

            /* Enable writing into pattern RAM and select the offset */
            emc_write(0x80000000 | i, EMC_TRAINING_PATRAM_CTRL);
        }
    }

    emc_write((dst_timing_tables->burst_regs.emc_training_read_ctrl_misc & 0xFFFF0000) | 0x00001000, EMC_TRAINING_QUSE_CTRL_MISC);
    g_wrote_training_pattern = 1;

    if (dst_timing_tables->needs_training && !dst_timing_tables->trained) {
        uint32_t training_flags = dst_timing_tables->needs_training;

        int training_params_idx = 0;
        int training_params[8] = {};

        if (training_flags & 0x03) {
            training_params[training_params_idx++] = training_flags & 0x203;

            if (dram_dev_num & 1) {
                training_params[training_params_idx++] = training_flags & 0x303;
            }
        }

        if (training_flags & 0xF0) {
            training_params[training_params_idx++] = training_flags & 0x2F0;
        }

        for (int i = 0; i < training_params_idx; ++i) {
            /* Change frequency. */
            freq_change(src_timing_tables, dst_timing_tables, training_params[i], dst_clk_src, 0);

            /* Update tmp tables. */
            if (tmp_timing_tables != NULL) {
                update_timing_tables(tmp_timing_tables, dst_timing_tables, training_params[i]);
            }

            /* Change CFG_SWAP to ASSEMBLY_ONLY */
            uint32_t emc_dbg = emc_read(EMC_DBG);
            emc_dbg = ((emc_dbg & 0xF3FFFFFF) | 0x8000000);
            emc_write(emc_dbg, EMC_DBG);

            /* Wait for update. */
            emc_timing_update(src_timing_tables->emc_fbio_cfg7);

            /* Change CFG_SWAP to ACTIVE_ONLY */
            emc_dbg = emc_read(EMC_DBG);
            emc_dbg &= 0xF3FFFFFF;
            emc_write(emc_dbg, EMC_DBG);

            /* Set PMACRO_DLL_CFG_1, preserving MDDLL_SEL_CLK_SRC */
            emc_write((emc_read(EMC_PMACRO_DLL_CFG_1) & 0x00002000) | (src_timing_tables->burst_regs.emc_pmacro_dll_cfg_1 & 0xFFFFDFFF), EMC_PMACRO_DLL_CFG_1);

            /* Update CFG_DIG_DLL */
            uint32_t emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
            emc_cfg_dig_dll = (dst_timing_tables->misc_cfg_2 & 1) ? (emc_cfg_dig_dll & 0xFFFFFFFE) : ((emc_cfg_dig_dll & 0xFFFFFF3E) | 0x80);
            emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);

            /* Wait for update. */
            emc_timing_update(src_timing_tables->emc_fbio_cfg7);

            /* Disable or enable DLL */
            emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
            if (src_timing_tables->burst_regs.emc_cfg_dig_dll & 1) {
                emc_cfg_dig_dll |= 0x01;
            } else {
                emc_cfg_dig_dll &= 0xFFFFFFFE;
            }
            if ((dst_timing_tables->misc_cfg_2 & 1) == 0) {
                emc_cfg_dig_dll = ((emc_cfg_dig_dll & 0xFFFFFF3F) | 0x80);
            }
            emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);

            /* Wait for update. */
            emc_timing_update(src_timing_tables->emc_fbio_cfg7);

            /* Wait for DLL_LOCK to be set */
            wait_for_update(EMC_DIG_DLL_STATUS, EMC_DIG_DLL_STATUS_DLL_LOCK_B01, true, src_timing_tables->emc_fbio_cfg7);

            /* Wait for update. */
            emc_timing_update(src_timing_tables->emc_fbio_cfg7);

            /* Set AUTO_CAL_CONFIG. */
            uint32_t emc_auto_cal_config = emc_read(EMC_AUTO_CAL_CONFIG);
            emc_auto_cal_config = (dst_timing_tables->misc_cfg_2 & 4) ? (emc_auto_cal_config & 0xDFFFF9FF) : ((emc_auto_cal_config & 0x5FFFF9FF) | 0x80000000);
            emc_write(emc_auto_cal_config | 0x20000000, EMC_AUTO_CAL_CONFIG);
        }

        dst_timing_tables->trained = 1;
    }

    if (update_clk) {
        freq_change(src_timing_tables, dst_timing_tables, 0, dst_clk_src, 0);
    }
}

static int dvfs(int z_val, uint32_t next_rate, uint32_t current_rate, tegra_b01_emc_timing_t *timing_tables, int num_timing_tables, TrainMode mode) {
    int current_timing_table_idx = -1, next_timing_table_idx = -1;
    tegra_b01_emc_timing_t *current_timing_table, *next_timing_table;

    /* Locate the right tables for the transition */
    for (int i = 0; i < num_timing_tables; i++) {
        uint32_t rate = timing_tables[i].rate_khz;

        if (rate == current_rate)
            current_timing_table_idx = i;
        if (rate == next_rate)
            next_timing_table_idx = i;
    }

    /* Failed to find the tables. */
    if ((current_timing_table_idx < 0) || (next_timing_table_idx < 0))
        return 4;

    current_timing_table = timing_tables + current_timing_table_idx;
    next_timing_table = timing_tables + next_timing_table_idx;

    uint32_t clk_src_emc_from = current_timing_table->clk_src_emc;
    uint32_t clk_src_emc_to = next_timing_table->clk_src_emc;
    uint32_t rate_from = current_timing_table->rate_khz;
    uint32_t rate_to = next_timing_table->rate_khz;

    //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Changing rate from %d to %d!\n", rate_from, rate_to);

    /* Configure pll. */
    if ((clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) != TEGRA_EMC_SRC_PLLP && (clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) != TEGRA_EMC_SRC_PLLP_UD) {
        if (next_timing_table->emc_fbio_cfg7 & 0x6) {
            if (pll_reprogram(rate_to, clk_src_emc_to, rate_from, clk_src_emc_from) == 1) {
                switch (clk_src_emc_from >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) {
                    case TEGRA_EMC_SRC_PLLMB_UD:
                    case TEGRA_EMC_SRC_PLLMB:
                        g_next_pll = 0;
                        break;
                    case TEGRA_EMC_SRC_PLLC:
                    case TEGRA_EMC_SRC_PLLP:
                    case TEGRA_EMC_SRC_CLKM:
                    case TEGRA_EMC_SRC_PLLP_UD:
                        break;
                    case TEGRA_EMC_SRC_PLLM:
                    case TEGRA_EMC_SRC_PLLM_UD:
                        g_next_pll = !g_next_pll;
                        break;
                }

                clk_src_emc_to = program_pllm(rate_to, 0x9600, clk_src_emc_to, clk_src_emc_to, g_next_pll, next_timing_table);
            } else {
                switch (clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) {
                    case TEGRA_EMC_SRC_PLLM:
                    case TEGRA_EMC_SRC_PLLMB:
                        if (g_next_pll) {
                            clk_src_emc_to = ((clk_src_emc_to & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLMB << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
                        }
                        break;
                    case TEGRA_EMC_SRC_PLLM_UD:
                    case TEGRA_EMC_SRC_PLLMB_UD:
                        if (g_next_pll) {
                            clk_src_emc_to = ((clk_src_emc_to & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLMB_UD << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
                        }
                        break;
                }
            }
        }
    }

    g_active_timing_table_idx = next_timing_table_idx;

    switch (mode) {
        case OP_SWITCH:
            //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Train mode is OP_SWITCH!\n");
            freq_change(current_timing_table, next_timing_table, false, clk_src_emc_to, 0);
            return 0;
        case OP_TRAIN:
            //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Train mode is OP_TRAIN!\n");
            train_freq(current_timing_table, next_timing_table, next_timing_table, false, clk_src_emc_to);
            if ((next_timing_table->emc_fbio_cfg7 & 0x6) == 0) {
                return 0;
            }
            {
                int result = pll_reprogram(next_timing_table->rate_khz, next_timing_table->clk_src_emc, current_timing_table->rate_khz, current_timing_table->clk_src_emc);
                if (result) {
                    g_next_pll = !g_next_pll;
                    result = 0;
                }
                return result;
            }
            return 0;
        case OP_TRAIN_SWITCH:
            //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Train mode is OP_TRAIN_SWITCH!\n");
            train_freq(current_timing_table, next_timing_table, next_timing_table, true, clk_src_emc_to);
            return 0;
        default:
            return 4;
    }
}

void train_dram_mariko(void) {
    volatile tegra_car_t *car = car_get_regs();

    int32_t num_timing_tables = 0;
    tegra_b01_emc_timing_t *timing_tables = get_t210_b01_emc_dvfs_timing_table(&num_timing_tables);
    if (timing_tables == NULL) {
        fatal_error("[MTC]: Missing tables for DRAM id %d!\n", fuse_get_dram_id());
    }

    /* Locate the right timing table. */
    /* TODO: Based on rate, rather than clock source? */
    int boot_index = 0;
    for (boot_index = 0; boot_index < num_timing_tables; boot_index++) {
        //print(SCREEN_LOG_LEVEL_DEBUG, "%d (%d kHz): %s\n", boot_index, timing_tables[boot_index].rate_khz, timing_tables[boot_index].dvfs_ver);
        if (car_read(&car->clk_source_emc) == timing_tables[boot_index].clk_src_emc)
            break;
    }

    if (boot_index >= num_timing_tables) {
        fatal_error("[MTC]: Failed to find timing table!\n");
    }

    if (boot_index != 0) {
        //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: DRAM seems to be already trained, rate >= %dMhz\n", timing_tables[boot_index].rate_khz / 1000);
    } else {
        g_active_timing_table_idx = boot_index;

        /* Switch to upgraded timings. */
        for (int i = boot_index + 1; i < num_timing_tables; ++i) {
            //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Training from %dMhz to %dMhz\n", timing_tables[boot_index].rate_khz / 1000, timing_tables[i].rate_khz / 1000);
            dvfs(0, timing_tables[i].rate_khz, timing_tables[boot_index].rate_khz, timing_tables, num_timing_tables, OP_TRAIN);
            //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Switched\n");
        }

        //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Switching from %dMhz to %dMhz\n", timing_tables[boot_index].rate_khz / 1000, timing_tables[num_timing_tables - 1].rate_khz / 1000);
        dvfs(0, timing_tables[num_timing_tables - 1].rate_khz, timing_tables[boot_index].rate_khz, timing_tables, num_timing_tables, OP_SWITCH);
        //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Switched\n");

        //print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Done!\n");
    }
}
