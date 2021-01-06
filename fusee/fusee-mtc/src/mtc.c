/*
 * Copyright (c) 2015, NVIDIA CORPORATION.  All rights reserved.
 * Copyright (c) 2018 CTCaer  <ctcaer@gmail.com>
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
#include "mtc_tables.h"
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
    
#define TRIM_REG(chan, rank, reg, byte)                 \
    ((EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## reg ##    \
      _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte ## _MASK & \
      next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## \
                 rank ## _ ## reg ## _INDEX]) >>    \
     EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## reg ##     \
     _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte ## _SHIFT)  \
    +                               \
    (((EMC_DATA_BRLSHFT_ ## rank ## _RANK ## rank ## _BYTE ##   \
       byte ## _DATA_BRLSHFT_MASK &                 \
       next_timing->trim_perch_regs[REG_EMC ## chan ##          \
                  _EMC_DATA_BRLSHFT_ ## rank ## _INDEX]) >> \
      EMC_DATA_BRLSHFT_ ## rank ## _RANK ## rank ## _BYTE ##    \
      byte ## _DATA_BRLSHFT_SHIFT) * 64)

#define CALC_TEMP(rank, reg, byte1, byte2, n)               \
    ((new[n] << EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## \
      reg ## _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte1 ## _SHIFT) & \
     EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## reg ##     \
     _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte1 ## _MASK)  \
    |                               \
    ((new[n + 1] << EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## \
      reg ## _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte2 ## _SHIFT) & \
     EMC_PMACRO_OB_DDLL_LONG_DQ_RANK ## rank ## _ ## reg ##     \
     _OB_DDLL_LONG_DQ_RANK ## rank ## _BYTE ## byte2 ## _MASK)  \

/*
 * PTFV defines - basically just indexes into the per table PTFV array.
 */
#define PTFV_DQSOSC_MOVAVG_C0D0U0_INDEX     0
#define PTFV_DQSOSC_MOVAVG_C0D0U1_INDEX     1
#define PTFV_DQSOSC_MOVAVG_C0D1U0_INDEX     2
#define PTFV_DQSOSC_MOVAVG_C0D1U1_INDEX     3
#define PTFV_DQSOSC_MOVAVG_C1D0U0_INDEX     4
#define PTFV_DQSOSC_MOVAVG_C1D0U1_INDEX     5
#define PTFV_DQSOSC_MOVAVG_C1D1U0_INDEX     6
#define PTFV_DQSOSC_MOVAVG_C1D1U1_INDEX     7
#define PTFV_WRITE_SAMPLES_INDEX        8
#define PTFV_DVFS_SAMPLES_INDEX         9
#define PTFV_MOVAVG_WEIGHT_INDEX        10
#define PTFV_CONFIG_CTRL_INDEX          11

#define PTFV_CONFIG_CTRL_USE_PREVIOUS_EMA   (1 << 0)

/*
 * Do arithmetic in fixed point.
 */
#define MOVAVG_PRECISION_FACTOR     100

/*
 * The division portion of the average operation.
 */
#define __AVERAGE_PTFV(dev)                     \
    ({ next_timing->ptfv_list[PTFV_DQSOSC_MOVAVG_ ## dev ## _INDEX] = \
       next_timing->ptfv_list[PTFV_DQSOSC_MOVAVG_ ## dev ## _INDEX] / \
       next_timing->ptfv_list[PTFV_DVFS_SAMPLES_INDEX]; })
       
/*
 * The division portion of the average write operation.
 */
#define __AVERAGE_WRITE_PTFV(dev)                       \
    ({ next_timing->ptfv_list[PTFV_DQSOSC_MOVAVG_ ## dev ## _INDEX] = \
       next_timing->ptfv_list[PTFV_DQSOSC_MOVAVG_ ## dev ## _INDEX] / \
       next_timing->ptfv_list[PTFV_WRITE_SAMPLES_INDEX]; })

/*
 * Convert val to fixed point and add it to the temporary average.
 */
#define __INCREMENT_PTFV(dev, val)                  \
    ({ next_timing->ptfv_list[PTFV_DQSOSC_MOVAVG_ ## dev ## _INDEX] += \
       ((val) * MOVAVG_PRECISION_FACTOR); })

/*
 * Convert a moving average back to integral form and return the value.
 */
#define __MOVAVG_AC(timing, dev)                    \
    ((timing)->ptfv_list[PTFV_DQSOSC_MOVAVG_ ## dev ## _INDEX] /    \
     MOVAVG_PRECISION_FACTOR)

/* Weighted update. */
#define __WEIGHTED_UPDATE_PTFV(dev, nval)               \
    do {                                \
        int w = PTFV_MOVAVG_WEIGHT_INDEX;           \
        int dqs = PTFV_DQSOSC_MOVAVG_ ## dev ## _INDEX;     \
                                    \
        next_timing->ptfv_list[dqs] =               \
            ((nval * MOVAVG_PRECISION_FACTOR) +     \
             (next_timing->ptfv_list[dqs] *         \
              next_timing->ptfv_list[w])) /         \
            (next_timing->ptfv_list[w] + 1);        \
    } while (0)

/* Access a particular average. */
#define __MOVAVG(timing, dev)                      \
    ((timing)->ptfv_list[PTFV_DQSOSC_MOVAVG_ ## dev ## _INDEX])
     
static int g_active_timing_table_idx = -1;
static bool g_is_pllmb = false;
static bool g_fsp_for_next_freq = false;
static bool g_write_training_pattern = true;

#define DEFINE_REG(type, reg) (reg)
static const uint32_t burst_regs_per_ch_off[] = BURST_REGS_PER_CH_LIST;
static const uint32_t burst_regs_off[] = BURST_REGS_LIST;
static const uint32_t trim_regs_per_ch_off[] = TRIM_REGS_PER_CH_LIST;
static const uint32_t trim_regs_off[] = TRIM_REGS_LIST;
static const uint32_t vref_regs_per_ch_off[] = VREF_REGS_PER_CH_LIST;
static const uint32_t training_mod_regs_per_ch_off[] = TRAINING_MOD_REGS_PER_CH_LIST;
static const uint32_t burst_mc_regs_off[] = BURST_MC_REGS_LIST;
static const uint32_t la_scale_regs_off[] = BURST_UP_DOWN_REGS_LIST;
#undef DEFINE_REG

#define DEFINE_REG(type, reg) (type)
static const uint32_t burst_regs_per_ch_type[] = BURST_REGS_PER_CH_LIST;
static const uint32_t trim_regs_per_ch_type[] = TRIM_REGS_PER_CH_LIST;
static const uint32_t vref_regs_per_ch_type[] = VREF_REGS_PER_CH_LIST;
static const uint32_t training_mod_regs_per_ch_type[] = TRAINING_MOD_REGS_PER_CH_LIST;
#undef DEFINE_REG

static const uint32_t g_ram_pattern_dq[0x500] = {
    0x18181818, 0x61616161, 0x85858585, 0x14141414, 0x51515151,
    0x47474747, 0x1E1E1E1E, 0x79797979, 0xE5E5E5E5, 0x94949494,
    0x51515151, 0x46464646, 0x19191919, 0x67676767, 0x9C9C9C9C,
    0x71717171, 0xC5C5C5C5, 0x17171717, 0x5F5F5F5F, 0x7E7E7E7E,
    0xFBFBFBFB, 0xEDEDEDED, 0xB4B4B4B4, 0xD2D2D2D2, 0x48484848,
    0x21212121, 0x85858585, 0x16161616, 0x59595959, 0x66666666,
    0x9A9A9A9A, 0x69696969, 0xA4A4A4A4, 0x93939393, 0x4F4F4F4F,
    0x3F3F3F3F, 0xFCFCFCFC, 0xF3F3F3F3, 0xCDCDCDCD, 0x37373737,
    0xDCDCDCDC, 0x70707070, 0xC3C3C3C3, 0x0F0F0F0F, 0x3E3E3E3E,
    0xFAFAFAFA, 0xEBEBEBEB, 0xACACACAC, 0xB3B3B3B3, 0xCCCCCCCC,
    0x31313131, 0xC5C5C5C5, 0x15151515, 0x57575757, 0x5F5F5F5F,
    0x7F7F7F7F, 0xFDFDFDFD, 0xF4F4F4F4, 0xD0D0D0D0, 0x42424242,
    0x08080808, 0x23232323, 0x8F8F8F8F, 0x3F3F3F3F, 0x18181818,
    0x61616161, 0x85858585, 0x14141414, 0x51515151, 0x47474747,
    0x1E1E1E1E, 0x79797979, 0xE5E5E5E5, 0x94949494, 0x51515151,
    0x46464646, 0x19191919, 0x67676767, 0x9C9C9C9C, 0x71717171,
    0xC5C5C5C5, 0x17171717, 0x5F5F5F5F, 0x7E7E7E7E, 0xFBFBFBFB,
    0xEDEDEDED, 0xB4B4B4B4, 0xD2D2D2D2, 0x48484848, 0x21212121,
    0x85858585, 0x16161616, 0x59595959, 0x66666666, 0x9A9A9A9A,
    0x69696969, 0xA4A4A4A4, 0x93939393, 0x4F4F4F4F, 0x3F3F3F3F,
    0xFCFCFCFC, 0xF3F3F3F3, 0xCDCDCDCD, 0x37373737, 0xDCDCDCDC,
    0x70707070, 0xC3C3C3C3, 0x0F0F0F0F, 0x3E3E3E3E, 0xFAFAFAFA,
    0xEBEBEBEB, 0xACACACAC, 0xB3B3B3B3, 0xCCCCCCCC, 0x31313131,
    0xC5C5C5C5, 0x15151515, 0x57575757, 0x5F5F5F5F, 0x7F7F7F7F,
    0xFDFDFDFD, 0xF4F4F4F4, 0xD0D0D0D0, 0x42424242, 0x08080808,
    0x23232323, 0x8F8F8F8F, 0x3F3F3F3F, 0x06060606, 0x18181818,
    0x21212121, 0x05050505, 0x14141414, 0x11111111, 0x07070707,
    0x1E1E1E1E, 0x39393939, 0x25252525, 0x14141414, 0x11111111,
    0x06060606, 0x19191919, 0x27272727, 0x1C1C1C1C, 0x31313131,
    0x05050505, 0x17171717, 0x1F1F1F1F, 0x3E3E3E3E, 0x3B3B3B3B,
    0x2D2D2D2D, 0x34343434, 0x12121212, 0x08080808, 0x21212121,
    0x05050505, 0x16161616, 0x19191919, 0x26262626, 0x1A1A1A1A,
    0x29292929, 0x24242424, 0x13131313, 0x0F0F0F0F, 0x3F3F3F3F,
    0x3C3C3C3C, 0x33333333, 0x0D0D0D0D, 0x37373737, 0x1C1C1C1C,
    0x30303030, 0x03030303, 0x0F0F0F0F, 0x3E3E3E3E, 0x3A3A3A3A,
    0x2B2B2B2B, 0x2C2C2C2C, 0x33333333, 0x0C0C0C0C, 0x31313131,
    0x05050505, 0x15151515, 0x17171717, 0x1F1F1F1F, 0x3F3F3F3F,
    0x3D3D3D3D, 0x34343434, 0x10101010, 0x02020202, 0x08080808,
    0x23232323, 0x0F0F0F0F, 0x06060606, 0x18181818, 0x21212121,
    0x05050505, 0x14141414, 0x11111111, 0x07070707, 0x1E1E1E1E,
    0x39393939, 0x25252525, 0x14141414, 0x11111111, 0x06060606,
    0x19191919, 0x27272727, 0x1C1C1C1C, 0x31313131, 0x05050505,
    0x17171717, 0x1F1F1F1F, 0x3E3E3E3E, 0x3B3B3B3B, 0x2D2D2D2D,
    0x34343434, 0x12121212, 0x08080808, 0x21212121, 0x05050505,
    0x16161616, 0x19191919, 0x26262626, 0x1A1A1A1A, 0x29292929,
    0x24242424, 0x13131313, 0x0F0F0F0F, 0x3F3F3F3F, 0x3C3C3C3C,
    0x33333333, 0x0D0D0D0D, 0x37373737, 0x1C1C1C1C, 0x30303030,
    0x03030303, 0x0F0F0F0F, 0x3E3E3E3E, 0x3A3A3A3A, 0x2B2B2B2B,
    0x2C2C2C2C, 0x33333333, 0x0C0C0C0C, 0x31313131, 0x05050505,
    0x15151515, 0x17171717, 0x1F1F1F1F, 0x3F3F3F3F, 0x3D3D3D3D,
    0x34343434, 0x10101010, 0x02020202, 0x08080808, 0x23232323,
    0x0F0F0F0F,
    
    0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F,
    0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F,
    0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F,
    0x3F3F3F3F, 0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F,
    0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F, 0x3F3F3F3F,
    0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x00000000, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x3F3F3F3F, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F, 0x3F3F3F3F,
    0x3F3F3F3F, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F, 0x3F3F3F3F,
    0x3F3F3F3F, 0x00000000, 0x00000000, 0x00000000, 0x3F3F3F3F,
    0x00000000,
    
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF,
    0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000,
    0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F,
    0x00000000, 0x3F3F3F3F, 0x00000000, 0x3F3F3F3F, 0x00000000,
    0x3F3F3F3F,
    
    0x80808080, 0x00000000, 0x80808080, 0x00000000, 0x80808080,
    0x00000000, 0x80808080, 0x40404040, 0x00000000, 0x40404040,
    0x00000000, 0x40404040, 0x00000000, 0x40404040, 0x20202020,
    0x00000000, 0x20202020, 0x00000000, 0x20202020, 0x00000000,
    0x20202020, 0x10101010, 0x00000000, 0x10101010, 0x00000000,
    0x10101010, 0x00000000, 0x10101010, 0x08080808, 0x00000000,
    0x08080808, 0x00000000, 0x08080808, 0x00000000, 0x08080808,
    0x04040404, 0x00000000, 0x04040404, 0x00000000, 0x04040404,
    0x00000000, 0x04040404, 0x02020202, 0x00000000, 0x02020202,
    0x00000000, 0x02020202, 0x00000000, 0x02020202, 0x01010101,
    0x00000000, 0x01010101, 0x00000000, 0x01010101, 0x00000000,
    0x01010101, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x80808080,
    0x00000000, 0x80808080, 0x00000000, 0x80808080, 0x00000000,
    0x80808080, 0x40404040, 0x00000000, 0x40404040, 0x00000000,
    0x40404040, 0x00000000, 0x40404040, 0x20202020, 0x00000000,
    0x20202020, 0x00000000, 0x20202020, 0x00000000, 0x20202020,
    0x10101010, 0x00000000, 0x10101010, 0x00000000, 0x10101010,
    0x00000000, 0x10101010, 0x08080808, 0x00000000, 0x08080808,
    0x00000000, 0x08080808, 0x00000000, 0x08080808, 0x04040404,
    0x00000000, 0x04040404, 0x00000000, 0x04040404, 0x00000000,
    0x04040404, 0x02020202, 0x00000000, 0x02020202, 0x00000000,
    0x02020202, 0x00000000, 0x02020202, 0x01010101, 0x00000000,
    0x01010101, 0x00000000, 0x01010101, 0x00000000, 0x01010101,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x20202020,
    0x00000000, 0x20202020, 0x00000000, 0x20202020, 0x00000000,
    0x20202020, 0x00000000, 0x20202020, 0x00000000, 0x10101010,
    0x00000000, 0x10101010, 0x00000000, 0x10101010, 0x00000000,
    0x10101010, 0x00000000, 0x10101010, 0x00000000, 0x08080808,
    0x00000000, 0x08080808, 0x00000000, 0x08080808, 0x00000000,
    0x08080808, 0x00000000, 0x08080808, 0x00000000, 0x04040404,
    0x00000000, 0x04040404, 0x00000000, 0x04040404, 0x00000000,
    0x04040404, 0x00000000, 0x04040404, 0x00000000, 0x02020202,
    0x00000000, 0x02020202, 0x00000000, 0x02020202, 0x00000000,
    0x02020202, 0x00000000, 0x02020202, 0x00000000, 0x01010101,
    0x00000000, 0x01010101, 0x00000000, 0x01010101, 0x00000000,
    0x01010101, 0x00000000, 0x01010101, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x20202020, 0x00000000,
    0x20202020, 0x00000000, 0x20202020, 0x00000000, 0x20202020,
    0x00000000, 0x20202020, 0x00000000, 0x10101010, 0x00000000,
    0x10101010, 0x00000000, 0x10101010, 0x00000000, 0x10101010,
    0x00000000, 0x10101010, 0x00000000, 0x08080808, 0x00000000,
    0x08080808, 0x00000000, 0x08080808, 0x00000000, 0x08080808,
    0x00000000, 0x08080808, 0x00000000, 0x04040404, 0x00000000,
    0x04040404, 0x00000000, 0x04040404, 0x00000000, 0x04040404,
    0x00000000, 0x04040404, 0x00000000, 0x02020202, 0x00000000,
    0x02020202, 0x00000000, 0x02020202, 0x00000000, 0x02020202,
    0x00000000, 0x02020202, 0x00000000, 0x01010101, 0x00000000,
    0x01010101, 0x00000000, 0x01010101, 0x00000000, 0x01010101,
    0x00000000, 0x01010101, 0x00000000, 0x00000000, 0x00000000,
    0x00000000,
    
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333,
    0xAAAAAAAA, 0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA,
    0x55555555, 0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555,
    0xCCCCCCCC, 0x33333333, 0xAAAAAAAA, 0x55555555, 0xCCCCCCCC,
    0x33333333
};

static const uint32_t g_ram_pattern_dmi[0x500] = {    
    0xF, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF,
    0x0, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF,
    0xF, 0xF, 0x0, 0xF, 0xF, 0x0, 0x0, 0x0,
    0xF, 0xF, 0x0, 0xF, 0x0, 0x0, 0xF, 0x0,
    0xF, 0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0x0,
    0x0, 0xF, 0xF, 0x0, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
    0x0, 0x0, 0x0, 0x0, 0xF, 0xF, 0xF, 0x0,
    0xF, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF,
    0x0, 0xF, 0x0, 0xF, 0xF, 0x0, 0xF, 0xF,
    0xF, 0xF, 0x0, 0xF, 0xF, 0x0, 0x0, 0x0,
    0xF, 0xF, 0x0, 0xF, 0x0, 0x0, 0xF, 0x0,
    0xF, 0xF, 0xF, 0x0, 0xF, 0xF, 0xF, 0x0,
    0x0, 0xF, 0xF, 0x0, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF, 0xF,
    0x0, 0x0, 0x0, 0x0, 0xF, 0xF, 0xF, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

    0x0, 0x0, 0xF, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xF, 0xF, 0x0, 0x0, 0x0, 0x0, 0xF, 0x0,
    0xF, 0x0, 0x0, 0x0, 0xF, 0xF, 0xF, 0xF,
    0x0, 0x0, 0xF, 0x0, 0x0, 0x0, 0xF, 0x0,
    0xF, 0xF, 0x0, 0x0, 0xF, 0xF, 0xF, 0x0,
    0xF, 0x0, 0xF, 0x0, 0x0, 0xF, 0xF, 0xF,
    0xF, 0xF, 0x0, 0xF, 0x0, 0x0, 0x0, 0x0,
    0xF, 0xF, 0xF, 0x0, 0x0, 0x0, 0xF, 0x0,
    0x0, 0x0, 0xF, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xF, 0xF, 0x0, 0x0, 0x0, 0x0, 0xF, 0x0,
    0xF, 0x0, 0x0, 0x0, 0xF, 0xF, 0xF, 0xF,
    0x0, 0x0, 0xF, 0x0, 0x0, 0x0, 0xF, 0x0,
    0xF, 0xF, 0x0, 0x0, 0xF, 0xF, 0xF, 0x0,
    0xF, 0x0, 0xF, 0x0, 0x0, 0xF, 0xF, 0xF,
    0xF, 0xF, 0x0, 0xF, 0x0, 0x0, 0x0, 0x0,
    0xF, 0xF, 0xF, 0x0, 0x0, 0x0, 0xF, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0xF, 0x0, 0xF, 0x0, 0xF, 0x0, 0xF, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,

    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3,
    0xA, 0x5, 0xC, 0x3, 0xA, 0x5, 0xC, 0x3
};

/* Determine the current SoC for Mariko specific code. */
static bool is_soc_mariko() {
    return (fuse_get_soc_type() == 1);
}

/* Register read/write helpers. */
static inline void emc_write(uint32_t val, uint32_t offset) {
    MAKE_EMC_REG(offset) = val;
}

static inline uint32_t emc_read(uint32_t offset) {
    return MAKE_EMC_REG(offset);
}

static inline void emc0_write(uint32_t val, uint32_t offset) {
    MAKE_EMC0_REG(offset) = val;
}

static inline uint32_t emc0_read(uint32_t offset) {
    return MAKE_EMC0_REG(offset);
}

static inline void emc1_write(uint32_t val, uint32_t offset) {
    MAKE_EMC1_REG(offset) = val;
}

static inline uint32_t emc1_read(uint32_t offset) {
    return MAKE_EMC1_REG(offset);
}

static inline void emc_write_per_ch(uint32_t val, int type, uint32_t offset) {
    switch (type) {
        case REG_EMC:
            emc_write(val, offset);
            break;
        case REG_EMC0:
            emc0_write(val, offset);
            break;
        case REG_EMC1:
            emc1_write(val, offset);
            break;
    }
}

static inline uint32_t emc_read_per_ch(int type, uint32_t offset) {
    uint32_t val = 0;
    switch (type) {
        case REG_EMC:
            val = emc_read(offset);
            break;
        case REG_EMC0:
            val = emc0_read(offset);
            break;
        case REG_EMC1:
            val = emc1_read(offset);
            break;
    }
    return val;
}

static inline void mc_write(uint32_t val, uint32_t offset) {
    MAKE_MC_REG(offset) = val;
}

static inline uint32_t mc_read(uint32_t offset) {
    return MAKE_MC_REG(offset);
}

/* Configure clock change sequence FIFO */
static void ccfifo_write(uint32_t ccfifo_addr, uint32_t ccfifo_data, uint32_t ccfifo_stall_cnt) {
    MAKE_EMC_REG(EMC_CCFIFO_DATA) = ccfifo_data;
    MAKE_EMC_REG(EMC_CCFIFO_ADDR) = ((ccfifo_addr & 0xFFFF) | ((ccfifo_stall_cnt & 0x7FFF) << 16) | 0x80000000);
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

static void emc_set_shadow_bypass(int set) {
    uint32_t emc_dbg = emc_read(EMC_DBG);

    if (set)
        emc_write(emc_dbg | EMC_DBG_WRITE_MUX_ACTIVE, EMC_DBG);
    else
        emc_write(emc_dbg & ~EMC_DBG_WRITE_MUX_ACTIVE, EMC_DBG);
}

static uint32_t wait_for_update(uint32_t status_reg, uint32_t bit_mask, bool updated_state, int chan) {
    for (int i = 0; i < EMC_STATUS_UPDATE_TIMEOUT; i++) {
        if (chan == REG_EMC) {
            if (((emc_read_per_ch(REG_EMC, status_reg) & bit_mask) != 0) == updated_state)
                return 0;
        } else  {
            if (((emc_read_per_ch(REG_EMC1, status_reg) & bit_mask) != 0) == updated_state)
                return 0;
        }
        
        udelay(1);
    }

    /* Timeout. */
    return 4;
}

static void emc_timing_update(bool dual_chan) {
    /* Trigger the timing update event. */
    emc_write(0x1, EMC_TIMING_CONTROL);
    
    /* Wait for the update to finish. */
    wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_TIMING_UPDATE_STALLED, false, REG_EMC);
    if (dual_chan)
        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_TIMING_UPDATE_STALLED, false, REG_EMC1);
}

static uint32_t get_dll_state(tegra_emc_timing_t *next_timing) {
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

static uint32_t dvfs_power_ramp_down(bool flip_backward, tegra_emc_timing_t* current_timing, tegra_emc_timing_t* next_timing, uint32_t clk) {    
    uint32_t ramp_down_wait = 0;
    uint32_t seq_wait = 0;
    uint32_t pmacro_cmd_pad = 0;
    uint32_t pmacro_dq_pad = 0;
    uint32_t pmacro_cfg5 = 0;
    uint32_t pmacro_rfu1 = 0;
    uint32_t pmacro_common_tx = 0;
  
    if (flip_backward) {
        pmacro_cmd_pad = next_timing->burst_regs[EMC_PMACRO_CMD_PAD_TX_CTRL_INDEX];
        pmacro_dq_pad = next_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX];
        pmacro_rfu1 = next_timing->burst_regs[EMC_PMACRO_BRICK_CTRL_RFU1_INDEX];
        pmacro_cfg5 = next_timing->burst_regs[EMC_FBIO_CFG5_INDEX];
        pmacro_common_tx = next_timing->burst_regs[EMC_PMACRO_COMMON_PAD_TX_CTRL_INDEX];
    } else {
        pmacro_cmd_pad = current_timing->burst_regs[EMC_PMACRO_CMD_PAD_TX_CTRL_INDEX];
        pmacro_dq_pad = ((next_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX] & 0x101) | current_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX]);
        pmacro_rfu1 = current_timing->burst_regs[EMC_PMACRO_BRICK_CTRL_RFU1_INDEX];
        pmacro_cfg5 = current_timing->burst_regs[EMC_FBIO_CFG5_INDEX];
        pmacro_common_tx = current_timing->burst_regs[EMC_PMACRO_COMMON_PAD_TX_CTRL_INDEX];
    }
    
    pmacro_cmd_pad |= EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_DRVFORCEON;
  
    ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, 0);
    ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 | EMC_FBIO_CFG5_CMD_TX_DIS, 12);
    
    ramp_down_wait = clk * 12;
    seq_wait = (100000 / clk) + 1;
    
    if (clk < (1000000 / DVFS_FGCG_HIGH_SPEED_THRESHOLD)) {
        if (clk < (1000000 / IOBRICK_DCC_THRESHOLD)) {
            pmacro_cmd_pad &= ~(EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_CMD_TX_E_DCC);
            pmacro_cmd_pad |= EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSP_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSN_TX_E_DCC;
            
            ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, seq_wait);
            ramp_down_wait += 100000;

            pmacro_dq_pad &= ~(EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_CMD_TX_E_DCC);  
            pmacro_dq_pad |= EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSP_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSN_TX_E_DCC;
            
            ccfifo_write(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x01120112, 0);
        } else {
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x01120112, seq_wait);
            ramp_down_wait += 100000;
        }
        
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x01bf01bf, seq_wait);
        ramp_down_wait += 100000;

        if (clk < (1000000 / IOBRICK_DCC_THRESHOLD)) {
            pmacro_cmd_pad &= ~(EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_CMD_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSP_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSN_TX_E_DCC);
                  
            ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, seq_wait);
            ramp_down_wait += 100000;

            pmacro_dq_pad &= ~(EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_CMD_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSP_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSN_TX_E_DCC);
            
            ccfifo_write(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x07ff07ff, 0);
        } else {
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x07ff07ff, seq_wait);
            ramp_down_wait += 100000;
        }
    } else {
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & ~0x07ff07ff, seq_wait + 19);
        ramp_down_wait += (100000 + (20 * clk));
    }
    
    if (clk < (1000000 / DVFS_FGCG_MID_SPEED_THRESHOLD)) {
        ramp_down_wait += 100000;
        ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & ~0x5, seq_wait);
        ramp_down_wait += 100000;
        ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & ~0xf, seq_wait);
        ramp_down_wait += 100000;
        ccfifo_write(0, 0, seq_wait);
        ramp_down_wait += 100000;
    } else {
        ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & ~0xf, seq_wait);
    }
  
    return ramp_down_wait;
}

static uint32_t dvfs_power_ramp_up(bool flip_backward, tegra_emc_timing_t* current_timing, tegra_emc_timing_t* next_timing, uint32_t training, uint32_t clk) {
    uint32_t ramp_up_wait = 0;
    uint32_t pmacro_cmd_pad = 0;
    uint32_t pmacro_dq_pad = 0;
    uint32_t pmacro_cfg5 = 0;
    uint32_t pmacro_rfu1 = 0;
    uint32_t pmacro_common_tx = 0;
  
    if (flip_backward) {
        pmacro_cmd_pad = current_timing->burst_regs[EMC_PMACRO_CMD_PAD_TX_CTRL_INDEX]; 
        pmacro_dq_pad = current_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX];
        pmacro_rfu1 = current_timing->burst_regs[EMC_PMACRO_BRICK_CTRL_RFU1_INDEX];
        pmacro_cfg5 = current_timing->burst_regs[EMC_FBIO_CFG5_INDEX];
        pmacro_common_tx = current_timing->burst_regs[EMC_PMACRO_COMMON_PAD_TX_CTRL_INDEX];
    } else if (training & 3) {
        pmacro_cmd_pad = next_timing->shadow_regs_ca_train[EMC_PMACRO_CMD_PAD_TX_CTRL_INDEX];
        pmacro_dq_pad = next_timing->shadow_regs_ca_train[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX];
        pmacro_rfu1 = next_timing->shadow_regs_ca_train[EMC_PMACRO_BRICK_CTRL_RFU1_INDEX];
        pmacro_cfg5 = next_timing->shadow_regs_ca_train[EMC_FBIO_CFG5_INDEX];
        pmacro_common_tx = next_timing->shadow_regs_ca_train[EMC_PMACRO_COMMON_PAD_TX_CTRL_INDEX];
    } else if (training & 0xC) {
        pmacro_cmd_pad = next_timing->shadow_regs_quse_train[EMC_PMACRO_CMD_PAD_TX_CTRL_INDEX];
        pmacro_dq_pad = next_timing->shadow_regs_quse_train[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX];
        pmacro_rfu1 = next_timing->shadow_regs_quse_train[EMC_PMACRO_BRICK_CTRL_RFU1_INDEX];
        pmacro_cfg5 = next_timing->shadow_regs_quse_train[EMC_FBIO_CFG5_INDEX];
        pmacro_common_tx = next_timing->shadow_regs_quse_train[EMC_PMACRO_COMMON_PAD_TX_CTRL_INDEX];
    } else if (training & 0xF0) {
        pmacro_cmd_pad = next_timing->shadow_regs_rdwr_train[EMC_PMACRO_CMD_PAD_TX_CTRL_INDEX];
        pmacro_dq_pad = next_timing->shadow_regs_rdwr_train[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX];
        pmacro_rfu1 = next_timing->shadow_regs_rdwr_train[EMC_PMACRO_BRICK_CTRL_RFU1_INDEX];
        pmacro_cfg5 = next_timing->shadow_regs_rdwr_train[EMC_FBIO_CFG5_INDEX];
        pmacro_common_tx = next_timing->shadow_regs_rdwr_train[EMC_PMACRO_COMMON_PAD_TX_CTRL_INDEX];
    } else {
        pmacro_cmd_pad = next_timing->burst_regs[EMC_PMACRO_CMD_PAD_TX_CTRL_INDEX];
        pmacro_dq_pad = next_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX];
        pmacro_rfu1 = next_timing->burst_regs[EMC_PMACRO_BRICK_CTRL_RFU1_INDEX];
        pmacro_cfg5 = next_timing->burst_regs[EMC_FBIO_CFG5_INDEX];
        pmacro_common_tx = next_timing->burst_regs[EMC_PMACRO_COMMON_PAD_TX_CTRL_INDEX];
    }
    
    pmacro_cmd_pad |= EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_DRVFORCEON;
    
    if (clk < 1000000 / DVFS_FGCG_MID_SPEED_THRESHOLD) {
        ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xa, 0);
        ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx & 0xf, (100000 / clk) + 1);
        ramp_up_wait += 100000;
    } else {
        ccfifo_write(EMC_PMACRO_COMMON_PAD_TX_CTRL, pmacro_common_tx | 0x8, 0);
    }
    
    if (clk < 1000000 / DVFS_FGCG_HIGH_SPEED_THRESHOLD) {
        if (clk < 1000000 / IOBRICK_DCC_THRESHOLD) {
            pmacro_cmd_pad |= (EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSP_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSN_TX_E_DCC);
            pmacro_cmd_pad &= ~(EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_CMD_TX_E_DCC);
                  
            ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, (100000 / clk) + 1);
            ramp_up_wait += 100000;

            pmacro_dq_pad |= (EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSP_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSN_TX_E_DCC);
            pmacro_dq_pad &= ~(EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_CMD_TX_E_DCC);
            
            ccfifo_write(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xfe40fe40, 0);
        } else {
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xfe40fe40, (100000 / clk) + 1);
            ramp_up_wait += 100000;
        }
        
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 & 0xfeedfeed, (100000 / clk) + 1);
        ramp_up_wait += 100000;
        
        if (clk < 1000000 / IOBRICK_DCC_THRESHOLD) {
            pmacro_cmd_pad |= (EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSP_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSN_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_E_DCC | EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_CMD_TX_E_DCC);
                
            ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, (100000 / clk) + 1);
            ramp_up_wait += 100000;

            pmacro_dq_pad |= (EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSP_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSN_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_TX_E_DCC | EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_CMD_TX_E_DCC);
                
            ccfifo_write(EMC_PMACRO_DATA_PAD_TX_CTRL, pmacro_dq_pad, 0);
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1, 0);
        } else {
            ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1, (100000 / clk) + 1);
            ramp_up_wait += 100000;
        }
        
        ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 & ~EMC_FBIO_CFG5_CMD_TX_DIS, (100000 / clk) + 10);
        ramp_up_wait += (100000 + (10 * clk));
    } else if (clk < 1000000 / DVFS_FGCG_MID_SPEED_THRESHOLD) {
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 | 0x06000600, (100000 / clk) + 1);
        ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 & ~EMC_FBIO_CFG5_CMD_TX_DIS, (100000 / clk) + 10);
        ramp_up_wait += (100000 + 10 * clk);
    } else {
        ccfifo_write(EMC_PMACRO_BRICK_CTRL_RFU1, pmacro_rfu1 | 0x00000600, 0);
        ccfifo_write(EMC_FBIO_CFG5, pmacro_cfg5 & ~EMC_FBIO_CFG5_CMD_TX_DIS, 12);
        ramp_up_wait += (12 * clk);
    }
    
    pmacro_cmd_pad &= ~EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_DRVFORCEON;
    ccfifo_write(EMC_PMACRO_CMD_PAD_TX_CTRL, pmacro_cmd_pad, 5);
  
    return ramp_up_wait;
}

static uint32_t apply_periodic_compensation_trimmer(tegra_emc_timing_t* next_timing, uint32_t offset) {
    uint32_t temp = 0;
    uint32_t next_timing_rate_mhz = next_timing->rate / 1000;
    int tree_delta[4] = {0};
    int tree_delta_taps[4] = {0};
    int new[] = {
        TRIM_REG(0, 0, 0, 0),
        TRIM_REG(0, 0, 0, 1),
        TRIM_REG(0, 0, 1, 2),
        TRIM_REG(0, 0, 1, 3),

        TRIM_REG(1, 0, 2, 4),
        TRIM_REG(1, 0, 2, 5),
        TRIM_REG(1, 0, 3, 6),
        TRIM_REG(1, 0, 3, 7),

        TRIM_REG(0, 1, 0, 0),
        TRIM_REG(0, 1, 0, 1),
        TRIM_REG(0, 1, 1, 2),
        TRIM_REG(0, 1, 1, 3),

        TRIM_REG(1, 1, 2, 4),
        TRIM_REG(1, 1, 2, 5),
        TRIM_REG(1, 1, 3, 6),
        TRIM_REG(1, 1, 3, 7)
    };

    switch (offset) {
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0:
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1:
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2:
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3:
    case EMC_DATA_BRLSHFT_0:
        tree_delta[0] = 128 * (next_timing->current_dram_clktree_c0d0u0 - next_timing->trained_dram_clktree_c0d0u0);
        tree_delta[1] = 128 * (next_timing->current_dram_clktree_c0d0u1 - next_timing->trained_dram_clktree_c0d0u1);
        tree_delta[2] = 128 * (next_timing->current_dram_clktree_c1d0u0 - next_timing->trained_dram_clktree_c1d0u0);
        tree_delta[3] = 128 * (next_timing->current_dram_clktree_c1d0u1 - next_timing->trained_dram_clktree_c1d0u1);
        tree_delta_taps[0] = (tree_delta[0] * (int)next_timing_rate_mhz) / 1000000;
        tree_delta_taps[1] = (tree_delta[1] * (int)next_timing_rate_mhz) / 1000000;
        tree_delta_taps[2] = (tree_delta[2] * (int)next_timing_rate_mhz) / 1000000;
        tree_delta_taps[3] = (tree_delta[3] * (int)next_timing_rate_mhz) / 1000000;

        for (int i = 0; i < 4; i++) {
            if ((tree_delta_taps[i] > next_timing->tree_margin) ||
                (tree_delta_taps[i] <
                (-1 * next_timing->tree_margin))) {
                new[i * 2] = new[i * 2] + tree_delta_taps[i];
                new[i * 2 + 1] = new[i * 2 + 1] +
                         tree_delta_taps[i];
            }
        }

        if (offset == EMC_DATA_BRLSHFT_0) {
            for (int i = 0; i < 8; i++)
                new[i] = new[i] / 64;
        } else {
            for (int i = 0; i < 8; i++)
                new[i] = new[i] % 64;
        }
        break;

    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0:
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1:
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2:
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3:
    case EMC_DATA_BRLSHFT_1:
        tree_delta[0] = 128 * (next_timing->current_dram_clktree_c0d1u0 - next_timing->trained_dram_clktree_c0d1u0);
        tree_delta[1] = 128 * (next_timing->current_dram_clktree_c0d1u1 - next_timing->trained_dram_clktree_c0d1u1);
        tree_delta[2] = 128 * (next_timing->current_dram_clktree_c1d1u0 - next_timing->trained_dram_clktree_c1d1u0);
        tree_delta[3] = 128 * (next_timing->current_dram_clktree_c1d1u1 - next_timing->trained_dram_clktree_c1d1u1);
        tree_delta_taps[0] = (tree_delta[0] * (int)next_timing_rate_mhz) / 1000000;
        tree_delta_taps[1] = (tree_delta[1] * (int)next_timing_rate_mhz) / 1000000;
        tree_delta_taps[2] = (tree_delta[2] * (int)next_timing_rate_mhz) / 1000000;
        tree_delta_taps[3] = (tree_delta[3] * (int)next_timing_rate_mhz) / 1000000;

        for (int i = 0; i < 4; i++) {
            if ((tree_delta_taps[i] > next_timing->tree_margin) || (tree_delta_taps[i] < (-1 * next_timing->tree_margin))) {
                new[8 + i * 2] = new[8 + i * 2] + tree_delta_taps[i];
                new[8 + i * 2 + 1] = new[8 + i * 2 + 1] + tree_delta_taps[i];
            }
        }

        if (offset == EMC_DATA_BRLSHFT_1) {
            for (int i = 0; i < 8; i++)
                new[i + 8] = new[i + 8] / 64;
        } else {
            for (int i = 0; i < 8; i++)
                new[i + 8] = new[i + 8] % 64;
        }
        break;
    }

    switch (offset) {
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0:
        temp = CALC_TEMP(0, 0, 0, 1, 0);
        break;
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1:
        temp = CALC_TEMP(0, 1, 2, 3, 2);
        break;
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2:
        temp = CALC_TEMP(0, 2, 4, 5, 4);
        break;
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3:
        temp = CALC_TEMP(0, 3, 6, 7, 6);
        break;
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0:
        temp = CALC_TEMP(1, 0, 0, 1, 8);
        break;
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1:
        temp = CALC_TEMP(1, 1, 2, 3, 10);
        break;
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2:
        temp = CALC_TEMP(1, 2, 4, 5, 12);
        break;
    case EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3:
        temp = CALC_TEMP(1, 3, 6, 7, 14);
        break;
    case EMC_DATA_BRLSHFT_0:
        temp = ((new[0] <<
             EMC_DATA_BRLSHFT_0_RANK0_BYTE0_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_0_RANK0_BYTE0_DATA_BRLSHFT_MASK) |
               ((new[1] <<
             EMC_DATA_BRLSHFT_0_RANK0_BYTE1_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_0_RANK0_BYTE1_DATA_BRLSHFT_MASK) |
               ((new[2] <<
             EMC_DATA_BRLSHFT_0_RANK0_BYTE2_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_0_RANK0_BYTE2_DATA_BRLSHFT_MASK) |
               ((new[3] <<
             EMC_DATA_BRLSHFT_0_RANK0_BYTE3_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_0_RANK0_BYTE3_DATA_BRLSHFT_MASK) |
               ((new[4] <<
             EMC_DATA_BRLSHFT_0_RANK0_BYTE4_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_0_RANK0_BYTE4_DATA_BRLSHFT_MASK) |
               ((new[5] <<
             EMC_DATA_BRLSHFT_0_RANK0_BYTE5_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_0_RANK0_BYTE5_DATA_BRLSHFT_MASK) |
               ((new[6] <<
             EMC_DATA_BRLSHFT_0_RANK0_BYTE6_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_0_RANK0_BYTE6_DATA_BRLSHFT_MASK) |
               ((new[7] <<
             EMC_DATA_BRLSHFT_0_RANK0_BYTE7_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_0_RANK0_BYTE7_DATA_BRLSHFT_MASK);
        break;
    case EMC_DATA_BRLSHFT_1:
        temp = ((new[8] <<
             EMC_DATA_BRLSHFT_1_RANK1_BYTE0_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_1_RANK1_BYTE0_DATA_BRLSHFT_MASK) |
               ((new[9] <<
             EMC_DATA_BRLSHFT_1_RANK1_BYTE1_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_1_RANK1_BYTE1_DATA_BRLSHFT_MASK) |
               ((new[10] <<
             EMC_DATA_BRLSHFT_1_RANK1_BYTE2_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_1_RANK1_BYTE2_DATA_BRLSHFT_MASK) |
               ((new[11] <<
             EMC_DATA_BRLSHFT_1_RANK1_BYTE3_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_1_RANK1_BYTE3_DATA_BRLSHFT_MASK) |
               ((new[12] <<
             EMC_DATA_BRLSHFT_1_RANK1_BYTE4_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_1_RANK1_BYTE4_DATA_BRLSHFT_MASK) |
               ((new[13] <<
             EMC_DATA_BRLSHFT_1_RANK1_BYTE5_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_1_RANK1_BYTE5_DATA_BRLSHFT_MASK) |
               ((new[14] <<
             EMC_DATA_BRLSHFT_1_RANK1_BYTE6_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_1_RANK1_BYTE6_DATA_BRLSHFT_MASK) |
               ((new[15] <<
             EMC_DATA_BRLSHFT_1_RANK1_BYTE7_DATA_BRLSHFT_SHIFT) &
             EMC_DATA_BRLSHFT_1_RANK1_BYTE7_DATA_BRLSHFT_MASK);
        break;
    default:
        break;
    }

    return temp;
}

static uint32_t update_clock_tree_delay(tegra_emc_timing_t* current_timing, tegra_emc_timing_t* next_timing, uint32_t dram_dev_num, uint32_t channel_mode, int type) {
    uint32_t mrr_req = 0, mrr_data = 0;
    uint32_t temp0_0 = 0, temp0_1 = 0, temp1_0 = 0, temp1_1 = 0;
    int tdel = 0, tmdel = 0, adel = 0;
    uint32_t cval;
    uint32_t current_timing_rate_mhz = (current_timing->rate / 1000);
    uint32_t next_timing_rate_mhz = (next_timing->rate / 1000);
    bool dvfs_pt1 = (type == DVFS_PT1);
    bool training_pt1 = (type == TRAINING_PT1);
    bool dvfs_update = (type == DVFS_UPDATE);
    bool training_update = (type == TRAINING_UPDATE);
    bool periodic_training_update = (type == PERIODIC_TRAINING_UPDATE);

    /* Dev0 MSB. */
    if (dvfs_pt1 || training_pt1 || periodic_training_update) {
        mrr_req = ((2 << EMC_MRR_DEV_SEL_SHIFT) | (19 << EMC_MRR_MA_SHIFT));
        emc_write(mrr_req, EMC_MRR);

        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, REG_EMC);
        if (channel_mode == DUAL_CHANNEL)
            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, REG_EMC1);

        mrr_data = ((emc_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

        temp0_0 = ((mrr_data & 0xff) << 8);
        temp0_1 = (mrr_data & 0xff00);

        if (channel_mode == DUAL_CHANNEL) {
            mrr_data = ((emc1_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);
            temp1_0 = ((mrr_data & 0xff) << 8);
            temp1_1 = (mrr_data & 0xff00);
        }

        /* Dev0 LSB. */
        mrr_req = ((mrr_req & ~EMC_MRR_MA_MASK) | (18 << EMC_MRR_MA_SHIFT));
        emc_write(mrr_req, EMC_MRR);

        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, REG_EMC);
        if (channel_mode == DUAL_CHANNEL)
            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, REG_EMC1);

        mrr_data = ((emc_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

        temp0_0 |= (mrr_data & 0xff);
        temp0_1 |= ((mrr_data & 0xff00) >> 8);

        if (channel_mode == DUAL_CHANNEL) {
            mrr_data = ((emc1_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);
            temp1_0 |= (mrr_data & 0xff);
            temp1_1 |= ((mrr_data & 0xff00) >> 8);
        }
    }

    cval = ((1000000 * actual_osc_clocks(current_timing->run_clocks)) / (current_timing_rate_mhz * 2 * temp0_0));

    if (dvfs_pt1 || training_pt1)
        __INCREMENT_PTFV(C0D0U0, cval);
    else if (dvfs_update)
        __AVERAGE_PTFV(C0D0U0);
    else if (training_update)
        __AVERAGE_WRITE_PTFV(C0D0U0);
    else if (periodic_training_update)
        __WEIGHTED_UPDATE_PTFV(C0D0U0, cval);

    if (dvfs_update || training_update || periodic_training_update) {
        tdel = (next_timing->current_dram_clktree_c0d0u0 - __MOVAVG_AC(next_timing, C0D0U0));
        tmdel = (tdel < 0) ? -1 * tdel : tdel;
        adel = tmdel;
        if ((tmdel * 128 * next_timing_rate_mhz / 1000000) > next_timing->tree_margin)
            next_timing->current_dram_clktree_c0d0u0 = __MOVAVG_AC(next_timing, C0D0U0);
    }

    cval = ((1000000 * actual_osc_clocks(current_timing->run_clocks)) /
        (current_timing_rate_mhz * 2 * temp0_1));

    if (dvfs_pt1 || training_pt1)
        __INCREMENT_PTFV(C0D0U1, cval);
    else if (dvfs_update)
        __AVERAGE_PTFV(C0D0U1);
    else if (training_update)
        __AVERAGE_WRITE_PTFV(C0D0U1);
    else if (periodic_training_update)
        __WEIGHTED_UPDATE_PTFV(C0D0U1, cval);

    if (dvfs_update || training_update || periodic_training_update) {
        tdel = next_timing->current_dram_clktree_c0d0u1 - __MOVAVG_AC(next_timing, C0D0U1);
        tmdel = (tdel < 0) ? -1 * tdel : tdel;

        if (tmdel > adel)
            adel = tmdel;

        if ((tmdel * 128 * next_timing_rate_mhz / 1000000) > next_timing->tree_margin)
            next_timing->current_dram_clktree_c0d0u1 = __MOVAVG_AC(next_timing, C0D0U1);
    }

    if (channel_mode == DUAL_CHANNEL) {
        cval = ((1000000 * actual_osc_clocks(current_timing->run_clocks)) / (current_timing_rate_mhz * 2 * temp1_0));
        
        if (dvfs_pt1 || training_pt1)
            __INCREMENT_PTFV(C1D0U0, cval);
        else if (dvfs_update)
            __AVERAGE_PTFV(C1D0U0);
        else if (training_update)
            __AVERAGE_WRITE_PTFV(C1D0U0);
        else if (periodic_training_update)
            __WEIGHTED_UPDATE_PTFV(C1D0U0, cval);

        if (dvfs_update || training_update || periodic_training_update) {
            tdel = next_timing->current_dram_clktree_c1d0u0 - __MOVAVG_AC(next_timing, C1D0U0);
            tmdel = (tdel < 0) ? -1 * tdel : tdel;

            if (tmdel > adel)
                adel = tmdel;

            if ((tmdel * 128 * next_timing_rate_mhz / 1000000) > next_timing->tree_margin)
                next_timing->current_dram_clktree_c1d0u0 = __MOVAVG_AC(next_timing, C1D0U0);
        }

        cval = ((1000000 * actual_osc_clocks(current_timing->run_clocks)) / (current_timing_rate_mhz * 2 * temp1_1));
        
        if (dvfs_pt1 || training_pt1)
            __INCREMENT_PTFV(C1D0U1, cval);
        else if (dvfs_update)
            __AVERAGE_PTFV(C1D0U1);
        else if (training_update)
            __AVERAGE_WRITE_PTFV(C1D0U1);
        else if (periodic_training_update)
            __WEIGHTED_UPDATE_PTFV(C1D0U1, cval);

        if (dvfs_update || training_update || periodic_training_update) {
            tdel = next_timing->current_dram_clktree_c1d0u1 - __MOVAVG_AC(next_timing, C1D0U1);
            tmdel = (tdel < 0) ? -1 * tdel : tdel;

            if (tmdel > adel)
                adel = tmdel;

            if ((tmdel * 128 * next_timing_rate_mhz / 1000000) > next_timing->tree_margin)
                next_timing->current_dram_clktree_c1d0u1 = __MOVAVG_AC(next_timing, C1D0U1);
        }
    }

    if (dram_dev_num != TWO_RANK)
        return adel;

    /* Dev1 MSB. */
    if (dvfs_pt1 || training_pt1 || periodic_training_update) {
        mrr_req = ((1 << EMC_MRR_DEV_SEL_SHIFT) | (19 << EMC_MRR_MA_SHIFT));
        emc_write(mrr_req, EMC_MRR);

        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, REG_EMC);
        if (channel_mode == DUAL_CHANNEL)
            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, REG_EMC1);

        mrr_data = ((emc_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

        temp0_0 = ((mrr_data & 0xff) << 8);
        temp0_1 = (mrr_data & 0xff00);

        if (channel_mode == DUAL_CHANNEL) {
            mrr_data = ((emc1_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);
            temp1_0 = ((mrr_data & 0xff) << 8);
            temp1_1 = (mrr_data & 0xff00);
        }

        /* Dev1 LSB. */
        mrr_req = ((mrr_req & ~EMC_MRR_MA_MASK) | (18 << EMC_MRR_MA_SHIFT));
        emc_write(mrr_req, EMC_MRR);

        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, REG_EMC);
        if (channel_mode == DUAL_CHANNEL)
            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_MRR_DIVLD, true, REG_EMC1);

        mrr_data = ((emc_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);

        temp0_0 |= (mrr_data & 0xff);
        temp0_1 |= ((mrr_data & 0xff00) >> 8);

        if (channel_mode == DUAL_CHANNEL) {
            mrr_data = ((emc1_read(EMC_MRR) & EMC_MRR_DATA_MASK) << EMC_MRR_DATA_SHIFT);
            temp1_0 |= (mrr_data & 0xff);
            temp1_1 |= ((mrr_data & 0xff00) >> 8);
        }
    }

    cval = ((1000000 * actual_osc_clocks(current_timing->run_clocks)) / (current_timing_rate_mhz * 2 * temp0_0));

    if (dvfs_pt1 || training_pt1)
        __INCREMENT_PTFV(C0D1U0, cval);
    else if (dvfs_update)
        __AVERAGE_PTFV(C0D1U0);
    else if (training_update)
        __AVERAGE_WRITE_PTFV(C0D1U0);
    else if (periodic_training_update)
        __WEIGHTED_UPDATE_PTFV(C0D1U0, cval);

    if (dvfs_update || training_update || periodic_training_update) {
        tdel = next_timing->current_dram_clktree_c0d1u0 - __MOVAVG_AC(next_timing, C0D1U0);
        tmdel = (tdel < 0) ? -1 * tdel : tdel;
        
        if (tmdel > adel)
            adel = tmdel;

        if ((tmdel * 128 * next_timing_rate_mhz / 1000000) > next_timing->tree_margin)
            next_timing->current_dram_clktree_c0d1u0 = __MOVAVG_AC(next_timing, C0D1U0);
    }

    cval = ((1000000 * actual_osc_clocks(current_timing->run_clocks)) / (current_timing_rate_mhz * 2 * temp0_1));

    if (dvfs_pt1 || training_pt1)
        __INCREMENT_PTFV(C0D1U1, cval);
    else if (dvfs_update)
        __AVERAGE_PTFV(C0D1U1);
    else if (training_update)
        __AVERAGE_WRITE_PTFV(C0D1U1);
    else if (periodic_training_update)
        __WEIGHTED_UPDATE_PTFV(C0D1U1, cval);

    if (dvfs_update || training_update || periodic_training_update) {
        tdel = next_timing->current_dram_clktree_c0d1u1 - __MOVAVG_AC(next_timing, C0D1U1);
        tmdel = (tdel < 0) ? -1 * tdel : tdel;
        
        if (tmdel > adel)
            adel = tmdel;

        if ((tmdel * 128 * next_timing_rate_mhz / 1000000) > next_timing->tree_margin)
            next_timing->current_dram_clktree_c0d1u1 = __MOVAVG_AC(next_timing, C0D1U1);
    }

    if (channel_mode == DUAL_CHANNEL) {
        cval = ((1000000 * actual_osc_clocks(current_timing->run_clocks)) / (current_timing_rate_mhz * 2 * temp1_0));

        if (dvfs_pt1 || training_pt1)
            __INCREMENT_PTFV(C1D1U0, cval);
        else if (dvfs_update)
            __AVERAGE_PTFV(C1D1U0);
        else if (training_update)
            __AVERAGE_WRITE_PTFV(C1D1U0);
        else if (periodic_training_update)
            __WEIGHTED_UPDATE_PTFV(C1D1U0, cval);

        if (dvfs_update || training_update || periodic_training_update) {
            tdel = next_timing->current_dram_clktree_c1d1u0 - __MOVAVG_AC(next_timing, C1D1U0);
            
            tmdel = (tdel < 0) ? -1 * tdel : tdel;
            
            if (tmdel > adel)
                adel = tmdel;

            if ((tmdel * 128 * next_timing_rate_mhz / 1000000) > next_timing->tree_margin)
                next_timing->current_dram_clktree_c1d1u0 = __MOVAVG_AC(next_timing, C1D1U0);
        }

        cval = ((1000000 * actual_osc_clocks(current_timing->run_clocks)) / (current_timing_rate_mhz * 2 * temp1_1));

        if (dvfs_pt1 || training_pt1)
            __INCREMENT_PTFV(C1D1U1, cval);
        else if (dvfs_update)
            __AVERAGE_PTFV(C1D1U1);
        else if (training_update)
            __AVERAGE_WRITE_PTFV(C1D1U1);
        else if (periodic_training_update)
            __WEIGHTED_UPDATE_PTFV(C1D1U1, cval);

        if (dvfs_update || training_update || periodic_training_update) {
            tdel = next_timing->current_dram_clktree_c1d1u1 - __MOVAVG_AC(next_timing, C1D1U1);
            tmdel = (tdel < 0) ? -1 * tdel : tdel;
            
            if (tmdel > adel)
                adel = tmdel;

            if ((tmdel * 128 * next_timing_rate_mhz / 1000000) > next_timing->tree_margin)
                next_timing->current_dram_clktree_c1d1u1 = __MOVAVG_AC(next_timing, C1D1U1);
        }
    }
    
    if (training_update) {
        next_timing->trained_dram_clktree_c0d0u0 = next_timing->current_dram_clktree_c0d0u0;
        next_timing->trained_dram_clktree_c0d0u1 = next_timing->current_dram_clktree_c0d0u1;
        next_timing->trained_dram_clktree_c0d1u0 = next_timing->current_dram_clktree_c0d1u0;
        next_timing->trained_dram_clktree_c0d1u1 = next_timing->current_dram_clktree_c0d1u1;
        next_timing->trained_dram_clktree_c1d0u0 = next_timing->current_dram_clktree_c1d0u0;
        next_timing->trained_dram_clktree_c1d0u1 = next_timing->current_dram_clktree_c1d0u1;
        next_timing->trained_dram_clktree_c1d1u0 = next_timing->current_dram_clktree_c1d1u0;
        next_timing->trained_dram_clktree_c1d1u1 = next_timing->current_dram_clktree_c1d1u1;
    }

    return adel;
}

static void reset_dram_clktree_values(tegra_emc_timing_t *table) {
    #define __RESET_CLKTREE(TBL, C, D, U)               \
    TBL->current_dram_clktree_c ## C ## d ## D ## u ## U =      \
    TBL->trained_dram_clktree_c ## C ## d ## D ## u ## U

    __RESET_CLKTREE(table, 0, 0, 0);
    __RESET_CLKTREE(table, 0, 0, 1);
    __RESET_CLKTREE(table, 1, 0, 0);
    __RESET_CLKTREE(table, 1, 0, 1);
    __RESET_CLKTREE(table, 1, 1, 0);
    __RESET_CLKTREE(table, 1, 1, 1);
}

static uint32_t periodic_compensation_handler(tegra_emc_timing_t *current_timing, tegra_emc_timing_t *next_timing, uint32_t dram_dev_num, uint32_t channel_mode, int type) {
#define __COPY_EMA(nt, lt, dev)                     \
    ({ __MOVAVG(nt, dev) = __MOVAVG(lt, dev) *          \
       (nt)->ptfv_list[PTFV_DVFS_SAMPLES_INDEX]; })

    uint32_t adel = 0;
    uint32_t samples = next_timing->ptfv_list[PTFV_DVFS_SAMPLES_INDEX];
    uint32_t samples_write = next_timing->ptfv_list[PTFV_WRITE_SAMPLES_INDEX];
    uint32_t delay = 2 + (1000 * actual_osc_clocks(current_timing->run_clocks) / current_timing->rate);

    if (!next_timing->periodic_training)
        return 0;

    if (type == DVFS_SEQUENCE) {
        if (current_timing->periodic_training && (next_timing->ptfv_list[PTFV_CONFIG_CTRL_INDEX] & PTFV_CONFIG_CTRL_USE_PREVIOUS_EMA)) {
            /*
             * If the previous frequency was using periodic
             * calibration then we can reuse the previous
             * frequencies EMA data.
             */
            __COPY_EMA(next_timing, current_timing, C0D0U0);
            __COPY_EMA(next_timing, current_timing, C0D0U1);
            __COPY_EMA(next_timing, current_timing, C1D0U0);
            __COPY_EMA(next_timing, current_timing, C1D0U1);
            __COPY_EMA(next_timing, current_timing, C0D1U0);
            __COPY_EMA(next_timing, current_timing, C0D1U1);
            __COPY_EMA(next_timing, current_timing, C1D1U0);
            __COPY_EMA(next_timing, current_timing, C1D1U1);
        } else {
            /* Reset the EMA.*/
            __MOVAVG(next_timing, C0D0U0) = 0;
            __MOVAVG(next_timing, C0D0U1) = 0;
            __MOVAVG(next_timing, C1D0U0) = 0;
            __MOVAVG(next_timing, C1D0U1) = 0;
            __MOVAVG(next_timing, C0D1U0) = 0;
            __MOVAVG(next_timing, C0D1U1) = 0;
            __MOVAVG(next_timing, C1D1U0) = 0;
            __MOVAVG(next_timing, C1D1U1) = 0;

            for (int i = 0; i < samples; i++) {
                start_periodic_compensation();
                udelay(delay);

                /* Generate next sample of data. */
                adel = update_clock_tree_delay(current_timing, next_timing, dram_dev_num, channel_mode, DVFS_PT1);
            }
        }

        adel = update_clock_tree_delay(current_timing, next_timing, dram_dev_num, channel_mode, DVFS_UPDATE);
    } else if (type == WRITE_TRAINING_SEQUENCE) {
        /* Reset the EMA.*/
        __MOVAVG(next_timing, C0D0U0) = 0;
        __MOVAVG(next_timing, C0D0U1) = 0;
        __MOVAVG(next_timing, C1D0U0) = 0;
        __MOVAVG(next_timing, C1D0U1) = 0;
        __MOVAVG(next_timing, C0D1U0) = 0;
        __MOVAVG(next_timing, C0D1U1) = 0;
        __MOVAVG(next_timing, C1D1U0) = 0;
        __MOVAVG(next_timing, C1D1U1) = 0;
        
        for (int i = 0; i < samples_write; i++) {
            start_periodic_compensation();
            udelay(delay);
            
            /* Generate next sample of data. */
            update_clock_tree_delay(current_timing, next_timing, dram_dev_num, channel_mode, TRAINING_PT1);
        }
        
        adel = update_clock_tree_delay(current_timing, next_timing, dram_dev_num, channel_mode, TRAINING_UPDATE);
    } else if (type == PERIODIC_TRAINING_SEQUENCE) {
        start_periodic_compensation();
        udelay(delay);

        adel = update_clock_tree_delay(current_timing, next_timing, dram_dev_num, channel_mode, PERIODIC_TRAINING_UPDATE);
    }

    return adel;
}

static void set_over_temp_timing(tegra_emc_timing_t *next_timing, unsigned long state) {
#define REFRESH_X2      1
#define REFRESH_X4      2
#define REFRESH_SPEEDUP(val, speedup)                   \
        (val = ((val) & 0xFFFF0000) | (((val) & 0xFFFF) >> (speedup)))

    uint32_t ref = next_timing->burst_regs[EMC_REFRESH_INDEX];
    uint32_t pre_ref = next_timing->burst_regs[EMC_PRE_REFRESH_REQ_CNT_INDEX];
    uint32_t dsr_cntrl = next_timing->burst_regs[EMC_DYN_SELF_REF_CONTROL_INDEX];

    switch (state) {
        case TEGRA_DRAM_OVER_TEMP_NONE:
        case TEGRA_DRAM_OVER_TEMP_THROTTLE:
            break;
        case TEGRA_DRAM_OVER_TEMP_REFRESH_X2:
            REFRESH_SPEEDUP(ref, REFRESH_X2);
            REFRESH_SPEEDUP(pre_ref, REFRESH_X2);
            REFRESH_SPEEDUP(dsr_cntrl, REFRESH_X2);
            break;
        case TEGRA_DRAM_OVER_TEMP_REFRESH_X4:
            REFRESH_SPEEDUP(ref, REFRESH_X4);
            REFRESH_SPEEDUP(pre_ref, REFRESH_X4);
            REFRESH_SPEEDUP(dsr_cntrl, REFRESH_X4);
            break;
        default:
            return;
    }

    emc_write(ref, burst_regs_off[EMC_REFRESH_INDEX]);
    emc_write(pre_ref, burst_regs_off[EMC_PRE_REFRESH_REQ_CNT_INDEX]);
    emc_write(dsr_cntrl, burst_regs_off[EMC_DYN_SELF_REF_CONTROL_INDEX]);
}

static void change_dll_src(tegra_emc_timing_t* next_timing, uint32_t clk_src_emc_to) {
    volatile tegra_car_t *car = car_get_regs();
    
    uint32_t emc_2x_clk_src_to = (clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT);
    uint32_t val = (((((next_timing->dll_clk_src & 0x1FFFFFFF) | (emc_2x_clk_src_to << EMC_CLK_EMC_2X_CLK_SRC_SHIFT)) & 0xFFFFFF00) | (clk_src_emc_to & 0xFF)) & 0xFFFFF3FF);
    
    /* Clock source is PLLMB_UD */
    if (emc_2x_clk_src_to == TEGRA_EMC_SRC_PLLMB_UD)
        val |= 0x400;
    else if (emc_2x_clk_src_to != TEGRA_EMC_SRC_PLLM_UD)  /* Clock source is not PLLM_UD */
        val |= 0x800;
    
    /* Set EMC_DLL_CLK_SRC, DDLL_CLK_SEL and EMC_DLL_CLK_DIVISOR */
    car->clk_source_emc_dll = val;
    
    /* Clear and set CLK_ENB_EMC_DLL */
    uint32_t clk_enb_emc_dll = ((car->clk_out_enb_x & 0xFFFFBFFF) | ((next_timing->clk_out_enb_x_0_clk_enb_emc_dll & 1) << 14));
    car->clk_out_enb_x = clk_enb_emc_dll;
}

static uint32_t dll_prelock(tegra_emc_timing_t* next_timing, bool dvfs_with_training, uint32_t clk_src_emc_to) {
    /* Check for dual channel LPDDR4 */
    bool dual_channel_lpddr4_case = ((emc_read(EMC_FBIO_CFG7) & EMC_FBIO_CFG7_CH0_ENABLE) & (emc_read(EMC_FBIO_CFG7) & EMC_FBIO_CFG7_CH1_ENABLE));

    uint32_t emc_dig_dll_status = 0;
    uint32_t emc_cfg_dig_dll = (emc_read(EMC_CFG_DIG_DLL) & ~EMC_CFG_DIG_DLL_CFG_DLL_LOCK_LIMIT_MASK);
              
    emc_cfg_dig_dll |= (3 << EMC_CFG_DIG_DLL_CFG_DLL_LOCK_LIMIT_SHIFT);
    emc_cfg_dig_dll &= ~EMC_CFG_DIG_DLL_CFG_DLL_EN;
    emc_cfg_dig_dll &= ~EMC_CFG_DIG_DLL_CFG_DLL_MODE_MASK;
    emc_cfg_dig_dll |= (3 << EMC_CFG_DIG_DLL_CFG_DLL_MODE_SHIFT);
    emc_cfg_dig_dll |= EMC_CFG_DIG_DLL_CFG_DLL_STALL_ALL_TRAFFIC;
    emc_cfg_dig_dll &= ~EMC_CFG_DIG_DLL_CFG_DLL_STALL_RW_UNTIL_LOCK;
    emc_cfg_dig_dll &= ~EMC_CFG_DIG_DLL_CFG_DLL_STALL_ALL_UNTIL_LOCK;
    
    /* Update EMC_CFG_DIG_DLL_0 */
    emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);
    
    /* Request a timing update event */
    emc_timing_update(dual_channel_lpddr4_case);
    
    /* Wait until CFG_DLL_EN is cleared for EMC */
    do {
        emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    } while (emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN);
    
    /* Wait until CFG_DLL_EN is cleared for EMC1 */
    if (dual_channel_lpddr4_case) {
        do {
            emc_cfg_dig_dll = emc1_read(EMC_CFG_DIG_DLL);
        } while (emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN);
    }
    
    /* Manual configuration (superseded). */
    /*
    uint32_t emc_dll_cfg_0 = emc_read(EMC_DLL_CFG_0);
    emc_dll_cfg_0 &= 0xDF00000F;
    emc_dll_cfg_0 |= 0x1FA340AF;
    emc_write(emc_dll_cfg_0, EMC_DLL_CFG_0);
    */
    
    emc_write(next_timing->burst_regs[EMC_DLL_CFG_0_INDEX], EMC_DLL_CFG_0);
    emc_write(next_timing->burst_regs[EMC_DLL_CFG_1_INDEX], EMC_DLL_CFG_1);
    
    /* Manual configuration (superseded). */
    /*
    uint32_t ddllcal_ctrl_start_trim_val = 0;
    
    if ((next_timing->rate >= 400000) && (next_timing->rate < 600000))
        ddllcal_ctrl_start_trim_val = 150;
    else if ((next_timing->rate >= 600000) && (next_timing->rate < 800000))
        ddllcal_ctrl_start_trim_val = 100;
    else if ((next_timing->rate >= 800000) && (next_timing->rate < 1000000))
        ddllcal_ctrl_start_trim_val = 70;
    else if ((next_timing->rate >= 1000000) && (next_timing->rate < 1200000))
        ddllcal_ctrl_start_trim_val = 30;
    else
        ddllcal_ctrl_start_trim_val = 20;

    uint32_t emc_dll_cfg_1 = emc_read(EMC_DLL_CFG_1);
    emc_dll_cfg_1 &= EMC_DLL_CFG_1_DDLLCAL_CTRL_START_TRIM_MASK;
    emc_dll_cfg_1 |= ddllcal_ctrl_start_trim_val;
    emc_write(emc_dll_cfg_1, EMC_DLL_CFG_1);
    */
    
    /* Configure the clock and reset controller for EMC DLL */
    change_dll_src(next_timing, clk_src_emc_to);
    
    /* Enable DLL */
    emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    emc_cfg_dig_dll |= EMC_CFG_DIG_DLL_CFG_DLL_EN;
    emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);
    
    /* Request a timing update event */
    emc_timing_update(dual_channel_lpddr4_case);
    
    /* Wait until CFG_DLL_EN is set for EMC */
    do {
        emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    } while (!(emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN));
    
    /* Wait until CFG_DLL_EN is set for EMC1 */
    if (dual_channel_lpddr4_case) {
        do {
            emc_cfg_dig_dll = emc1_read(EMC_CFG_DIG_DLL);
        } while (!(emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN));
    }
    
    /* Wait until DLL_PRIV_UPDATED or DLL_LOCK have been cleared */
    do {
        emc_dig_dll_status = emc_read(EMC_DIG_DLL_STATUS);
    } while (!(emc_dig_dll_status & EMC_DIG_DLL_STATUS_DLL_LOCK) || !(emc_dig_dll_status & EMC_DIG_DLL_STATUS_DLL_PRIV_UPDATED));
    
    if (dvfs_with_training) {
        /* Set WRITE_MUX to ACTIVE */
        emc_set_shadow_bypass(ACTIVE);
        
        /* Disable DLL */
        emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
        emc_cfg_dig_dll &= ~EMC_CFG_DIG_DLL_CFG_DLL_EN;
        emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);
        
        /* Set WRITE_MUX to ASSEMBLY */
        emc_set_shadow_bypass(ASSEMBLY);
        
        /* Wait until CFG_DLL_EN is cleared for EMC */
        do {
            emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
        } while (emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN);
        
        /* Wait until CFG_DLL_EN is cleared for EMC1 */
        if (dual_channel_lpddr4_case) {
            do {
                emc_cfg_dig_dll = emc1_read(EMC_CFG_DIG_DLL);
            } while (emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN);
        }
    }
    
    /* Return the DLL_OUT value */
    return (emc_read(EMC_DIG_DLL_STATUS) & EMC_DIG_DLL_STATUS_DLL_OUT_MASK);
}

static void dll_disable(bool dual_channel_lpddr4_case) {
    /* Disable DLL */
    uint32_t emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    emc_cfg_dig_dll &= ~EMC_CFG_DIG_DLL_CFG_DLL_EN;
    emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);
    
    /* Request a timing update event */
    emc_timing_update(dual_channel_lpddr4_case);
    
    /* Wait until CFG_DLL_EN is cleared for EMC */
    do {
        emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    } while (emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN);
    
    /* Wait until CFG_DLL_EN is cleared for EMC1 */
    if (dual_channel_lpddr4_case) {
        do {
            emc_cfg_dig_dll = emc1_read(EMC_CFG_DIG_DLL);
        } while (emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN);
    }
}

static void dll_enable(bool dual_channel_lpddr4_case) {
    /* Enable DLL */
    uint32_t emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    emc_cfg_dig_dll |= EMC_CFG_DIG_DLL_CFG_DLL_EN;
    emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);
    
    /* Request a timing update event */
    emc_timing_update(dual_channel_lpddr4_case);
    
    /* Wait until CFG_DLL_EN is set for EMC */
    do {
        emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    } while (!(emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN));
    
    /* Wait until CFG_DLL_EN is set for EMC1 */
    if (dual_channel_lpddr4_case) {
        do {
            emc_cfg_dig_dll = emc1_read(EMC_CFG_DIG_DLL);
        } while (!(emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN));
    }
}

static void dll_enable_stall(bool dual_channel_lpddr4_case) {
    /* Enable DLL */
    uint32_t emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    emc_cfg_dig_dll |=  EMC_CFG_DIG_DLL_CFG_DLL_STALL_ALL_TRAFFIC;
    emc_cfg_dig_dll |=  EMC_CFG_DIG_DLL_CFG_DLL_EN;
    emc_cfg_dig_dll &= ~EMC_CFG_DIG_DLL_CFG_DLL_STALL_RW_UNTIL_LOCK;
    emc_cfg_dig_dll &= ~EMC_CFG_DIG_DLL_CFG_DLL_STALL_ALL_UNTIL_LOCK;
    emc_cfg_dig_dll =  (emc_cfg_dig_dll & ~EMC_CFG_DIG_DLL_CFG_DLL_MODE_MASK) | (2 << EMC_CFG_DIG_DLL_CFG_DLL_MODE_SHIFT);
    emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);
    
    /* Request a timing update event */
    emc_timing_update(dual_channel_lpddr4_case);
    
    /* Wait until CFG_DLL_EN is set for EMC */
    do {
        emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
    } while (!(emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN));
    
    /* Wait until CFG_DLL_EN is set for EMC1 */
    if (dual_channel_lpddr4_case) {
        do {
            emc_cfg_dig_dll = emc1_read(EMC_CFG_DIG_DLL);
        } while (!(emc_cfg_dig_dll & EMC_CFG_DIG_DLL_CFG_DLL_EN));
    }
}

static bool test_clk_ratio(uint32_t rate_to, uint32_t clk_src_emc_to, uint32_t rate_from, uint32_t clk_src_emc_from) {
    volatile tegra_car_t *car = car_get_regs();
    
    uint32_t emc_2x_clk_src = (car->clk_source_emc >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT);
    uint32_t post_div = 0;
  
    if ((emc_2x_clk_src == TEGRA_EMC_SRC_PLLM) || (emc_2x_clk_src == TEGRA_EMC_SRC_PLLM_UD)) {
        post_div = ((car->pllm_base >> 0x14) & 0x1F);
    } else if ((emc_2x_clk_src == TEGRA_EMC_SRC_PLLMB_UD) || (emc_2x_clk_src == TEGRA_EMC_SRC_PLLMB)) {
        post_div = ((car->pllmb_base >> 0x14) & 0x1F);
    }
  
    /* Bad post divider value */
    if (post_div > 0x05)
        return false;
  
    uint32_t emc_2x_clk_src_from = (clk_src_emc_from >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT);
    uint32_t emc_2x_clk_src_to = (clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT);
    uint8_t emc_2x_clk_div_from = (clk_src_emc_from & 0xFF);
    uint8_t emc_2x_clk_div_to = (clk_src_emc_to & 0xFF);
  
    if (emc_2x_clk_src_from <= TEGRA_EMC_SRC_PLLMB_UD)
        emc_2x_clk_div_from = 0;
    if (emc_2x_clk_src_to <= TEGRA_EMC_SRC_PLLMB_UD)
        emc_2x_clk_div_to = 0;

    /* Clock sources are different and one of them is CLK_M */
    if ((emc_2x_clk_src_to != emc_2x_clk_src_from)
        && ((emc_2x_clk_src_to & 0xFFFFFFFB)
        || (emc_2x_clk_src_from & 0xFFFFFFFB)))
        return true;

    float val_to = (double)rate_to * ((double)((emc_2x_clk_div_to >> 1) + 1) + (double)(emc_2x_clk_div_to & 1) * 0.5) * (double)(post_div + 1);
    float val_from = (double)rate_from * ((double)((emc_2x_clk_div_from >> 1) + 1) + (double)(emc_2x_clk_div_from & 1) * 0.5) * (double)(post_div + 1);    
    float ratio = (val_from / val_to);
  
    if ((ratio > 1.01f) || (ratio < 0.99f))
        return true;
  
    return false;
}

static uint32_t set_pll(uint32_t rate_to, uint32_t rate_osc, uint32_t clk_src_emc_to, bool is_pllmb) {
    volatile tegra_car_t *car = car_get_regs();
    
    static const pll_cfg_t pll_vals[] = {
        {0xB71B00, 0x2FAF0800, 0x42, 0x01, 0x00},       /* 800Mhz rate with 12Mhz oscillator (unsupported). */
        {0xC65D40, 0x2FAF0800, 0x3D, 0x01, 0x00},       /* 800Mhz rate with 13Mhz oscillator (unsupported). */
        {0x249F000, 0x11BD0400, 0x5D, 0x04, 0x02},      /* 297.6Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x17D78400, 0x7D, 0x04, 0x02},      /* 400Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x18519600, 0x55, 0x04, 0x01},      /* 408Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x1FC1E200, 0x6F, 0x04, 0x01},      /* 532.8Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x27AC4000, 0x68, 0x03, 0x01},      /* 665.6Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x2FAF0800, 0x7D, 0x03, 0x01},      /* 800Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x3780FC00, 0x61, 0x04, 0x00},      /* 931.2Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x3F83C400, 0x6F, 0x04, 0x00},      /* 1065.6Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x47868C00, 0x7D, 0x04, 0x00},      /* 1200Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x4F588000, 0x68, 0x03, 0x00},      /* 1331.2Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x56F9A000, 0x4C, 0x02, 0x00},      /* 1459.2Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x5F5E1000, 0x7D, 0x03, 0x00},      /* 1600Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x6F01F800, 0x61, 0x02, 0x00},      /* 1862.4Mhz rate with 38.4Mhz oscillator. */
        {0x249F000, 0x7F078800, 0x6F, 0x02, 0x00},      /* 2131.2Mhz rate with 38.4Mhz oscillator. */
        {0x00, 0x00, 0x00, 0x00, 0x00}                  /* Dummy entry. */
    };
    
    uint32_t rate_to_hz = (rate_to * 1000);
    uint32_t rate_osc_hz = (rate_osc * 1000);
        
    int freq_idx = 0;
    for (int i = 0; pll_vals[i].osc_freq; i++) {
        if ((rate_osc_hz == pll_vals[i].osc_freq) && (rate_to_hz == pll_vals[i].out_freq)) {
            freq_idx = i;
            break;
        }
    }
    
    uint32_t res = clk_src_emc_to;
    
    /* Failed to find the PLL values */
    if (!pll_vals[freq_idx].osc_freq)
        return res;
  
    uint32_t feedback_div = pll_vals[freq_idx].feedback_div;
    uint32_t input_div = pll_vals[freq_idx].input_div;
    uint32_t post_div = pll_vals[freq_idx].post_div;
    
    if (is_pllmb) {
        /* Set PLLMB_DIVM, PLLMB_DIVN and PLLMB_DIVP */
        car->pllmb_base = (input_div | (feedback_div << 0x08) | ((post_div & 0x1F) << 0x14));
        
        /* Set PLLMB_ENABLE */
        car->pllmb_base |= 0x40000000;
            
        /* Clock source is PLLM_UD */
        if ((clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) == TEGRA_EMC_SRC_PLLM_UD)
            res = ((clk_src_emc_to & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLMB_UD << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
        else if ((clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) == TEGRA_EMC_SRC_PLLM)    /* Clock source is PLLM_OUT0 */
            res = (clk_src_emc_to | (TEGRA_EMC_SRC_PLLMB << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
        
        while (!(car->pllmb_base & 0x8000000)) {
            /* Wait for PLLMB_LOCK to be set */
        }
    } else {
        /* Set PLLM_DIVM, PLLM_DIVN and PLLM_DIVP */
        car->pllm_base = (input_div | (feedback_div << 0x08) | ((post_div & 0x1F) << 0x14));
        
        /* Set PLLM_EN_LCKDET */
        car->pllm_misc2 |= 0x10;
        
        /* Set PLLM_ENABLE */
        car->pllm_base |= 0x40000000;
            
        /* Clock source is PLLM_UD */
        if ((clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT) == TEGRA_EMC_SRC_PLLM_UD)
            res = ((clk_src_emc_to & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLM_UD << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));

        while (!(car->pllm_base & 0x8000000)) {
            /* Wait for PLLM_LOCK to be set */
        }
    }
    
    return res;
}

static void set_clock(tegra_emc_timing_t* current_timing, tegra_emc_timing_t* next_timing, uint32_t training, uint32_t next_clk_src) {
    volatile tegra_car_t *car = car_get_regs();
    
    /* Extract training values */
    bool train_ca = (training & 0x01);
    bool train_ca_vref = (training & 0x02);
    bool train_quse = (training & 0x04);
    bool train_quse_vref = (training & 0x08);
    bool train_wr = (training & 0x10);
    bool train_wr_vref = (training & 0x20);
    bool train_rd = (training & 0x40);
    bool train_rd_vref = (training & 0x80);
    bool train_swap_rank = (training & 0x100);
    bool train_self_refresh = (training & 0x200);
    
    /* Check if we should do training. */
    bool dvfs_with_training = (training & 0xF7);
    
    bool skip_zqcal = false;
    bool compensate_trimmer_applicable = false;
    uint32_t zqcal_before_cc_cutoff = 2400; /* In picoseconds */
    int zq_latch_dvfs_wait_time;
    
    uint32_t mr13_catr_enable;
    uint32_t mr13_flip_fspwr;
    uint32_t mr13_flip_fspop;

    int next_push, next_dq_e_ivref, next_dqs_e_ivref;

    uint32_t zq_wait_long;
    uint32_t zq_wait_short;

    uint32_t tRTM;
    uint32_t RP_war;
    uint32_t R2P_war;
    uint32_t TRPab_war;
    int nRTP;
    uint32_t deltaTWATM;
    uint32_t W2P_war;
    uint32_t tRPST;

    uint32_t mrw_req;
    uint32_t adel = 0;
    uint32_t next_timing_rate_mhz = next_timing->rate / 1000;

    /* Set some common values needed. */
    int dram_type = emc_read(EMC_FBIO_CFG5) & EMC_FBIO_CFG5_DRAM_TYPE_MASK >> EMC_FBIO_CFG5_DRAM_TYPE_SHIFT;
    int dram_dev_num = ((mc_read(MC_EMEM_ADR_CFG) & 1) + 1);
    bool shared_zq_resistor = ((current_timing->burst_regs[EMC_ZCAL_WAIT_CNT_INDEX] >> 31) & 1);
    bool channel_mode = ((current_timing->burst_regs[EMC_FBIO_CFG7_INDEX] >> 2) & 1);
    bool is_lpddr3 = (dram_type == DRAM_TYPE_LPDDR2) && ((next_timing->burst_regs[EMC_FBIO_CFG5_INDEX] >> 25) & 1);
    bool opt_zcal_en_cc = ((next_timing->burst_regs[EMC_ZCAL_INTERVAL_INDEX] && !current_timing->burst_regs[EMC_ZCAL_INTERVAL_INDEX]) || (dram_type == DRAM_TYPE_LPDDR4));
    bool opt_war_200024907 = (dram_type == DRAM_TYPE_LPDDR4);
    bool opt_do_sw_qrst = false;
    bool opt_cc_short_zcal = true;
    bool opt_short_zcal = true;
    bool save_restore_clkstop_pd = true;
    uint32_t opt_dll_mode = (dram_type == DRAM_TYPE_DDR3) ? get_dll_state(next_timing) : DLL_OFF;
    uint32_t opt_dvfs_mode = MAN_SR;
    uint32_t emc_auto_cal_config = emc_read(EMC_AUTO_CAL_CONFIG);

    /* In picoseconds. */
    uint32_t source_clock_period = 1000000000 / current_timing->rate;
    uint32_t destination_clock_period = 1000000000 / next_timing->rate;

    uint32_t tFC_lpddr4 = 1000 * next_timing->dram_timings[T_FC_LPDDR4];
    uint32_t tZQCAL_lpddr4 = 1000000;
    int tZQCAL_lpddr4_fc_adj = (source_clock_period > zqcal_before_cc_cutoff) ? tZQCAL_lpddr4 / destination_clock_period : (tZQCAL_lpddr4 - tFC_lpddr4) / destination_clock_period;
    
    g_fsp_for_next_freq = !g_fsp_for_next_freq;
    
    uint32_t emc_dbg_o = emc_read(EMC_DBG);
    uint32_t emc_pin_o = emc_read(EMC_PIN);
    uint32_t emc_cfg_pipe_clk_o = emc_read(EMC_CFG_PIPE_CLK);
    uint32_t emc_dbg = emc_dbg_o;

    uint32_t emc_cfg = next_timing->burst_regs[EMC_CFG_INDEX];
    uint32_t emc_sel_dpd_ctrl = next_timing->emc_sel_dpd_ctrl;
    
    emc_cfg &= ~(EMC_CFG_DYN_SELF_REF | EMC_CFG_DRAM_ACPD | EMC_CFG_DRAM_CLKSTOP_SR | EMC_CFG_DRAM_CLKSTOP_PD);
    emc_sel_dpd_ctrl &= ~(EMC_SEL_DPD_CTRL_CLK_SEL_DPD_EN | EMC_SEL_DPD_CTRL_CA_SEL_DPD_EN | EMC_SEL_DPD_CTRL_RESET_SEL_DPD_EN | EMC_SEL_DPD_CTRL_ODT_SEL_DPD_EN | EMC_SEL_DPD_CTRL_DATA_SEL_DPD_EN);

    /* Step 1:
     *   Pre DVFS SW sequence.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 1\n");
    
    /* Step 1.1: Disable DLL. */
    uint32_t tmp = emc_read(EMC_CFG_DIG_DLL);
    tmp &= ~EMC_CFG_DIG_DLL_CFG_DLL_EN;
    emc_write(tmp, EMC_CFG_DIG_DLL);

    /* Request a timing update. */
    emc_timing_update(channel_mode);
    
    /* Wait for DLL to be disabled. */
    wait_for_update(EMC_CFG_DIG_DLL, EMC_CFG_DIG_DLL_CFG_DLL_EN, false, REG_EMC);
    if (channel_mode)
        wait_for_update(EMC_CFG_DIG_DLL, EMC_CFG_DIG_DLL_CFG_DLL_EN, false, REG_EMC1);
    
    /* Step 1.2: Disable AUTOCAL. */
    emc_auto_cal_config = next_timing->emc_auto_cal_config;
    uint32_t auto_cal_en = emc_auto_cal_config & EMC_AUTO_CAL_CONFIG_AUTO_CAL_ENABLE;
    emc_auto_cal_config &= ~EMC_AUTO_CAL_CONFIG_AUTO_CAL_START;
    emc_auto_cal_config |=  EMC_AUTO_CAL_CONFIG_AUTO_CAL_MEASURE_STALL;
    emc_auto_cal_config |=  EMC_AUTO_CAL_CONFIG_AUTO_CAL_UPDATE_STALL;
    emc_auto_cal_config |=  auto_cal_en;
    emc_write(emc_auto_cal_config, EMC_AUTO_CAL_CONFIG);

    /* Step 1.3: Disable other power features. */
    emc_set_shadow_bypass(ACTIVE);
    emc_write(emc_cfg, EMC_CFG);
    emc_write(emc_sel_dpd_ctrl, EMC_SEL_DPD_CTRL);
    emc_set_shadow_bypass(ASSEMBLY);
    
    /* Skip this if dvfs_with_training is set. */
    if (!dvfs_with_training && next_timing->periodic_training) {
        if (dram_dev_num == TWO_RANK) {
            /* Wait for DRAM to get out of power down. */
            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_DRAM_IN_POWERDOWN_MASK, false, REG_EMC);
            if (channel_mode)
                wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_DRAM_IN_POWERDOWN_MASK, false, REG_EMC1);
        } else {
            wait_for_update(EMC_EMC_STATUS, 0x10, false, REG_EMC);
            if (channel_mode)
                wait_for_update(EMC_EMC_STATUS, 0x10, false, REG_EMC1);
        }
        
        /* Wait for DRAM to get out of self refresh. */
        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_DRAM_IN_SELF_REFRESH_MASK, false, REG_EMC);
        if (channel_mode)
            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_DRAM_IN_SELF_REFRESH_MASK, false, REG_EMC1);

        /* Reset all clock tree values. */
        reset_dram_clktree_values(next_timing);
        
        /* Do DVFS_SEQUENCE. */
        adel = periodic_compensation_handler(current_timing, next_timing, dram_dev_num, channel_mode, DVFS_SEQUENCE);
                             
        /* Check if we should use compensate trimmer. */
        compensate_trimmer_applicable = next_timing->periodic_training && ((adel * 128 * next_timing_rate_mhz) / 1000000) > next_timing->tree_margin;
    }

    emc_write(EMC_INTSTATUS_CLKCHANGE_COMPLETE, EMC_INTSTATUS);
    emc_set_shadow_bypass(ACTIVE);
    emc_write(emc_cfg, EMC_CFG);
    emc_write(emc_sel_dpd_ctrl, EMC_SEL_DPD_CTRL);
    emc_write(emc_cfg_pipe_clk_o | EMC_CFG_PIPE_CLK_CLK_ALWAYS_ON, EMC_CFG_PIPE_CLK);
    emc_write(next_timing->emc_fdpd_ctrl_cmd_no_ramp & ~EMC_FDPD_CTRL_CMD_NO_RAMP_CMD_DPD_NO_RAMP_ENABLE, EMC_FDPD_CTRL_CMD_NO_RAMP);

    uint32_t bg_regulator_mode_change = ((next_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & EMC_PMACRO_BG_BIAS_CTRL_0_BGLP_E_PWRD) ^ (current_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & EMC_PMACRO_BG_BIAS_CTRL_0_BGLP_E_PWRD)) || ((next_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] &
    EMC_PMACRO_BG_BIAS_CTRL_0_BG_E_PWRD) ^ (current_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & EMC_PMACRO_BG_BIAS_CTRL_0_BG_E_PWRD));
         
    uint32_t enable_bg_regulator = (next_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & EMC_PMACRO_BG_BIAS_CTRL_0_BG_E_PWRD) == 0;

    /* Check if we need to change BG the regulator. */
    if (bg_regulator_mode_change) {
        if (enable_bg_regulator)
            emc_write(current_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & ~EMC_PMACRO_BG_BIAS_CTRL_0_BG_E_PWRD, EMC_PMACRO_BG_BIAS_CTRL_0);
        else
            emc_write(current_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & ~EMC_PMACRO_BG_BIAS_CTRL_0_BGLP_E_PWRD, EMC_PMACRO_BG_BIAS_CTRL_0);
    }

    /* Check if we need to turn on VREF generator. */
    if ((((!current_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX] &
           EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF)) &&
         ((next_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX] &
           EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF))) ||
        ((!(current_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX] &
           EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF)) &&
         ((next_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX] &
           EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF))))
    {
        uint32_t pad_tx_ctrl = next_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX];
        uint32_t last_pad_tx_ctrl = current_timing->burst_regs[EMC_PMACRO_DATA_PAD_TX_CTRL_INDEX];

        next_dqs_e_ivref = pad_tx_ctrl & EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF;
        next_dq_e_ivref = pad_tx_ctrl & EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF;
        next_push = (last_pad_tx_ctrl & ~EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF & ~EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF) | next_dq_e_ivref | next_dqs_e_ivref;
        emc_write(next_push, EMC_PMACRO_DATA_PAD_TX_CTRL);
        udelay(1);
    } else if (bg_regulator_mode_change) {
        udelay(1);
    }

    emc_set_shadow_bypass(ASSEMBLY);

    /* Step 2:
     *   Prelock the DLL.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 2\n");
    
    if (next_timing->burst_regs[EMC_CFG_DIG_DLL_INDEX] & EMC_CFG_DIG_DLL_CFG_DLL_EN) {
        dll_prelock(next_timing, dvfs_with_training, next_clk_src);
    } else {
        change_dll_src(next_timing, next_clk_src);
        dll_disable(channel_mode);
    }

    /* Step 3:
     *   Prepare autocal for the clock change.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 3\n");
    
    emc_set_shadow_bypass(ACTIVE);
    emc_write(next_timing->emc_auto_cal_config2, EMC_AUTO_CAL_CONFIG2);
    emc_write(next_timing->emc_auto_cal_config3, EMC_AUTO_CAL_CONFIG3);
    emc_write(next_timing->emc_auto_cal_config4, EMC_AUTO_CAL_CONFIG4);
    emc_write(next_timing->emc_auto_cal_config5, EMC_AUTO_CAL_CONFIG5);
    emc_write(next_timing->emc_auto_cal_config6, EMC_AUTO_CAL_CONFIG6);
    emc_write(next_timing->emc_auto_cal_config7, EMC_AUTO_CAL_CONFIG7);
    emc_write(next_timing->emc_auto_cal_config8, EMC_AUTO_CAL_CONFIG8);
    emc_set_shadow_bypass(ASSEMBLY);

    emc_auto_cal_config |= (EMC_AUTO_CAL_CONFIG_AUTO_CAL_COMPUTE_START | auto_cal_en);
    emc_write(emc_auto_cal_config, EMC_AUTO_CAL_CONFIG);

    /* Step 4:
     *   Update EMC_CFG.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 4\n");
    
    if ((source_clock_period > 50000) && (dram_type == DRAM_TYPE_LPDDR4))
        ccfifo_write(EMC_SELF_REF, 1, 0);
    else
        emc_write(next_timing->emc_cfg_2, EMC_CFG_2);

    /* Step 5:
     *   Prepare reference variables for ZQCAL regs.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 5\n");
    
    uint32_t emc_zcal_interval = current_timing->burst_regs[EMC_ZCAL_INTERVAL_INDEX];
    emc_zcal_interval &= 0xFF000000;
    uint32_t emc_zcal_wait_cnt_old = current_timing->burst_regs[EMC_ZCAL_WAIT_CNT_INDEX];
    uint32_t emc_zcal_wait_cnt_new = next_timing->burst_regs[EMC_ZCAL_WAIT_CNT_INDEX];
    emc_zcal_wait_cnt_old &= ~EMC_ZCAL_WAIT_CNT_ZCAL_WAIT_CNT_MASK;
    emc_zcal_wait_cnt_new &= ~EMC_ZCAL_WAIT_CNT_ZCAL_WAIT_CNT_MASK;

    if (dram_type == DRAM_TYPE_LPDDR4)
        zq_wait_long = max((uint32_t)1, div_o3(1000000, destination_clock_period));
    else if (dram_type == DRAM_TYPE_LPDDR2 || is_lpddr3)
        zq_wait_long = max(next_timing->min_mrs_wait, div_o3(360000, destination_clock_period)) + 4;
    else if (dram_type == DRAM_TYPE_DDR3)
        zq_wait_long = max((uint32_t)256, div_o3(320000, destination_clock_period) + 2);
    else
        zq_wait_long = 0;

    if (dram_type == DRAM_TYPE_LPDDR2 || is_lpddr3)
        zq_wait_short = max(max(next_timing->min_mrs_wait, (uint32_t)6), div_o3(90000, destination_clock_period)) + 4;
    else if (dram_type == DRAM_TYPE_DDR3)
        zq_wait_short = max((uint32_t)64, div_o3(80000, destination_clock_period)) + 2;
    else
        zq_wait_short = 0;
    
    /* TODO: Actually use the reference variables. */
    (void)zq_wait_long;
    (void)zq_wait_short;

    /* Step 6:
     *   Training code.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 6\n");
    
    if ((train_ca || train_ca_vref) && (dram_dev_num == TWO_RANK)) {
        emc_write(0x107, EMC_PIN);
    }

    /* Step 7:
     *   Program FSP reference registers and send MRWs to new FSPWR.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 7\n");
    
    /* Step 7.1: Bug 200024907 - Patch RP R2P */
    if (opt_war_200024907) {
        nRTP = 16;
        if (source_clock_period >= 1000000/1866) /* 535.91 ps */
            nRTP = 14;
        if (source_clock_period >= 1000000/1600) /* 625.00 ps */
            nRTP = 12;
        if (source_clock_period >= 1000000/1333) /* 750.19 ps */
            nRTP = 10;
        if (source_clock_period >= 1000000/1066) /* 938.09 ps */
            nRTP = 8;

        deltaTWATM = max_t(uint32_t, div_o3(7500, source_clock_period), 8);

        /*
         * Originally there was a + .5 in the tRPST calculation.
         * However since we can't do FP in the kernel and the tRTM
         * computation was in a floating point ceiling function, adding
         * one to tRTP should be ok. There is no other source of non
         * integer values, so the result was always going to be
         * something for the form: f_ceil(N + .5) = N + 1;
         */
        tRPST = ((current_timing->emc_mrw & 0x80) >> 7);
        tRTM = current_timing->dram_timings[RL] + div_o3(3600, source_clock_period) + max_t(uint32_t, div_o3(7500, source_clock_period), 8) + tRPST + 1 + nRTP;

        if (current_timing->burst_regs[EMC_RP_INDEX] < tRTM) {
            if (tRTM > (current_timing->burst_regs[EMC_R2P_INDEX] + current_timing->burst_regs[EMC_RP_INDEX])) {
                R2P_war = tRTM - current_timing->burst_regs[EMC_RP_INDEX];
                RP_war = current_timing->burst_regs[EMC_RP_INDEX];
                TRPab_war = current_timing->burst_regs[EMC_TRPAB_INDEX];
                if (R2P_war > 63) {
                    RP_war = R2P_war + current_timing->burst_regs[EMC_RP_INDEX] - 63;
                    if (TRPab_war < RP_war)
                        TRPab_war = RP_war;
                    R2P_war = 63;
                }
            } else {
                R2P_war = current_timing-> burst_regs[EMC_R2P_INDEX];
                RP_war = current_timing->burst_regs[EMC_RP_INDEX];
                TRPab_war = current_timing->burst_regs[EMC_TRPAB_INDEX];
            }

            if (RP_war < deltaTWATM) {
                W2P_war = current_timing->burst_regs[EMC_W2P_INDEX] + deltaTWATM - RP_war;
                if (W2P_war > 63) {
                    RP_war = RP_war + W2P_war - 63;
                    if (TRPab_war < RP_war)
                        TRPab_war = RP_war;
                    W2P_war = 63;
                }
            } else {
                W2P_war = current_timing->burst_regs[EMC_W2P_INDEX];
            }

            if ((current_timing->burst_regs[EMC_W2P_INDEX] != W2P_war)
                || (current_timing->burst_regs[EMC_R2P_INDEX] != R2P_war)
                || (current_timing->burst_regs[EMC_RP_INDEX] != RP_war)
                || (current_timing->burst_regs[EMC_TRPAB_INDEX] != TRPab_war))
            {
                emc_set_shadow_bypass(ACTIVE);
                emc_write(RP_war, EMC_RP);
                emc_write(R2P_war, EMC_R2P);
                emc_write(W2P_war, EMC_W2P);
                emc_write(TRPab_war, EMC_TRPAB);
                emc_set_shadow_bypass(ASSEMBLY);
                udelay(1);
            }
        }
    }

    if (!g_fsp_for_next_freq) {
        mr13_flip_fspwr = (next_timing->emc_mrw3 & 0xffffff3f) | 0x80;
        mr13_flip_fspop = (next_timing->emc_mrw3 & 0xffffff3f) | 0x00;
    } else {
        mr13_flip_fspwr = (next_timing->emc_mrw3 & 0xffffff3f) | 0x40;
        mr13_flip_fspop = (next_timing->emc_mrw3 & 0xffffff3f) | 0xc0;
    }

    mr13_catr_enable = (mr13_flip_fspwr & 0xFFFFFFFE) | 0x01;
    
    if (dram_dev_num == TWO_RANK) {
        if (train_ca || train_ca_vref) {
            if (train_swap_rank) {
                mr13_flip_fspop = (mr13_flip_fspop & 0x3FFFFFFF) | 0x80000000;
                mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF)| 0x40000000;
            } else {
                mr13_flip_fspop = (mr13_flip_fspop & 0x3FFFFFFF) | 0x40000000;
                mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF) | 0x80000000;
            }
        } else {
            if (train_swap_rank)
                mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF) | 0x40000000;
            else
                mr13_catr_enable = (mr13_catr_enable & 0x3FFFFFFF) | 0x80000000;
        }
    }

    if (dram_type == DRAM_TYPE_LPDDR4) {
        emc_write(mr13_flip_fspwr, EMC_MRW3);
        emc_write(next_timing->emc_mrw, EMC_MRW);
        emc_write(next_timing->emc_mrw2, EMC_MRW2);
    }

    /* Step 8:
     *   Program the shadow registers.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 8\n");
    
    /* Set burst registers. */
    for (int i = 0; i < next_timing->num_burst; i++) {
        uint32_t var = 0;
        uint32_t wval = 0;

        if (!burst_regs_off[i])
            continue;

        var = burst_regs_off[i];
        
        if (dvfs_with_training) {
            if (train_ca || train_ca_vref)
                wval = next_timing->shadow_regs_ca_train[i];
            else if (train_quse || train_quse_vref)
                wval = next_timing->shadow_regs_quse_train[i];
            else if (train_wr || train_wr_vref || train_rd || train_rd_vref)
                wval = next_timing->shadow_regs_rdwr_train[i];
        }
        else
            wval = next_timing->burst_regs[i];

        if (dram_type != DRAM_TYPE_LPDDR4 &&
            (var == EMC_MRW6      || var == EMC_MRW7 ||
             var == EMC_MRW8      || var == EMC_MRW9 ||
             var == EMC_MRW10     || var == EMC_MRW11 ||
             var == EMC_MRW12     || var == EMC_MRW13 ||
             var == EMC_MRW14     || var == EMC_MRW15 ||
             var == EMC_TRAINING_CTRL))
            continue;

        if (var == EMC_CFG) {
            wval &= ~EMC_CFG_DRAM_ACPD;
            wval &= ~EMC_CFG_DYN_SELF_REF;
            if (dram_type == DRAM_TYPE_LPDDR4) {
                wval &= ~EMC_CFG_DRAM_CLKSTOP_SR;
                wval &= ~EMC_CFG_DRAM_CLKSTOP_PD;
            }
        } else if ((var == EMC_MRS_WAIT_CNT)
            && (dram_type == DRAM_TYPE_LPDDR2)
            && opt_zcal_en_cc && !opt_cc_short_zcal && opt_short_zcal) {
            wval = (wval & ~(EMC_MRS_WAIT_CNT_SHORT_WAIT_MASK <<
                     EMC_MRS_WAIT_CNT_SHORT_WAIT_SHIFT)) |
               ((zq_wait_long & EMC_MRS_WAIT_CNT_SHORT_WAIT_MASK) <<
                EMC_MRS_WAIT_CNT_SHORT_WAIT_SHIFT);
        } else if ((var == EMC_ZCAL_WAIT_CNT)
            && (dram_type == DRAM_TYPE_DDR3)
            && opt_zcal_en_cc && !opt_cc_short_zcal && opt_short_zcal) {
            wval = (wval & ~(EMC_ZCAL_WAIT_CNT_ZCAL_WAIT_CNT_MASK <<
                       EMC_ZCAL_WAIT_CNT_ZCAL_WAIT_CNT_SHIFT)) |
                ((zq_wait_long &
                  EMC_ZCAL_WAIT_CNT_ZCAL_WAIT_CNT_MASK) <<
                  EMC_MRS_WAIT_CNT_SHORT_WAIT_SHIFT);
        } else if ((var == EMC_ZCAL_INTERVAL) && opt_zcal_en_cc) {
            wval = 0; /* EMC_ZCAL_INTERVAL reset value. */
        } else if (var == EMC_PMACRO_AUTOCAL_CFG_COMMON) {
            wval |= EMC_PMACRO_AUTOCAL_CFG_COMMON_E_CAL_BYPASS_DVFS;
        } else if (var == EMC_PMACRO_DATA_PAD_TX_CTRL) {
            wval &=
                 ~(EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSP_TX_E_DCC |
                   EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSN_TX_E_DCC |
                   EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_TX_E_DCC |
                   EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_CMD_TX_E_DCC);
        } else if (var == EMC_PMACRO_CMD_PAD_TX_CTRL) {
            wval |= EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_DRVFORCEON;
            wval &= ~(EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSP_TX_E_DCC |
                  EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSN_TX_E_DCC |
                  EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_E_DCC |
                  EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_CMD_TX_E_DCC);
        } else if (var == EMC_PMACRO_BRICK_CTRL_RFU1) {
            wval &= 0xf800f800;
        } else if (var == EMC_PMACRO_COMMON_PAD_TX_CTRL) {
            wval &= 0xfffffff0;
        } else if (var == EMC_TRAINING_CTRL) {
            wval |= (train_swap_rank << 14);     /* Training only. */
        }

        emc_write(wval, var);
    }

    /* Do EMC refresh adjustment here (disabled). */
    set_over_temp_timing(next_timing, TEGRA_DRAM_OVER_TEMP_NONE);

    if (dram_type == DRAM_TYPE_LPDDR4) {
        /* Use the current timing when training. */
        if (dvfs_with_training)
            mrw_req = (23 << EMC_MRW_MRW_MA_SHIFT) | (current_timing->run_clocks & EMC_MRW_MRW_OP_MASK);
        else
            mrw_req = (23 << EMC_MRW_MRW_MA_SHIFT) | (next_timing->run_clocks & EMC_MRW_MRW_OP_MASK);
        
        emc_write(mrw_req, EMC_MRW);
    }

    /* Per channel burst registers. */
    for (int i = 0; i < next_timing->num_burst_per_ch; i++) {
        if (!burst_regs_per_ch_off[i])
            continue;

        if (dram_type != DRAM_TYPE_LPDDR4 &&
            (burst_regs_per_ch_off[i] == EMC_MRW6 ||
                        burst_regs_per_ch_off[i] == EMC_MRW7 ||
                        burst_regs_per_ch_off[i] == EMC_MRW8 ||
                        burst_regs_per_ch_off[i] == EMC_MRW9 ||
                        burst_regs_per_ch_off[i] == EMC_MRW10 ||
                        burst_regs_per_ch_off[i] == EMC_MRW11 ||
                        burst_regs_per_ch_off[i] == EMC_MRW12 ||
                        burst_regs_per_ch_off[i] == EMC_MRW13 ||
                        burst_regs_per_ch_off[i] == EMC_MRW14 ||
                        burst_regs_per_ch_off[i] == EMC_MRW15))
            continue;

        /* Filter out second channel if not in DUAL_CHANNEL mode. */
        if ((channel_mode != DUAL_CHANNEL) && (burst_regs_per_ch_type[i] >= REG_EMC1))
            continue;
        
        emc_write_per_ch(next_timing->burst_reg_per_ch[i], burst_regs_per_ch_type[i], burst_regs_per_ch_off[i]);
    }

    /* Vref regs. */
    for (int i = 0; i < next_timing->vref_num; i++) {
        if (!vref_regs_per_ch_off[i])
            continue;

        if ((channel_mode != DUAL_CHANNEL) && (vref_regs_per_ch_type[i] >= REG_EMC1))
            continue;

        emc_write_per_ch(next_timing->vref_perch_regs[i], vref_regs_per_ch_type[i], vref_regs_per_ch_off[i]);
    }
    
    /* Training regs. */
    if (dvfs_with_training) {
        for (int i = 0; i < next_timing->training_mod_num; i++) {
            if (!training_mod_regs_per_ch_off[i])
                continue;

            if ((channel_mode != DUAL_CHANNEL) && (training_mod_regs_per_ch_type[i] >= REG_EMC1))
                continue;

            emc_write_per_ch(next_timing->training_mod_regs[i], training_mod_regs_per_ch_type[i], training_mod_regs_per_ch_off[i]);
        }
    }

    /* Trimmers. */
    for (int i = 0; i < next_timing->num_trim; i++) {
        uint32_t trim_reg;

        if (!trim_regs_off[i])
            continue;

        trim_reg = trim_regs_off[i];
        
        if (compensate_trimmer_applicable &&
            (trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3 ||
             trim_reg == EMC_DATA_BRLSHFT_0 ||
             trim_reg == EMC_DATA_BRLSHFT_1)) {
            uint32_t reg = apply_periodic_compensation_trimmer(next_timing, trim_reg);
            emc_write(reg, trim_regs_off[i]);
        } else {
            emc_write(next_timing->trim_regs[i], trim_regs_off[i]);
        }
    }

    /* Per channel trimmers. */
    for (int i = 0; i < next_timing->num_trim_per_ch; i++) {
        uint32_t trim_reg;

        if (!trim_regs_per_ch_off[i])
            continue;

        if ((channel_mode != DUAL_CHANNEL) && (trim_regs_per_ch_type[i] >= REG_EMC1))
            continue;

        trim_reg = trim_regs_per_ch_off[i];
        
        if (compensate_trimmer_applicable &&
            (trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2 ||
             trim_reg == EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3 ||
             trim_reg == EMC_DATA_BRLSHFT_0 ||
             trim_reg == EMC_DATA_BRLSHFT_1)) {
            uint32_t reg = apply_periodic_compensation_trimmer(next_timing, trim_reg);
            emc_write_per_ch(reg, trim_regs_per_ch_type[i], trim_regs_per_ch_off[i]);
        } else {
            emc_write_per_ch(next_timing->trim_perch_regs[i], trim_regs_per_ch_type[i], trim_regs_per_ch_off[i]);
        }
    }
    
    if (dvfs_with_training) {
        if (train_wr && next_timing->periodic_training && (dram_type == DRAM_TYPE_LPDDR4)) {
            periodic_compensation_handler(current_timing, next_timing, dram_dev_num, channel_mode, WRITE_TRAINING_SEQUENCE);
        }
    } else {
        /* Write burst_mc_regs. */
        for (int i = 0; i < next_timing->num_mc_regs; i++) {
            mc_write(next_timing->burst_mc_regs[i], burst_mc_regs_off[i]);
        }
    }
    
    /* Registers to be programmed on the faster clock. */
    if (!dvfs_with_training && (next_timing->rate < current_timing->rate)) {
        for (int i = 0; i < next_timing->num_up_down; i++) {
            mc_write(next_timing->la_scale_regs[i], la_scale_regs_off[i]);
        }
    }

    /* Step 9:
     *   LPDDR4 section A.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 9\n");
    
    if (dram_type == DRAM_TYPE_LPDDR4) {
        emc_write(emc_zcal_interval, EMC_ZCAL_INTERVAL);
        emc_write(emc_zcal_wait_cnt_new, EMC_ZCAL_WAIT_CNT);
        emc_write(emc_dbg_o | (EMC_DBG_WRITE_MUX_ACTIVE | EMC_DBG_WRITE_ACTIVE_ONLY), EMC_DBG);
        emc_write(emc_zcal_interval, EMC_ZCAL_INTERVAL);
        emc_write(emc_dbg_o, EMC_DBG);
        
        if (dvfs_with_training) {
            emc_set_shadow_bypass(ACTIVE);
            
            emc_write(next_timing->burst_regs[EMC_PMACRO_AUTOCAL_CFG_COMMON_INDEX] | EMC_PMACRO_AUTOCAL_CFG_COMMON_E_CAL_BYPASS_DVFS, EMC_PMACRO_AUTOCAL_CFG_COMMON);
            
            if (train_ca || train_ca_vref)
                emc_write(current_timing->burst_regs[EMC_FBIO_CFG5_INDEX] | EMC_FBIO_CFG5_CMD_BUS_RETURN_TO_ZERO, EMC_FBIO_CFG5);
            
            emc_set_shadow_bypass(ASSEMBLY);
            
            if (channel_mode)
                ccfifo_write(EMC_CFG_SYNC, 0, 0);
            
            /* Change CFG_SWAP. */
            ccfifo_write(EMC_DBG, ((emc_dbg_o & 0xF3FFFFFF) | 0x4000000), 0);
        }
    }

    /* Step 10:
     *   LPDDR4 and DDR3 common section.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 10\n");
    
    if (opt_dvfs_mode == MAN_SR || dram_type == DRAM_TYPE_LPDDR4) {
        if (dram_type == DRAM_TYPE_LPDDR4)
            ccfifo_write(EMC_SELF_REF, 0x101, 0);
        else
            ccfifo_write(EMC_SELF_REF, 0x1, 0);

        if (!(train_ca || train_ca_vref) && (dram_type == DRAM_TYPE_LPDDR4) && (source_clock_period <= zqcal_before_cc_cutoff)) {
            ccfifo_write(EMC_MRW3, mr13_flip_fspwr ^ 0x40, 0);
            ccfifo_write(EMC_MRW6, (next_timing->burst_regs[EMC_MRW6_INDEX] & 0xFFFF3F3F) | (current_timing->burst_regs[EMC_MRW6_INDEX] & 0x0000C0C0), 0);
            ccfifo_write(EMC_MRW14, (next_timing->burst_regs[EMC_MRW14_INDEX] & 0xFFFF0707) | (current_timing->burst_regs[EMC_MRW14_INDEX] & 0x00003838), 0);

            if (dram_dev_num == TWO_RANK) {
                ccfifo_write(EMC_MRW7, (next_timing->burst_regs[EMC_MRW7_INDEX] & 0xFFFF3F3F) | (current_timing->burst_regs[EMC_MRW7_INDEX] & 0x0000C0C0), 0);
                ccfifo_write(EMC_MRW15, (next_timing->burst_regs[EMC_MRW15_INDEX] & 0xFFFF0707) | (current_timing->burst_regs[EMC_MRW15_INDEX] & 0x00003838), 0);
            }
            
            if (opt_zcal_en_cc) {
                if ((dram_dev_num == ONE_RANK) || shared_zq_resistor)
                    ccfifo_write(EMC_ZQ_CAL, 2 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                else
                    ccfifo_write(EMC_ZQ_CAL, EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
            }
        }
    }
    
    emc_dbg = emc_dbg_o;
    if (dram_type == DRAM_TYPE_LPDDR4) {
        if (dvfs_with_training) {
            /* Change CFG_SWAP. */
            emc_dbg = ((emc_dbg_o & 0xF3FFFFFF) | 0x4000000 | EMC_DBG_WRITE_ACTIVE_ONLY);
            ccfifo_write(EMC_DBG, emc_dbg, 0);
        }
        if (train_ca || train_ca_vref) {
            ccfifo_write(EMC_PMACRO_DATA_RX_TERM_MODE, current_timing->burst_regs[EMC_PMACRO_DATA_RX_TERM_MODE_INDEX] & 0xFFFFFCCC, 0);
            
            if ((dram_dev_num == TWO_RANK) && train_swap_rank) {
                ccfifo_write(EMC_MRW3, mr13_flip_fspop | 0x8, (1000 * current_timing->dram_timings[T_RP]) / source_clock_period);
                ccfifo_write(EMC_MRW3, mr13_catr_enable | 0x8, 0);
            } else {
                ccfifo_write(EMC_MRW3, mr13_catr_enable | 0x8, (1000 * current_timing->dram_timings[T_RP]) / source_clock_period);
            }
          
            ccfifo_write(EMC_TR_CTRL_0, 0x15A, 0);
            ccfifo_write(EMC_INTSTATUS, 0, 1000000 / source_clock_period);
        } else {
            ccfifo_write(EMC_MRW3, mr13_flip_fspop | 0x8, (1000 * current_timing->dram_timings[T_RP]) / source_clock_period);
            ccfifo_write(EMC_INTSTATUS, 0, tFC_lpddr4 / source_clock_period);
        }
    }
    
    bool ref_b4_sref_en = false;
    bool cya_issue_pc_ref = false;
    bool cya_allow_ref_cc = false;

    if ((dram_type == DRAM_TYPE_LPDDR4) || (opt_dvfs_mode != MAN_SR)) {
        uint32_t t = 30 + (cya_allow_ref_cc ? (4000 * current_timing->dram_timings[T_RFC]) + ((1000 * current_timing->dram_timings[T_RP]) / source_clock_period) : 0);
        ccfifo_write(EMC_PIN, emc_pin_o & ~(EMC_PIN_PIN_CKE_PER_DEV | EMC_PIN_PIN_CKEB | EMC_PIN_PIN_CKE), t);
    }

    uint32_t ref_delay_mult = 1;
    ref_delay_mult += ref_b4_sref_en   ? 1 : 0;
    ref_delay_mult += cya_allow_ref_cc ? 1 : 0;
    ref_delay_mult += cya_issue_pc_ref ? 1 : 0;
    uint32_t ref_delay = ref_delay_mult * ((1000 * current_timing->dram_timings[T_RP] / source_clock_period) + (1000 * current_timing->dram_timings[T_RFC] / source_clock_period)) + 20;

    /* Step 11:
     *   Ramp down.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 11\n");
    
    ccfifo_write(EMC_CFG_SYNC, 0, (dram_type == DRAM_TYPE_LPDDR4) ? 0 : ref_delay);
    ccfifo_write(EMC_DBG, emc_dbg | (EMC_DBG_WRITE_MUX_ACTIVE | EMC_DBG_WRITE_ACTIVE_ONLY), 0);
    uint32_t ramp_down_wait = dvfs_power_ramp_down(false, current_timing, next_timing, source_clock_period);

    /* Step 12:
     *   Trigger the clock change.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 12\n");
    
    ccfifo_write(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
    if (!dvfs_with_training) {
        ccfifo_write(EMC_DBG, (emc_dbg & ~EMC_DBG_WRITE_ACTIVE_ONLY) | EMC_DBG_WRITE_MUX_ACTIVE, 0);
    }
    
    /* Step 13:
     *   Ramp up.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 13\n");
    
    uint32_t ramp_up_wait = dvfs_power_ramp_up(false, current_timing, next_timing, training, destination_clock_period);
    ccfifo_write(EMC_DBG, emc_dbg, 0);

    /* Step 14:
     *   Bringup CKE pins.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 14\n");
    
    if ((dram_type == DRAM_TYPE_LPDDR4)) {
        uint32_t r = emc_pin_o | EMC_PIN_PIN_CKE;
        if (train_ca || train_ca_vref) {
            r = emc_pin_o & ~(EMC_PIN_PIN_CKE_PER_DEV | EMC_PIN_PIN_CKEB | EMC_PIN_PIN_CKE);
            if (dram_dev_num == TWO_RANK) {
                if (train_swap_rank)
                    ccfifo_write(EMC_PIN, r | EMC_PIN_PIN_CKE_PER_DEV | EMC_PIN_PIN_CKE, 0);
                else
                    ccfifo_write(EMC_PIN, r | EMC_PIN_PIN_CKE_PER_DEV | EMC_PIN_PIN_CKEB, 0);
            }
            else
                ccfifo_write(EMC_PIN, r, 0);
        } else if (dram_dev_num == TWO_RANK) {
            ccfifo_write(EMC_PIN, r | EMC_PIN_PIN_CKEB | EMC_PIN_PIN_CKE_PER_DEV, 0);
        } else {
            ccfifo_write(EMC_PIN, r & ~(EMC_PIN_PIN_CKEB | EMC_PIN_PIN_CKE_PER_DEV), 0);
        }
    }

    /* Step 15:
     *   Calculate zqlatch wait time; has dependency on ramping times.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 15\n");
    
    if (source_clock_period <= zqcal_before_cc_cutoff) {
        int t = (int)(ramp_up_wait + ramp_down_wait) / (int)destination_clock_period;
        zq_latch_dvfs_wait_time = (int)tZQCAL_lpddr4_fc_adj - t;
    } else {
        zq_latch_dvfs_wait_time = tZQCAL_lpddr4_fc_adj - div_o3(1000 * next_timing->dram_timings[T_PDEX], destination_clock_period);
    }

    if (!(train_ca || train_ca_vref) && (dram_type == DRAM_TYPE_LPDDR4) && opt_zcal_en_cc) {
        if (dram_dev_num == ONE_RANK) {
            if (source_clock_period > zqcal_before_cc_cutoff) {
                ccfifo_write(EMC_ZQ_CAL, 2 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_CAL_CMD, div_o3(1000 * next_timing->dram_timings[T_PDEX], destination_clock_period));
            }
            
            if (!dvfs_with_training) {            
                ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, div_o3(1000 * next_timing->dram_timings[T_PDEX], destination_clock_period));
                ccfifo_write(EMC_SELF_REF, 0x100, 0);
                ccfifo_write(EMC_REF, 0, 0);
            }
            
            ccfifo_write(EMC_ZQ_CAL, 2 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_LATCH_CMD, max_t(int, 0, zq_latch_dvfs_wait_time));
        } else if (shared_zq_resistor) {
            if (source_clock_period > zqcal_before_cc_cutoff) {
                ccfifo_write(EMC_ZQ_CAL, 2 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_CAL_CMD, div_o3(1000 * next_timing->dram_timings[T_PDEX], destination_clock_period));
            }
            
            ccfifo_write(EMC_ZQ_CAL, 2 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_LATCH_CMD, max_t(int, 0, zq_latch_dvfs_wait_time) + div_o3(1000 * next_timing->dram_timings[T_PDEX], destination_clock_period));
            ccfifo_write(EMC_ZQ_CAL, 1 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_LATCH_CMD, 0);

            if (!dvfs_with_training) {  
                ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, 0);
                ccfifo_write(EMC_SELF_REF, 0x100, 0);
                ccfifo_write(EMC_REF, 0, 0);
            }
            
            ccfifo_write(EMC_ZQ_CAL, 1 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_LATCH_CMD, tZQCAL_lpddr4 / destination_clock_period);
        } else {
            if (source_clock_period > zqcal_before_cc_cutoff) {
                ccfifo_write(EMC_ZQ_CAL, EMC_ZQ_CAL_ZQ_CAL_CMD, div_o3(1000 * next_timing->dram_timings[T_PDEX], destination_clock_period));
            }

            if (!dvfs_with_training) {  
                ccfifo_write(EMC_MRW3, (mr13_flip_fspop & 0xF3FFFFF7) | 0xC000000, div_o3(1000 * next_timing->dram_timings[T_PDEX], destination_clock_period));
                ccfifo_write(EMC_SELF_REF, 0x100, 0);
                ccfifo_write(EMC_REF, 0, 0);
            }
            
            ccfifo_write(EMC_ZQ_CAL, EMC_ZQ_CAL_ZQ_LATCH_CMD, max_t(int, 0, zq_latch_dvfs_wait_time));
        }
    }

    /* WAR: delay for zqlatch */
    ccfifo_write(EMC_INTSTATUS, 0, 10);

    /* Step 16:
     *   LPDDR4 Conditional Training Kickoff.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 16\n");
    
    if (dvfs_with_training && (dram_type == DRAM_TYPE_LPDDR4)) {
        ccfifo_write(EMC_INTSTATUS, 0, (1020000 / destination_clock_period));

        uint32_t train_cmd = 0;

        if (train_ca)
            train_cmd |= (1 << 1);  /* CA */
        if (train_ca_vref)
            train_cmd |= (1 << 5);  /* CA_VREF */
        if (train_quse)
            train_cmd |= (1 << 4);  /* QUSE */
        if (train_quse_vref)
            train_cmd |= (1 << 8);  /* QUSE_VREF */
        if (train_wr)
            train_cmd |= (1 << 3);  /* WR */
        if (train_wr_vref)
            train_cmd |= (1 << 6);  /* WR_VREF */
        if (train_rd)
            train_cmd |= (1 << 2);  /* RD */
        if (train_rd_vref)
            train_cmd |= (1 << 7);  /* RD_VREF */
        
        train_cmd |= (1 << 31);     /* GO */

        ccfifo_write(EMC_TRAINING_CMD, train_cmd, 0);

        if (bg_regulator_mode_change) {
            if (enable_bg_regulator)
                ccfifo_write(EMC_PMACRO_BG_BIAS_CTRL_0, current_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & ~EMC_PMACRO_BG_BIAS_CTRL_0_BG_E_PWRD, 0);
            else
                ccfifo_write(EMC_PMACRO_BG_BIAS_CTRL_0, current_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & ~EMC_PMACRO_BG_BIAS_CTRL_0_BGLP_E_PWRD, 0);
        }

        ccfifo_write(EMC_SWITCH_BACK_CTRL, 1, 0);

        if (!(train_ca || train_ca_vref) || train_swap_rank) {
            ccfifo_write(EMC_MRW3, mr13_flip_fspop ^ 0xC0, 0);
            ccfifo_write(EMC_INTSTATUS, 0, (1000000 / destination_clock_period));
        }

        ccfifo_write(EMC_PIN, emc_pin_o & ~(EMC_PIN_PIN_CKE_PER_DEV | EMC_PIN_PIN_CKEB | EMC_PIN_PIN_CKE), 0);
        ccfifo_write(EMC_CFG_SYNC, 0, 0);
        ccfifo_write(EMC_DBG, emc_dbg | (EMC_DBG_WRITE_ACTIVE_ONLY | EMC_DBG_WRITE_MUX_ACTIVE), 0);
        
        dvfs_power_ramp_down(true, current_timing, next_timing, destination_clock_period);
        
        ccfifo_write(EMC_STALL_THEN_EXE_AFTER_CLKCHANGE, 1, 0);
        ccfifo_write(EMC_DBG, (emc_dbg & ~EMC_DBG_WRITE_ACTIVE_ONLY) | EMC_DBG_WRITE_MUX_ACTIVE, 0);
        
        dvfs_power_ramp_up(true, current_timing, next_timing, training, source_clock_period);

        ccfifo_write(EMC_DBG, emc_dbg, 0);

        if (dram_dev_num == TWO_RANK)
            ccfifo_write(EMC_PIN, emc_pin_o | (EMC_PIN_PIN_CKE_PER_DEV | EMC_PIN_PIN_CKEB | EMC_PIN_PIN_CKE), 0);
        else
            ccfifo_write(EMC_PIN, (emc_pin_o & ~(EMC_PIN_PIN_CKE_PER_DEV | EMC_PIN_PIN_CKEB | EMC_PIN_PIN_CKE)) | EMC_PIN_PIN_CKE, 0);

        if (train_ca || train_ca_vref) {
            ccfifo_write(EMC_TR_CTRL_0, 0x4A, (200000 / source_clock_period));
            ccfifo_write(EMC_TR_CTRL_0, 0x40, (1000000 / source_clock_period));
            ccfifo_write(EMC_MRW3, mr13_catr_enable & 0xFFFFFFFE, 0);
            ccfifo_write(EMC_INTSTATUS, 0, (1000000 / source_clock_period));
            ccfifo_write(EMC_PMACRO_DATA_RX_TERM_MODE, current_timing->burst_regs[EMC_PMACRO_DATA_RX_TERM_MODE_INDEX], 0);
        }
        
        ccfifo_write(EMC_DBG, emc_dbg_o, 0);

        if (opt_zcal_en_cc) {
            ccfifo_write(EMC_ZQ_CAL, 2 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
            ccfifo_write(EMC_ZQ_CAL, 2 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_LATCH_CMD, (1000000 / source_clock_period));
            
            if (dram_dev_num == TWO_RANK) {
                if (shared_zq_resistor) {
                    if (!(train_ca || train_ca_vref) || train_swap_rank) {
                        ccfifo_write(EMC_ZQ_CAL, 1 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                        ccfifo_write(EMC_ZQ_CAL, 1 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_LATCH_CMD, (1000000 / source_clock_period));
                        
                        if (!(train_ca || train_ca_vref))
                            ccfifo_write(EMC_MRW3, ((mr13_flip_fspop ^ 0xC0) & 0xF3FFFFF7) | 0xC000000, 0);
                    }

                    ccfifo_write(EMC_SELF_REF, 0x100, 0);
                    skip_zqcal = true;
                } else {
                    if ((train_ca || train_ca_vref) && !train_swap_rank) {
                        ccfifo_write(EMC_SELF_REF, 0x100, 0);
                        skip_zqcal = true;
                    } else {
                        ccfifo_write(EMC_ZQ_CAL, 1 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                        ccfifo_write(EMC_ZQ_CAL, 1 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_LATCH_CMD, (1000000 / source_clock_period));
                    }
                }
            }
        }

        if (!skip_zqcal) {
            if (!(train_ca || train_ca_vref))
                ccfifo_write(EMC_MRW3, ((mr13_flip_fspop ^ 0xC0) & 0xF3FFFFF7) | 0xC000000, 0);

            ccfifo_write(EMC_SELF_REF, 0x100, 0);
        }
    }
    
    if (!skip_zqcal) {
        /* Step 17:
         *   MANSR exit self refresh.
         */
        print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 17\n");
        
        if ((opt_dvfs_mode == MAN_SR) && (dram_type != DRAM_TYPE_LPDDR4))
            ccfifo_write(EMC_SELF_REF, 0, 0);

        /* Step 18:
         *   Send MRWs to LPDDR3/DDR3.
         */
        print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 18\n");
        
        if (dram_type == DRAM_TYPE_LPDDR2) {
            ccfifo_write(EMC_MRW2, next_timing->emc_mrw2, 0);
            ccfifo_write(EMC_MRW, next_timing->emc_mrw, 0);
            
            if (is_lpddr3) {
                ccfifo_write(EMC_MRW4, next_timing->emc_mrw4, 0);
            }
        } else if (dram_type == DRAM_TYPE_DDR3) {
            if (opt_dll_mode == DLL_ON) {
                ccfifo_write(EMC_EMRS, next_timing->emc_emrs & ~EMC_EMRS_USE_EMRS_LONG_CNT, 0);
            }
            ccfifo_write(EMC_EMRS2, next_timing->emc_emrs2 & ~EMC_EMRS2_USE_EMRS2_LONG_CNT, 0);
            ccfifo_write(EMC_MRS, next_timing->emc_mrs | EMC_EMRS_USE_EMRS_LONG_CNT, 0);
        }

        /* Step 19:
         *   ZQCAL for LPDDR3/DDR3
         */
        print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 19\n");
        
        if (opt_zcal_en_cc) {
            if (dram_type == DRAM_TYPE_LPDDR2) {
                uint32_t r;
                uint32_t zq_op = opt_cc_short_zcal ? 0x56 : 0xAB;
                uint32_t zcal_wait_time_ps = opt_cc_short_zcal ? 90000 : 360000;
                uint32_t zcal_wait_time_clocks = div_o3(zcal_wait_time_ps, destination_clock_period);
                r = zcal_wait_time_clocks << EMC_MRS_WAIT_CNT2_MRS_EXT2_WAIT_CNT_SHIFT | zcal_wait_time_clocks << EMC_MRS_WAIT_CNT2_MRS_EXT1_WAIT_CNT_SHIFT;
                
                ccfifo_write(EMC_MRS_WAIT_CNT2, r, 0);
                ccfifo_write(EMC_MRW, 2 << EMC_MRW_MRW_DEV_SELECTN_SHIFT |  EMC_MRW_USE_MRW_EXT_CNT | 10 << EMC_MRW_MRW_MA_SHIFT | zq_op << EMC_MRW_MRW_OP_SHIFT, 0);
                
                if (dram_dev_num == TWO_RANK) {
                    r = 1 << EMC_MRW_MRW_DEV_SELECTN_SHIFT | EMC_MRW_USE_MRW_EXT_CNT | 10 << EMC_MRW_MRW_MA_SHIFT | zq_op << EMC_MRW_MRW_OP_SHIFT;
                    ccfifo_write(EMC_MRW, r, 0);
                }
            } else if (dram_type == DRAM_TYPE_DDR3) {
                uint32_t zq_op = opt_cc_short_zcal ? 0 : EMC_ZQ_CAL_LONG;
                ccfifo_write(EMC_ZQ_CAL, zq_op | 2 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                
                if (dram_dev_num == TWO_RANK) {
                    ccfifo_write(EMC_ZQ_CAL, zq_op | 1 << EMC_ZQ_CAL_DEV_SEL_SHIFT | EMC_ZQ_CAL_ZQ_CAL_CMD, 0);
                }
            }
        }
    }
    
    if (bg_regulator_mode_change) {
        emc_set_shadow_bypass(ACTIVE);
        
        uint32_t bg_regulator_switch_complete_wait_clks = ramp_up_wait > 1250000 ? 0 : (1250000 - ramp_up_wait) / destination_clock_period;
        
        if (dvfs_with_training) {
            bg_regulator_switch_complete_wait_clks = (1250000 / source_clock_period);
            ccfifo_write(EMC_PMACRO_BG_BIAS_CTRL_0, current_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX], bg_regulator_switch_complete_wait_clks);
        } else {
            ccfifo_write(EMC_PMACRO_BG_BIAS_CTRL_0, next_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX], bg_regulator_switch_complete_wait_clks);
        }
        
        emc_set_shadow_bypass(ASSEMBLY);
    }

    /* Step 20:
     *   Issue ref and optional QRST.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 20\n");
    
    if (dvfs_with_training || (dram_type != DRAM_TYPE_LPDDR4))
        ccfifo_write(EMC_REF, 0, 0);

    if (opt_do_sw_qrst) {
        ccfifo_write(EMC_ISSUE_QRST, 1, 0);
        ccfifo_write(EMC_ISSUE_QRST, 0, 2);
    }

    /* Step 21:
     *   Restore ZCAL and ZCAL interval.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 21\n");
    
    if (save_restore_clkstop_pd || opt_zcal_en_cc) {
        emc_set_shadow_bypass(ACTIVE);
        
        if (opt_zcal_en_cc) {
            if (dvfs_with_training) {
                ccfifo_write(EMC_ZCAL_INTERVAL, current_timing->burst_regs[EMC_ZCAL_INTERVAL_INDEX], 0);
            } else if (dram_type != DRAM_TYPE_LPDDR4) {
                ccfifo_write(EMC_ZCAL_INTERVAL, next_timing->burst_regs[EMC_ZCAL_INTERVAL_INDEX], 0);
            }
        }
        
        if (save_restore_clkstop_pd) {
            ccfifo_write(EMC_CFG, next_timing->burst_regs[EMC_CFG_INDEX] & ~EMC_CFG_DYN_SELF_REF, 0);
        }
        
        if (dvfs_with_training && (dram_type == DRAM_TYPE_LPDDR4)) {
            ccfifo_write(EMC_SEL_DPD_CTRL, current_timing->emc_sel_dpd_ctrl, 0);
        }
        
        emc_set_shadow_bypass(ASSEMBLY);
    }
    
    /* Step 22:
     *   Restore EMC_CFG_PIPE_CLK.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 22\n");
    
    ccfifo_write(EMC_CFG_PIPE_CLK, emc_cfg_pipe_clk_o, 0);

    if (bg_regulator_mode_change) {
        if (enable_bg_regulator) {
            emc_write(next_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & ~EMC_PMACRO_BG_BIAS_CTRL_0_BGLP_E_PWRD, EMC_PMACRO_BG_BIAS_CTRL_0);
        } else {
            emc_write(next_timing->burst_regs[EMC_PMACRO_BG_BIAS_CTRL_0_INDEX] & ~EMC_PMACRO_BG_BIAS_CTRL_0_BG_E_PWRD, EMC_PMACRO_BG_BIAS_CTRL_0);
        }
    }

    /* Step 23:
     *   Do clock change.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 23\n");
    
    if (dvfs_with_training) {
        car->clk_source_emc_safe = car->clk_source_emc;
        change_dll_src(current_timing, car->clk_source_emc);
    }
    
    uint32_t cfg_dig_dll_tmp = emc_read(EMC_CFG_DIG_DLL);
    cfg_dig_dll_tmp |= EMC_CFG_DIG_DLL_CFG_DLL_STALL_ALL_TRAFFIC;
    cfg_dig_dll_tmp &= ~EMC_CFG_DIG_DLL_CFG_DLL_STALL_RW_UNTIL_LOCK;
    cfg_dig_dll_tmp &= ~EMC_CFG_DIG_DLL_CFG_DLL_STALL_ALL_UNTIL_LOCK;
    cfg_dig_dll_tmp &= ~EMC_CFG_DIG_DLL_CFG_DLL_EN;
    cfg_dig_dll_tmp = (cfg_dig_dll_tmp & ~EMC_CFG_DIG_DLL_CFG_DLL_MODE_MASK) | (2 << EMC_CFG_DIG_DLL_CFG_DLL_MODE_SHIFT);
    emc_write(cfg_dig_dll_tmp, EMC_CFG_DIG_DLL);

    car->clk_source_emc = next_clk_src;
    wait_for_update(EMC_INTSTATUS, EMC_INTSTATUS_CLKCHANGE_COMPLETE, true, REG_EMC);
    
    /* Step 24:
     *   Save training results.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 24\n");
    
    if (dvfs_with_training) {
        uint32_t emc_dbg_tmp = emc_read(EMC_DBG);
        emc_write(emc_dbg_tmp | 1, EMC_DBG);       /* Set READ_MUX to ASSEMBLY. */

        /* Save CA results. */
        if (train_ca) {
            next_timing->trim_perch_regs[REG_EMC0_EMC_CMD_BRLSHFT_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_CMD_BRLSHFT_0);
            next_timing->trim_perch_regs[REG_EMC1_EMC_CMD_BRLSHFT_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_CMD_BRLSHFT_1): 0;
            next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4);
            next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1,EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5) : 0;

            if (train_self_refresh) {
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2_INDEX] = emc_read(EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2);
            }
        }

        /* Save CA_VREF results. */
        if (train_ca_vref) {
            next_timing->burst_reg_per_ch[REG_EMC0_EMC_MRW10_INDEX] = (emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_CA_VREF) & 0xFFFF) | 0x880C0000;
            next_timing->burst_reg_per_ch[REG_EMC1_EMC_MRW10_INDEX] = (channel_mode ? emc_read_per_ch(REG_EMC1, EMC_TRAINING_OPT_CA_VREF) & 0xFFFF : 0) | 0x880C0000;

            if (dram_dev_num == TWO_RANK) {
                next_timing->burst_reg_per_ch[REG_EMC0_EMC_MRW11_INDEX] = ((emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_CA_VREF) >> 16) & 0xFF) | (emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_CA_VREF) >> 24 << 8) | (0x480C0000 & 0xFFFFFF00);
                next_timing->burst_reg_per_ch[REG_EMC1_EMC_MRW11_INDEX] = (((channel_mode ? emc_read_per_ch(REG_EMC1, EMC_TRAINING_OPT_CA_VREF) : 0) >> 16) & 0xFF) | ((channel_mode ? emc_read_per_ch(REG_EMC1, EMC_TRAINING_OPT_CA_VREF) : 0) >> 24 << 8) | (0x480C0000 & 0xFFFFFF00);
            } else {
                next_timing->burst_reg_per_ch[REG_EMC0_EMC_MRW11_INDEX] = ((emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_CA_VREF) >> 16) & 0xFF) | (emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_CA_VREF) >> 24 << 8) | (0xC80C0000 & 0xFFFFFF00);

                next_timing->burst_reg_per_ch[REG_EMC1_EMC_MRW11_INDEX] = (((channel_mode ? emc_read_per_ch(REG_EMC1, EMC_TRAINING_OPT_CA_VREF) : 0) >> 16) & 0xFF) | ((channel_mode ? emc_read_per_ch(REG_EMC1, EMC_TRAINING_OPT_CA_VREF) : 0) >> 24 << 8) | (0xC80C0000 & 0xFFFFFF00);
            }
        }

        /* Save QUSE results. */
        if (train_quse || train_rd) {
            next_timing->trim_perch_regs[REG_EMC0_EMC_QUSE_BRLSHFT_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_QUSE_BRLSHFT_0);
            next_timing->trim_perch_regs[REG_EMC1_EMC_QUSE_BRLSHFT_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_QUSE_BRLSHFT_1) : 0;

            next_timing->trim_regs[EMC_PMACRO_QUSE_DDLL_RANK0_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_QUSE_DDLL_RANK0_0);
            next_timing->trim_regs[EMC_PMACRO_QUSE_DDLL_RANK0_1_INDEX]= emc_read_per_ch(REG_EMC0, EMC_PMACRO_QUSE_DDLL_RANK0_1);
            next_timing->trim_regs[EMC_PMACRO_QUSE_DDLL_RANK0_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_QUSE_DDLL_RANK0_2) : 0;
            next_timing->trim_regs[EMC_PMACRO_QUSE_DDLL_RANK0_3_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_QUSE_DDLL_RANK0_3) : 0;

            if (dram_dev_num == TWO_RANK) {
                next_timing->trim_perch_regs[REG_EMC0_EMC_QUSE_BRLSHFT_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_QUSE_BRLSHFT_2);
                next_timing->trim_perch_regs[REG_EMC1_EMC_QUSE_BRLSHFT_3_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_QUSE_BRLSHFT_3) : 0;

                next_timing->trim_regs[EMC_PMACRO_QUSE_DDLL_RANK1_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_QUSE_DDLL_RANK1_0);
                next_timing->trim_regs[EMC_PMACRO_QUSE_DDLL_RANK1_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_QUSE_DDLL_RANK1_1);
                next_timing->trim_regs[EMC_PMACRO_QUSE_DDLL_RANK1_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_QUSE_DDLL_RANK1_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_QUSE_DDLL_RANK1_3_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_QUSE_DDLL_RANK1_3) : 0;
            }
        }

        /* Save QUSE_VREF results. */
        if (train_quse_vref) {
            if (dram_dev_num == TWO_RANK) {
                uint32_t emc0_opt_dqs_array[4] = {0};
                uint32_t emc1_opt_dqs_array[4] = {0};
                uint32_t emc1_training_opt_dqs_ib_vref_rank0_val = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_TRAINING_OPT_DQS_IB_VREF_RANK0) : 0;
                uint32_t emc1_training_opt_dqs_ib_vref_rank1_val = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_TRAINING_OPT_DQS_IB_VREF_RANK1) : 0;

                for (int i = 0; i < 4; i++) {
                    emc0_opt_dqs_array[i] = (emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQS_IB_VREF_RANK0) >> (8 * i)) & 0xFF;
                    emc1_opt_dqs_array[i] = (emc1_training_opt_dqs_ib_vref_rank0_val >> (8 * i)) & 0xFF;
                }

                uint32_t ib_vref_dqs_0 = 0;
                uint32_t ib_vref_dqs_1 = 0;
                for (int i = 0; i < 4; i++)
                {
                    ib_vref_dqs_0 |= (emc0_opt_dqs_array[i] + ((emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQS_IB_VREF_RANK1) >> (8 * i)) & 0xFF)) >> 1 << (8 * i);
                    ib_vref_dqs_1 |= (emc1_opt_dqs_array[i] + ((emc1_training_opt_dqs_ib_vref_rank1_val >> (8 * i)) & 0xFF)) >> 1 << (8 * i);
                }

                next_timing->trim_regs[EMC_PMACRO_IB_VREF_DQS_0_INDEX] = ib_vref_dqs_0;
                next_timing->trim_regs[EMC_PMACRO_IB_VREF_DQS_1_INDEX] = ib_vref_dqs_1;
            }
            else
            {
                next_timing->trim_regs[EMC_PMACRO_IB_VREF_DQS_0_INDEX] = emc_read(EMC_PMACRO_IB_VREF_DQS_0);
                next_timing->trim_regs[EMC_PMACRO_IB_VREF_DQS_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_VREF_DQS_1) : 0;
            }
        }

        /* Save RD results. */
        if (train_rd) {
            next_timing->trim_regs[EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0);
            next_timing->trim_regs[EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1);
            next_timing->trim_regs[EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2_INDEX]= channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2) : 0;
            next_timing->trim_regs[EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3) : 0;

            if (dram_dev_num == TWO_RANK) {
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3_INDEX]  = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3) : 0;
            }

            if (train_self_refresh) {
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2);
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1) : 0;
                next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2) : 0;

                if (dram_dev_num == TWO_RANK) {
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2);
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1) : 0;
                    next_timing->trim_regs[EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2) : 0;
                }
            }

            /* Save RD_VREF results. */
            if (train_rd_vref) {
                uint8_t ib_vref_dq_byte0_icr = (emc_read(EMC_PMACRO_IB_VREF_DQ_0) & 0x7F) + (next_timing->save_restore_mod_regs[0] & 0x7F);
                if (next_timing->save_restore_mod_regs[0] & 0x80000000)
                    ib_vref_dq_byte0_icr = (emc_read(EMC_PMACRO_IB_VREF_DQ_0) & 0x7F) - (next_timing->save_restore_mod_regs[0] & 0x7F);

                uint8_t ib_vref_dq_byte1_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_0) >> 8) & 0x7F) + (next_timing->save_restore_mod_regs[1] & 0x7F);
                if (next_timing->save_restore_mod_regs[1] & 0x80000000)
                    ib_vref_dq_byte1_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_0) >> 8) & 0x7F) - (next_timing->save_restore_mod_regs[1] & 0x7F);

                uint8_t ib_vref_dq_byte2_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_0) >> 16) & 0x7F) + (next_timing->save_restore_mod_regs[2] & 0x7F);
                if (next_timing->save_restore_mod_regs[2] & 0x80000000)
                    ib_vref_dq_byte2_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_0) >> 16) & 0x7F) - (next_timing->save_restore_mod_regs[2] & 0x7F);

                uint8_t ib_vref_dq_byte3_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_0) >> 24) & 0x7F) + (next_timing->save_restore_mod_regs[3] & 0x7F);
                if (next_timing->save_restore_mod_regs[3] & 0x80000000)
                    ib_vref_dq_byte3_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_0) >> 24) & 0x7F) - (next_timing->save_restore_mod_regs[3] & 0x7F);

                next_timing->trim_regs[EMC_PMACRO_IB_VREF_DQ_0_INDEX] = ((ib_vref_dq_byte0_icr & 0x7F) | (ib_vref_dq_byte1_icr & 0x7F) << 8) | ((ib_vref_dq_byte2_icr & 0x7F) << 16) | ((ib_vref_dq_byte3_icr & 0x7F) << 24);
                
                uint8_t ib_vref_dq_byte4_icr = (emc_read(EMC_PMACRO_IB_VREF_DQ_1) & 0x7F) + (next_timing->save_restore_mod_regs[4] & 0x7F);
                if (next_timing->save_restore_mod_regs[4] & 0x80000000)
                    ib_vref_dq_byte4_icr = (emc_read(EMC_PMACRO_IB_VREF_DQ_1) & 0x7F) - (next_timing->save_restore_mod_regs[4] & 0x7F);

                uint8_t ib_vref_dq_byte5_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_1) >> 8) & 0x7F) + (next_timing->save_restore_mod_regs[5] & 0x7F);
                if (next_timing->save_restore_mod_regs[5] & 0x80000000)
                    ib_vref_dq_byte5_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_1) >> 8) & 0x7F) - (next_timing->save_restore_mod_regs[5] & 0x7F);

                uint8_t ib_vref_dq_byte6_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_1) >> 16) & 0x7F) + (next_timing->save_restore_mod_regs[6] & 0x7F);
                if (next_timing->save_restore_mod_regs[6] & 0x80000000)
                    ib_vref_dq_byte6_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_1) >> 16) & 0x7F) - (next_timing->save_restore_mod_regs[6] & 0x7F);

                uint8_t ib_vref_dq_byte7_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_1) >> 24) & 0x7F) + (next_timing->save_restore_mod_regs[7] & 0x7F);
                if (next_timing->save_restore_mod_regs[7] & 0x80000000)
                    ib_vref_dq_byte7_icr = ((emc_read(EMC_PMACRO_IB_VREF_DQ_1) >> 24) & 0x7F) - (next_timing->save_restore_mod_regs[7] & 0x7F);

                next_timing->trim_regs[EMC_PMACRO_IB_VREF_DQ_1_INDEX] = ((ib_vref_dq_byte4_icr & 0x7F) | (ib_vref_dq_byte5_icr & 0x7F) << 8) | ((ib_vref_dq_byte6_icr & 0x7F) << 16) | ((ib_vref_dq_byte7_icr & 0x7F) << 24);
            }
        }

        /* Save WR results. */
        if (train_wr) {
            next_timing->trim_perch_regs[REG_EMC0_EMC_DATA_BRLSHFT_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_DATA_BRLSHFT_0);
            next_timing->trim_perch_regs[REG_EMC1_EMC_DATA_BRLSHFT_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_DATA_BRLSHFT_0) : 0;

            if (dram_dev_num == TWO_RANK) {
                next_timing->trim_perch_regs[REG_EMC0_EMC_DATA_BRLSHFT_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_DATA_BRLSHFT_1);
                next_timing->trim_perch_regs[REG_EMC1_EMC_DATA_BRLSHFT_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_DATA_BRLSHFT_1) : 0;
            }

            next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0);
            next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1);
            next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2) : 0;
            next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3) : 0;

            if (dram_dev_num == TWO_RANK) {
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3) : 0;
            }

            if (train_self_refresh) {
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2);
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1) : 0;
                next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2) : 0;

                if (dram_dev_num == TWO_RANK) {
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2_INDEX] = emc_read_per_ch(REG_EMC0, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2);
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1) : 0;
                    next_timing->trim_regs[EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2_INDEX] = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2) : 0;
                }
            }

            /* Save WR_VREF results. */
            if (train_wr_vref) {
                uint32_t emc1_ranks_sub_partitions = channel_mode ? emc_read_per_ch(REG_EMC1, EMC_TRAINING_OPT_DQ_OB_VREF) : 0;

                uint8_t emc0_ib_vref_dq_byte8_modded_plus = next_timing->save_restore_mod_regs[8] + emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQ_OB_VREF);
                if (next_timing->save_restore_mod_regs[8] & 0x80000000)
                    emc0_ib_vref_dq_byte8_modded_plus = emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQ_OB_VREF) - next_timing->save_restore_mod_regs[8];

                uint8_t emc0_mrw12_op_sp1 = ((emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQ_OB_VREF) & 0xFFFF) >> 8) + next_timing->save_restore_mod_regs[9];
                if (next_timing->save_restore_mod_regs[9] & 0x80000000)
                    emc0_mrw12_op_sp1 = ((emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQ_OB_VREF) & 0xFFFF) >> 8) - next_timing->save_restore_mod_regs[9];

                uint8_t emc0_mrw13_op_sp0 = ((emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQ_OB_VREF) >> 16) & 0xFF) + next_timing->save_restore_mod_regs[8];
                if (next_timing->save_restore_mod_regs[8] & 0x80000000)
                    emc0_mrw13_op_sp0 = ((emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQ_OB_VREF) >> 16) & 0xFF) - next_timing->save_restore_mod_regs[8];

                uint8_t emc0_ib_vref_dq_byte9_modded_a_plus = next_timing->save_restore_mod_regs[9] + (emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQ_OB_VREF) >> 24);
                if (next_timing->save_restore_mod_regs[9] & 0x80000000)
                    emc0_ib_vref_dq_byte9_modded_a_plus = (emc_read_per_ch(REG_EMC0, EMC_TRAINING_OPT_DQ_OB_VREF) >> 24) - (uint8_t)next_timing->save_restore_mod_regs[9];

                uint8_t emc0_ib_vref_dq_byte10_modded_plus = emc1_ranks_sub_partitions + next_timing->save_restore_mod_regs[10];
                if (next_timing->save_restore_mod_regs[10] & 0x80000000)
                    emc0_ib_vref_dq_byte10_modded_plus = emc1_ranks_sub_partitions - next_timing->save_restore_mod_regs[10];

                uint8_t emc0_ib_vref_dq_byte11_modded_plus = ((emc1_ranks_sub_partitions & 0xFFFF) >> 8) + next_timing->save_restore_mod_regs[11];
                if (next_timing->save_restore_mod_regs[11] & 0x80000000)
                    emc0_ib_vref_dq_byte11_modded_plus = ((emc1_ranks_sub_partitions & 0xFFFF) >> 8) - next_timing->save_restore_mod_regs[11];

                uint8_t emc1_mrw13_op_sp0 = ((emc1_ranks_sub_partitions >> 16) & 0xFF) + next_timing->save_restore_mod_regs[10];
                if (next_timing->save_restore_mod_regs[10] & 0x80000000)
                    emc1_mrw13_op_sp0 = ((emc1_ranks_sub_partitions >> 16) & 0xFF) - next_timing->save_restore_mod_regs[10];

                uint8_t emc1_mrw13_op_sp1 = (emc1_ranks_sub_partitions >> 24) + next_timing->save_restore_mod_regs[11];
                if (next_timing->save_restore_mod_regs[11] & 0x80000000)
                    emc1_mrw13_op_sp1 = (emc1_ranks_sub_partitions >> 24) - next_timing->save_restore_mod_regs[11];

                next_timing->burst_reg_per_ch[REG_EMC1_EMC_MRW12_INDEX] = (uint8_t)emc0_ib_vref_dq_byte10_modded_plus | 0x880E0000 | (emc0_ib_vref_dq_byte11_modded_plus << 8);
                next_timing->burst_reg_per_ch[REG_EMC0_EMC_MRW12_INDEX]  = emc0_ib_vref_dq_byte8_modded_plus | 0x880E0000 | (emc0_mrw12_op_sp1 << 8);
                
                if (dram_dev_num == TWO_RANK) {
                    next_timing->burst_reg_per_ch[REG_EMC0_EMC_MRW13_INDEX] = emc0_ib_vref_dq_byte9_modded_a_plus << 8 | emc0_mrw13_op_sp0 | 0x480E0000;
                    next_timing->burst_reg_per_ch[REG_EMC1_EMC_MRW13_INDEX] = (emc1_mrw13_op_sp1 << 8) | emc1_mrw13_op_sp0 | 0x480E0000;
                } else {
                    next_timing->burst_reg_per_ch[REG_EMC0_EMC_MRW13_INDEX] = emc0_ib_vref_dq_byte9_modded_a_plus << 8 | emc0_mrw13_op_sp0 | 0xC80E0000;
                    next_timing->burst_reg_per_ch[REG_EMC1_EMC_MRW13_INDEX] = (emc1_mrw13_op_sp1 << 8) | emc1_mrw13_op_sp0 | 0xC80E0000;
                }
            }
        }
        
        emc_write(emc_dbg_tmp, EMC_DBG);
    }

    /* Step 25:
     *   Program MC updown registers.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 25\n");
    
    if ((next_timing->rate > current_timing->rate) && !dvfs_with_training) {
        for (int i = 0; i < next_timing->num_up_down; i++) {
            mc_write(next_timing->la_scale_regs[i], la_scale_regs_off[i]);
        }
        
        /* Request a timing update. */
        emc_timing_update(channel_mode);
    }

    /* Step 26:
     *   Restore ZCAL registers.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 26\n");
    
    if (dram_type == DRAM_TYPE_LPDDR4) {
        emc_set_shadow_bypass(ACTIVE);
        emc_write(next_timing->burst_regs[EMC_ZCAL_WAIT_CNT_INDEX], EMC_ZCAL_WAIT_CNT);
        emc_write(next_timing->burst_regs[EMC_ZCAL_INTERVAL_INDEX], EMC_ZCAL_INTERVAL);
        emc_set_shadow_bypass(ASSEMBLY);
    }

    if ((dram_type != DRAM_TYPE_LPDDR4)
        && opt_zcal_en_cc
        && !opt_short_zcal
        && opt_cc_short_zcal)
    {
        udelay(2);

        emc_set_shadow_bypass(ACTIVE);
        if (dram_type == DRAM_TYPE_LPDDR2) {
            emc_write(next_timing->burst_regs[EMC_MRS_WAIT_CNT_INDEX], EMC_MRS_WAIT_CNT);
        } else if (dram_type == DRAM_TYPE_DDR3) {
            emc_write(next_timing->burst_regs[EMC_ZCAL_WAIT_CNT_INDEX], EMC_ZCAL_WAIT_CNT);
        }
        emc_set_shadow_bypass(ASSEMBLY);
    }

    /* Step 27:
     *   Restore EMC_CFG, FDPD registers.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 27\n");
    
    emc_set_shadow_bypass(ACTIVE);
    emc_write(next_timing->burst_regs[EMC_CFG_INDEX], EMC_CFG);
    emc_set_shadow_bypass(ASSEMBLY);
    emc_write(next_timing->emc_fdpd_ctrl_cmd_no_ramp, EMC_FDPD_CTRL_CMD_NO_RAMP);
    emc_write(next_timing->emc_sel_dpd_ctrl, EMC_SEL_DPD_CTRL);

    /* Step 28:
     *   Training recover.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 28\n");
    
    if (dvfs_with_training && (dram_type == DRAM_TYPE_LPDDR4)) {
        emc_set_shadow_bypass(ACTIVE);
        emc_write(next_timing->burst_regs[EMC_CFG_INDEX], EMC_CFG);
        emc_write(next_timing->emc_sel_dpd_ctrl, EMC_SEL_DPD_CTRL);
        emc_write(current_timing->burst_regs[EMC_ZCAL_WAIT_CNT_INDEX], EMC_ZCAL_WAIT_CNT);
        emc_write(current_timing->burst_regs[EMC_ZCAL_INTERVAL_INDEX], EMC_ZCAL_INTERVAL);
        emc_write(current_timing->emc_auto_cal_config2, EMC_AUTO_CAL_CONFIG2);
        emc_write(current_timing->emc_auto_cal_config3, EMC_AUTO_CAL_CONFIG3);
        emc_write(current_timing->emc_auto_cal_config4, EMC_AUTO_CAL_CONFIG4);
        emc_write(current_timing->emc_auto_cal_config5, EMC_AUTO_CAL_CONFIG5);
        emc_write(current_timing->emc_auto_cal_config6, EMC_AUTO_CAL_CONFIG6);
        emc_write(current_timing->emc_auto_cal_config7, EMC_AUTO_CAL_CONFIG7);
        emc_write(current_timing->emc_auto_cal_config8, EMC_AUTO_CAL_CONFIG8);
        emc_set_shadow_bypass(ASSEMBLY);
        emc_write(next_timing->burst_regs[EMC_TR_DVFS_INDEX] & ~EMC_TR_DVFS_TRAINING_DVFS, EMC_TR_DVFS);
    }

    emc_set_shadow_bypass(ACTIVE);
    emc_write(next_timing->burst_regs[EMC_PMACRO_AUTOCAL_CFG_COMMON_INDEX], EMC_PMACRO_AUTOCAL_CFG_COMMON);
    emc_set_shadow_bypass(ASSEMBLY);

    /* Step 29:
     *   Power fix WAR.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 29\n");
    
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

    /* Step 30:
     *   Re-enable autocal.
     */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - step 30\n");
    
    if (dvfs_with_training) {
        emc_auto_cal_config = current_timing->emc_auto_cal_config;
        
        /* Restore FSP to account for switch back. Only needed in training. */
        g_fsp_for_next_freq = !g_fsp_for_next_freq;
    } else {
        emc_auto_cal_config = next_timing->emc_auto_cal_config;
        
        if (next_timing->burst_regs[EMC_CFG_DIG_DLL_INDEX] & EMC_CFG_DIG_DLL_CFG_DLL_EN) {
            dll_enable_stall(channel_mode);
        }
    }
    emc_write(emc_auto_cal_config, EMC_AUTO_CAL_CONFIG);
    
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Clock set - done!\n");
}

static void do_periodic_emc_compensation(tegra_emc_timing_t* current_timing) {
    uint32_t reg_count = 10;
    uint32_t reg_list[] = {
        EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0,
        EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1,
        EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2,
        EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3,
        EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0,
        EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1,
        EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2,
        EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3,
        EMC_DATA_BRLSHFT_0,
        EMC_DATA_BRLSHFT_1
    };
    
    if (current_timing->periodic_training) {
        int dram_dev_num = ((mc_read(MC_EMEM_ADR_CFG) & 1) + 1);
        bool channel_mode = ((current_timing->burst_regs[EMC_FBIO_CFG7_INDEX] >> 2) & 1);

        uint32_t emc_cfg_o = emc_read(EMC_CFG);
        uint32_t emc_cfg = emc_cfg_o & ~(EMC_CFG_DYN_SELF_REF | EMC_CFG_DRAM_ACPD | EMC_CFG_DRAM_CLKSTOP_PD | EMC_CFG_DRAM_CLKSTOP_PD);

        /*
         * 1. Power optimizations should be off.
         */
        emc_write(emc_cfg, EMC_CFG);

        /* Does emc_timing_update() for above changes. */
        dll_disable(channel_mode);

        if (dram_dev_num == TWO_RANK) {
            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_DRAM_IN_POWERDOWN_MASK, false, REG_EMC);
            if (channel_mode)
                wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_DRAM_IN_POWERDOWN_MASK, false, REG_EMC1);
        } else {
            wait_for_update(EMC_EMC_STATUS, 0x10, false, REG_EMC);
            if (channel_mode)
                wait_for_update(EMC_EMC_STATUS, 0x10, false, REG_EMC1);
        }

        wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_DRAM_IN_SELF_REFRESH_MASK, false, REG_EMC);
        if (channel_mode)
            wait_for_update(EMC_EMC_STATUS, EMC_EMC_STATUS_DRAM_IN_SELF_REFRESH_MASK, false, REG_EMC1);
        
        wait_for_update(EMC_EMC_STATUS, 0x01, false, REG_EMC);
        if (channel_mode)
            wait_for_update(EMC_EMC_STATUS, 0x01, false, REG_EMC1);

        uint32_t emc_cfg_update = emc_read(EMC_CFG_UPDATE);
        emc_write((emc_cfg_update & ~EMC_CFG_UPDATE_UPDATE_DLL_IN_UPDATE_MASK) | (2 << EMC_CFG_UPDATE_UPDATE_DLL_IN_UPDATE_SHIFT), EMC_CFG_UPDATE);

        /*
         * 2. osc kick off - this assumes training and dvfs have set
         *    correct MR23.
         */
        start_periodic_compensation();

        /*
         * 3. Let dram capture its clock tree delays.
         */
        udelay((actual_osc_clocks(current_timing->run_clocks) * 1000) / current_timing->rate + 1);

        /*
         * 4. Check delta wrt previous values (save value if margin
         *    exceeds what is set in table).
         */
        uint32_t del = periodic_compensation_handler(current_timing, current_timing, dram_dev_num, channel_mode, PERIODIC_TRAINING_SEQUENCE);

        /*
         * 5. Apply compensation w.r.t. trained values (if clock tree
         *    has drifted more than the set margin).
         */
        if (current_timing->tree_margin < ((del * 128 * (current_timing->rate / 1000)) / 1000000)) {
            for (int i = 0; i < reg_count; i++) {
                uint32_t tmp = apply_periodic_compensation_trimmer(current_timing, reg_list[i]);
                emc_write(tmp, reg_list[i]);
            }
        }

        emc_write(emc_cfg_o, EMC_CFG);

        /*
         * 6. Timing update actually applies the new trimmers.
         */
        emc_timing_update(channel_mode);

        /* 6.1. Restore the UPDATE_DLL_IN_UPDATE field. */
        emc_write(emc_cfg_update, EMC_CFG_UPDATE);

        /* 6.2. Restore the DLL. */
        dll_enable(channel_mode);
    }
}

static void train_set_clock(tegra_emc_timing_t* current_timing, tegra_emc_timing_t* next_timing, bool update_clk, uint32_t next_clk_src) {
    /* Check for dual channel LPDDR4 */
    bool dual_channel_lpddr4_case = ((emc_read(EMC_FBIO_CFG7) & EMC_FBIO_CFG7_CH0_ENABLE) & (emc_read(EMC_FBIO_CFG7) & EMC_FBIO_CFG7_CH1_ENABLE));
    
    /* Get the DRAM type */
    uint32_t dram_type = (next_timing->burst_regs[EMC_FBIO_CFG5_INDEX] & 0x03);
    
    if (g_write_training_pattern) {
        print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Writing training patterns...\n");
        
        /* Write the training data into pattern RAM */
        for (int i = 0; i < 0x100; i++) {
            uint32_t training_pattern_val = g_ram_pattern_dq[i + (next_timing->training_pattern * 0x100)];
            
            /* Write the DQ data into pattern RAM */
            emc_write(training_pattern_val, EMC_TRAINING_PATRAM_DQ);
            
            training_pattern_val = g_ram_pattern_dmi[i + (next_timing->training_pattern * 0x100)];
            
            /* Write the DMI data into pattern RAM */
            emc_write(training_pattern_val & 0x0F, EMC_TRAINING_PATRAM_DMI);
            
            /* Enable writing into pattern RAM and select the offset */
            emc_write(0x80000000 + i, EMC_TRAINING_PATRAM_CTRL);
        }
    }
    
    /* Only write the training pattern once */
    g_write_training_pattern = false;
    
    if (next_timing->needs_training && !next_timing->trained) {
        uint32_t training_flags = next_timing->needs_training;
        
        /* Read from MC_EMEM_ADR_CFG */
        uint32_t dram_dev_num = mc_read(MC_EMEM_ADR_CFG);
        
        int training_params_idx = 0;
        int training_params[8] = {};
        
        if (training_flags & 0x03) {
            training_params_idx = 1;
            training_params[0] = (training_flags & 0x203);
            
            if (dram_dev_num & 0x01) {
                training_params_idx = 2;
                training_params[1] = (training_flags & 0x303);
            }
        }
        
        if ((dram_dev_num & 0x01) && (training_flags & 0x0C)) {
            training_params[training_params_idx] = (training_flags & 0x20C);
            training_params[training_params_idx + 1] = (training_flags & 0x204);
            training_params_idx += 2;
        } else if (training_flags & 0x0C) {
            training_params[training_params_idx++] = (training_flags & 0x20C);
        }
        
        if (training_flags & 0xF0)
            training_params[training_params_idx++] = (training_flags & 0x2F0);
        
        for (int i = 0; i < training_params_idx; i++) {
            /* Adjust the clock */
            set_clock(current_timing, next_timing, training_params[i], next_clk_src);
            
            /* Change CFG_SWAP to ASSEMBLY_ONLY */
            uint32_t emc_dbg = emc_read(EMC_DBG);
            emc_dbg = ((emc_dbg & 0xF3FFFFFF) | 0x8000000);
            emc_write(emc_dbg, EMC_DBG);
            
            /* Change UPDATE_AUTO_CAL_IN_UPDATE to ALWAYS */
            uint32_t emc_cfg_update = emc_read(EMC_CFG_UPDATE);
            emc_cfg_update = ((emc_cfg_update & 0xFFFFFFF9) | 0x04);
            emc_write(emc_cfg_update, EMC_CFG_UPDATE);
            
            /* Request a timing update event */
            emc_timing_update(dual_channel_lpddr4_case);
            
            /* Change UPDATE_AUTO_CAL_IN_UPDATE to NEVER */
            emc_cfg_update = emc_read(EMC_CFG_UPDATE);
            emc_cfg_update &= 0xFFFFFFF9;
            emc_write(emc_cfg_update, EMC_CFG_UPDATE);
            
            /* Change CFG_SWAP to ACTIVE_ONLY */
            emc_dbg = emc_read(EMC_DBG);
            emc_dbg &= 0xF3FFFFFF;
            emc_write(emc_dbg, EMC_DBG);
            
            /* Disable DLL and change CFG_DLL_MODE to RUN_PERIODIC */
            uint32_t emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
            emc_cfg_dig_dll = ((emc_cfg_dig_dll & 0xFFFFFF3E) | 0x80);
            emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);
            
            /* Request a timing update event */
            emc_timing_update(dual_channel_lpddr4_case);
            
            /* Disable or enable DLL */
            emc_cfg_dig_dll = emc_read(EMC_CFG_DIG_DLL);
            if (next_timing->burst_regs[EMC_CFG_DIG_DLL_INDEX] == 0x01)
                emc_cfg_dig_dll |= 0x01;
            else
                emc_cfg_dig_dll &= 0xFFFFFFFE;
            
            /* Change CFG_DLL_MODE to RUN_PERIODIC */
            emc_cfg_dig_dll = ((emc_cfg_dig_dll & 0xFFFFFF3F) | 0x80);
            emc_write(emc_cfg_dig_dll, EMC_CFG_DIG_DLL);
            
            /* Request a timing update event */
            emc_timing_update(dual_channel_lpddr4_case);
            
            /* Wait for DLL_LOCK to be set */
            uint32_t emc_dig_dll_status = 0;
            do {
                emc_dig_dll_status = emc_read(EMC_DIG_DLL_STATUS);
            } while (!(emc_dig_dll_status & EMC_DIG_DLL_STATUS_DLL_LOCK));
            
            /* Check if DRAM is LPDDR4 */
            if (dram_type == DRAM_TYPE_LPDDR4) {
                emc_write(current_timing->burst_regs[EMC_RP_INDEX], EMC_RP);
                emc_write(current_timing->burst_regs[EMC_R2P_INDEX], EMC_R2P);
                emc_write(current_timing->burst_regs[EMC_W2P_INDEX], EMC_W2P);
                emc_write(current_timing->burst_regs[EMC_TRPAB_INDEX], EMC_TRPAB);
            }
            
            /* Request a timing update event */
            emc_timing_update(dual_channel_lpddr4_case);
        }
        
        /* We've finished training */
        next_timing->trained = 1;
        
        print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Memory is trained!\n");
    }
    
    /* Change the clock if requested */
    if (update_clk)
        set_clock(current_timing, next_timing, 0, next_clk_src);
}

static int train_one(int z_val, uint32_t next_rate, uint32_t current_rate, tegra_emc_timing_t* timing_tables, int timing_tables_count, TrainMode mode) {
    int current_timing_table_idx = -1;
    int next_timing_table_idx = -1;
    tegra_emc_timing_t* current_timing_table;
    tegra_emc_timing_t* next_timing_table;
    uint32_t next_clk_src = 0;
    
    /* Too many table entries */
    if (timing_tables_count > 0x384)
        return 4;
  
    /* Locate the right tables for the transition */
    for (int i = 0; i < timing_tables_count; i++) {
        uint32_t rate = timing_tables[i].rate;
        
        if (rate == current_rate)
            current_timing_table_idx = i;
        else if (rate == next_rate)
            next_timing_table_idx = i;
    }
    
    /* Failed to find the tables. */
    if ((current_timing_table_idx < 0) || (next_timing_table_idx < 0))
        return 4;
    
    current_timing_table = (tegra_emc_timing_t*)&timing_tables[current_timing_table_idx];
    next_timing_table = (tegra_emc_timing_t*)&timing_tables[next_timing_table_idx];

    uint32_t clk_src_emc_from = current_timing_table->clk_src_emc;
    uint32_t clk_src_emc_to = next_timing_table->clk_src_emc;
    uint32_t rate_from = current_timing_table->rate;
    uint32_t rate_to = next_timing_table->rate;
    
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Changing rate from %d to %d!\n", rate_from, rate_to);
    
    bool diff_freq = test_clk_ratio(rate_to, clk_src_emc_to, rate_from, clk_src_emc_from);
    
    if (diff_freq) {
        uint32_t emc_2x_clk_src = (clk_src_emc_from >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT);
        
        if (emc_2x_clk_src & 0x03) {
            if ((emc_2x_clk_src - TEGRA_EMC_SRC_PLLMB_UD) <= 1) {
                g_is_pllmb = false;
            }
        } else {
            g_is_pllmb = !g_is_pllmb;
        }
        
        /* Configure the PLL values */
        next_clk_src = set_pll(rate_to, 0x9600, clk_src_emc_to, g_is_pllmb);
    } else {
        uint32_t emc_2x_clk_src = (clk_src_emc_to >> EMC_CLK_EMC_2X_CLK_SRC_SHIFT);
        
        if ((emc_2x_clk_src != TEGRA_EMC_SRC_PLLMB) && emc_2x_clk_src) {
            if (((emc_2x_clk_src - TEGRA_EMC_SRC_PLLM_UD) <= TEGRA_EMC_SRC_PLLC) && g_is_pllmb) {
                next_clk_src = ((clk_src_emc_to & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLMB_UD << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
            }
        } else if (g_is_pllmb) {
            next_clk_src = ((clk_src_emc_to & 0x1FFFFFFF) | (TEGRA_EMC_SRC_PLLMB << EMC_CLK_EMC_2X_CLK_SRC_SHIFT));
        } else {
            next_clk_src = clk_src_emc_to;
        }
    }
    
    if (mode == OP_SWITCH) {
        print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Train mode is OP_SWITCH!\n");
        set_clock(current_timing_table, next_timing_table, false, next_clk_src);
        g_active_timing_table_idx = next_timing_table_idx;
        if (next_timing_table->periodic_training) {
            do_periodic_emc_compensation(next_timing_table);
        }
    } else if (mode == OP_TRAIN) {
        print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Train mode is OP_TRAIN!\n");
        train_set_clock(current_timing_table, next_timing_table, false, next_clk_src);
        g_active_timing_table_idx = next_timing_table_idx;        
        if (diff_freq) {
            g_is_pllmb = !g_is_pllmb;
        }
    } else if (mode == OP_TRAIN_SWITCH) {
        print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Train mode is OP_TRAIN_SWITCH!\n");
        train_set_clock(current_timing_table, next_timing_table, true, next_clk_src);
        g_active_timing_table_idx = next_timing_table_idx;
        if (next_timing_table->periodic_training) {
            do_periodic_emc_compensation(next_timing_table);
        }
    } else
        return 4;

    return 0;
}

static void train_dram_erista(void) {
    volatile tegra_car_t *car = car_get_regs();
    
    tegra_emc_timing_t *timing_tables;
    uint32_t dram_id = fuse_get_dram_id();
    
    /* Find the right timing table set. */
    if (dram_id == 0x01)
        timing_tables = (tegra_emc_timing_t *)nx_abca2_dram_1;
    else if ((dram_id == 0x00) || (dram_id == 0x02) || (dram_id == 0x03) || (dram_id == 0x04))
        timing_tables = (tegra_emc_timing_t *)nx_abca2_dram_0234;
    else
        fatal_error("[MTC]: Missing tables for DRAM id %d!\n", dram_id);

    /* Locate the right timing table. */
    int boot_index = 0;
    for (boot_index = 0; boot_index < MTC_TABLES_MAX_ENTRIES; boot_index++) {
        print(SCREEN_LOG_LEVEL_DEBUG, "%d (%d kHz): %s\n", boot_index, timing_tables[boot_index].rate, timing_tables[boot_index].dvfs_ver);
        if (car->clk_source_emc == timing_tables[boot_index].clk_src_emc)
            break;
    }

    if (boot_index >= MTC_TABLES_MAX_ENTRIES) {
        fatal_error("[MTC]: Failed to find timing table!\n");
    }
    
    /* Switch to 800Mhz. */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Switching from %dMhz to 800Mhz\n", timing_tables[boot_index].rate / 1000);
    train_one(0, 800000, timing_tables[boot_index].rate, timing_tables, MTC_TABLES_MAX_ENTRIES, OP_TRAIN_SWITCH);

    /* Switch to 1600Mhz. */
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Switching from %dMhz to 1600Mhz\n", timing_tables[g_active_timing_table_idx].rate / 1000);
    train_one(0, 1600000, timing_tables[g_active_timing_table_idx].rate, timing_tables, MTC_TABLES_MAX_ENTRIES, OP_TRAIN_SWITCH);
    
    /* Wait a while. */
    mdelay(100);
    
    /* Do periodic compensation. */
    do_periodic_emc_compensation((tegra_emc_timing_t*)&timing_tables[g_active_timing_table_idx]);
    
    print(SCREEN_LOG_LEVEL_DEBUG, "[MTC]: Done!\n");
}

void train_dram(void) {
    if (is_soc_mariko()) {
        /* TODO */
    } else {
        train_dram_erista();
    }
}