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

namespace ams::nxboot {

    #define MC_BASE   (0x70019000)
    #define EMC_BASE  (0x7001B000)
    #define EMC0_BASE (0x7001E000)
    #define EMC1_BASE (0x7001F000)

    #define FOREACH_PER_CHANNEL_BURST_REG(HANDLER) \
        HANDLER(EMC0, EMC_MRW10, emc0_mrw10)       \
        HANDLER(EMC1, EMC_MRW10, emc1_mrw10)       \
        HANDLER(EMC0, EMC_MRW11, emc0_mrw11)       \
        HANDLER(EMC1, EMC_MRW11, emc1_mrw11)       \
        HANDLER(EMC0, EMC_MRW12, emc0_mrw12)       \
        HANDLER(EMC1, EMC_MRW12, emc1_mrw12)       \
        HANDLER(EMC0, EMC_MRW13, emc0_mrw13)       \
        HANDLER(EMC1, EMC_MRW13, emc1_mrw13)       \

    #define FOREACH_TRIM_REG(HANDLER)                                                                      \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0, emc_pmacro_ib_ddll_long_dqs_rank0_0)             \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1, emc_pmacro_ib_ddll_long_dqs_rank0_1)             \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2, emc_pmacro_ib_ddll_long_dqs_rank0_2)             \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3, emc_pmacro_ib_ddll_long_dqs_rank0_3)             \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0, emc_pmacro_ib_ddll_long_dqs_rank1_0)             \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1, emc_pmacro_ib_ddll_long_dqs_rank1_1)             \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2, emc_pmacro_ib_ddll_long_dqs_rank1_2)             \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3, emc_pmacro_ib_ddll_long_dqs_rank1_3)             \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0, emc_pmacro_ib_ddll_short_dq_rank0_byte0_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1, emc_pmacro_ib_ddll_short_dq_rank0_byte0_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2, emc_pmacro_ib_ddll_short_dq_rank0_byte0_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0, emc_pmacro_ib_ddll_short_dq_rank0_byte1_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1, emc_pmacro_ib_ddll_short_dq_rank0_byte1_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2, emc_pmacro_ib_ddll_short_dq_rank0_byte1_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0, emc_pmacro_ib_ddll_short_dq_rank0_byte2_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1, emc_pmacro_ib_ddll_short_dq_rank0_byte2_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2, emc_pmacro_ib_ddll_short_dq_rank0_byte2_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0, emc_pmacro_ib_ddll_short_dq_rank0_byte3_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1, emc_pmacro_ib_ddll_short_dq_rank0_byte3_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2, emc_pmacro_ib_ddll_short_dq_rank0_byte3_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0, emc_pmacro_ib_ddll_short_dq_rank0_byte4_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1, emc_pmacro_ib_ddll_short_dq_rank0_byte4_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2, emc_pmacro_ib_ddll_short_dq_rank0_byte4_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0, emc_pmacro_ib_ddll_short_dq_rank0_byte5_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1, emc_pmacro_ib_ddll_short_dq_rank0_byte5_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2, emc_pmacro_ib_ddll_short_dq_rank0_byte5_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0, emc_pmacro_ib_ddll_short_dq_rank0_byte6_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1, emc_pmacro_ib_ddll_short_dq_rank0_byte6_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2, emc_pmacro_ib_ddll_short_dq_rank0_byte6_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0, emc_pmacro_ib_ddll_short_dq_rank0_byte7_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1, emc_pmacro_ib_ddll_short_dq_rank0_byte7_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2, emc_pmacro_ib_ddll_short_dq_rank0_byte7_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0, emc_pmacro_ib_ddll_short_dq_rank1_byte0_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1, emc_pmacro_ib_ddll_short_dq_rank1_byte0_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2, emc_pmacro_ib_ddll_short_dq_rank1_byte0_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0, emc_pmacro_ib_ddll_short_dq_rank1_byte1_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1, emc_pmacro_ib_ddll_short_dq_rank1_byte1_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2, emc_pmacro_ib_ddll_short_dq_rank1_byte1_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0, emc_pmacro_ib_ddll_short_dq_rank1_byte2_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1, emc_pmacro_ib_ddll_short_dq_rank1_byte2_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2, emc_pmacro_ib_ddll_short_dq_rank1_byte2_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0, emc_pmacro_ib_ddll_short_dq_rank1_byte3_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1, emc_pmacro_ib_ddll_short_dq_rank1_byte3_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2, emc_pmacro_ib_ddll_short_dq_rank1_byte3_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0, emc_pmacro_ib_ddll_short_dq_rank1_byte4_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1, emc_pmacro_ib_ddll_short_dq_rank1_byte4_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2, emc_pmacro_ib_ddll_short_dq_rank1_byte4_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0, emc_pmacro_ib_ddll_short_dq_rank1_byte5_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1, emc_pmacro_ib_ddll_short_dq_rank1_byte5_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2, emc_pmacro_ib_ddll_short_dq_rank1_byte5_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0, emc_pmacro_ib_ddll_short_dq_rank1_byte6_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1, emc_pmacro_ib_ddll_short_dq_rank1_byte6_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2, emc_pmacro_ib_ddll_short_dq_rank1_byte6_2) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0, emc_pmacro_ib_ddll_short_dq_rank1_byte7_0) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1, emc_pmacro_ib_ddll_short_dq_rank1_byte7_1) \
        HANDLER(EMC, EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2, emc_pmacro_ib_ddll_short_dq_rank1_byte7_2) \
        HANDLER(EMC, EMC_PMACRO_IB_VREF_DQS_0, emc_pmacro_ib_vref_dqs_0)                                   \
        HANDLER(EMC, EMC_PMACRO_IB_VREF_DQS_1, emc_pmacro_ib_vref_dqs_1)                                   \
        HANDLER(EMC, EMC_PMACRO_IB_VREF_DQ_0, emc_pmacro_ib_vref_dq_0)                                     \
        HANDLER(EMC, EMC_PMACRO_IB_VREF_DQ_1, emc_pmacro_ib_vref_dq_1)                                     \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0, emc_pmacro_ob_ddll_long_dq_rank0_0)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1, emc_pmacro_ob_ddll_long_dq_rank0_1)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2, emc_pmacro_ob_ddll_long_dq_rank0_2)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3, emc_pmacro_ob_ddll_long_dq_rank0_3)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4, emc_pmacro_ob_ddll_long_dq_rank0_4)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5, emc_pmacro_ob_ddll_long_dq_rank0_5)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0, emc_pmacro_ob_ddll_long_dq_rank1_0)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1, emc_pmacro_ob_ddll_long_dq_rank1_1)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2, emc_pmacro_ob_ddll_long_dq_rank1_2)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3, emc_pmacro_ob_ddll_long_dq_rank1_3)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0, emc_pmacro_ob_ddll_short_dq_rank0_byte0_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1, emc_pmacro_ob_ddll_short_dq_rank0_byte0_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2, emc_pmacro_ob_ddll_short_dq_rank0_byte0_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0, emc_pmacro_ob_ddll_short_dq_rank0_byte1_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1, emc_pmacro_ob_ddll_short_dq_rank0_byte1_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2, emc_pmacro_ob_ddll_short_dq_rank0_byte1_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0, emc_pmacro_ob_ddll_short_dq_rank0_byte2_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1, emc_pmacro_ob_ddll_short_dq_rank0_byte2_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2, emc_pmacro_ob_ddll_short_dq_rank0_byte2_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0, emc_pmacro_ob_ddll_short_dq_rank0_byte3_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1, emc_pmacro_ob_ddll_short_dq_rank0_byte3_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2, emc_pmacro_ob_ddll_short_dq_rank0_byte3_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0, emc_pmacro_ob_ddll_short_dq_rank0_byte4_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1, emc_pmacro_ob_ddll_short_dq_rank0_byte4_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2, emc_pmacro_ob_ddll_short_dq_rank0_byte4_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0, emc_pmacro_ob_ddll_short_dq_rank0_byte5_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1, emc_pmacro_ob_ddll_short_dq_rank0_byte5_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2, emc_pmacro_ob_ddll_short_dq_rank0_byte5_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0, emc_pmacro_ob_ddll_short_dq_rank0_byte6_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1, emc_pmacro_ob_ddll_short_dq_rank0_byte6_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2, emc_pmacro_ob_ddll_short_dq_rank0_byte6_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0, emc_pmacro_ob_ddll_short_dq_rank0_byte7_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1, emc_pmacro_ob_ddll_short_dq_rank0_byte7_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2, emc_pmacro_ob_ddll_short_dq_rank0_byte7_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0, emc_pmacro_ob_ddll_short_dq_rank0_cmd0_0)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1, emc_pmacro_ob_ddll_short_dq_rank0_cmd0_1)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2, emc_pmacro_ob_ddll_short_dq_rank0_cmd0_2)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0, emc_pmacro_ob_ddll_short_dq_rank0_cmd1_0)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1, emc_pmacro_ob_ddll_short_dq_rank0_cmd1_1)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2, emc_pmacro_ob_ddll_short_dq_rank0_cmd1_2)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0, emc_pmacro_ob_ddll_short_dq_rank0_cmd2_0)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1, emc_pmacro_ob_ddll_short_dq_rank0_cmd2_1)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2, emc_pmacro_ob_ddll_short_dq_rank0_cmd2_2)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0, emc_pmacro_ob_ddll_short_dq_rank0_cmd3_0)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1, emc_pmacro_ob_ddll_short_dq_rank0_cmd3_1)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2, emc_pmacro_ob_ddll_short_dq_rank0_cmd3_2)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0, emc_pmacro_ob_ddll_short_dq_rank1_byte0_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1, emc_pmacro_ob_ddll_short_dq_rank1_byte0_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2, emc_pmacro_ob_ddll_short_dq_rank1_byte0_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0, emc_pmacro_ob_ddll_short_dq_rank1_byte1_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1, emc_pmacro_ob_ddll_short_dq_rank1_byte1_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2, emc_pmacro_ob_ddll_short_dq_rank1_byte1_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0, emc_pmacro_ob_ddll_short_dq_rank1_byte2_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1, emc_pmacro_ob_ddll_short_dq_rank1_byte2_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2, emc_pmacro_ob_ddll_short_dq_rank1_byte2_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0, emc_pmacro_ob_ddll_short_dq_rank1_byte3_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1, emc_pmacro_ob_ddll_short_dq_rank1_byte3_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2, emc_pmacro_ob_ddll_short_dq_rank1_byte3_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0, emc_pmacro_ob_ddll_short_dq_rank1_byte4_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1, emc_pmacro_ob_ddll_short_dq_rank1_byte4_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2, emc_pmacro_ob_ddll_short_dq_rank1_byte4_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0, emc_pmacro_ob_ddll_short_dq_rank1_byte5_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1, emc_pmacro_ob_ddll_short_dq_rank1_byte5_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2, emc_pmacro_ob_ddll_short_dq_rank1_byte5_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0, emc_pmacro_ob_ddll_short_dq_rank1_byte6_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1, emc_pmacro_ob_ddll_short_dq_rank1_byte6_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2, emc_pmacro_ob_ddll_short_dq_rank1_byte6_2) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0, emc_pmacro_ob_ddll_short_dq_rank1_byte7_0) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1, emc_pmacro_ob_ddll_short_dq_rank1_byte7_1) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2, emc_pmacro_ob_ddll_short_dq_rank1_byte7_2) \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK0_0, emc_pmacro_quse_ddll_rank0_0)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK0_1, emc_pmacro_quse_ddll_rank0_1)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK0_2, emc_pmacro_quse_ddll_rank0_2)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK0_3, emc_pmacro_quse_ddll_rank0_3)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK1_0, emc_pmacro_quse_ddll_rank1_0)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK1_1, emc_pmacro_quse_ddll_rank1_1)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK1_2, emc_pmacro_quse_ddll_rank1_2)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK1_3, emc_pmacro_quse_ddll_rank1_3)

    #define FOREACH_PER_CHANNEL_TRIM_REG(HANDLER)             \
        HANDLER(EMC0, EMC_CMD_BRLSHFT_0, emc0_cmd_brlshft_0)   \
        HANDLER(EMC1, EMC_CMD_BRLSHFT_1, emc1_cmd_brlshft_1)   \
        HANDLER(EMC0, EMC_DATA_BRLSHFT_0, emc0_data_brlshft_0) \
        HANDLER(EMC1, EMC_DATA_BRLSHFT_0, emc1_data_brlshft_0) \
        HANDLER(EMC0, EMC_DATA_BRLSHFT_1, emc0_data_brlshft_1) \
        HANDLER(EMC1, EMC_DATA_BRLSHFT_1, emc1_data_brlshft_1) \
        HANDLER(EMC0, EMC_QUSE_BRLSHFT_0, emc0_quse_brlshft_0) \
        HANDLER(EMC1, EMC_QUSE_BRLSHFT_1, emc1_quse_brlshft_1) \
        HANDLER(EMC0, EMC_QUSE_BRLSHFT_2, emc0_quse_brlshft_2) \
        HANDLER(EMC1, EMC_QUSE_BRLSHFT_3, emc1_quse_brlshft_3)

    #define FOREACH_PER_CHANNEL_VREF_REG(HANDLER)                                              \
        HANDLER(EMC0, EMC_TRAINING_OPT_DQS_IB_VREF_RANK0, emc0_training_opt_dqs_ib_vref_rank0) \
        HANDLER(EMC1, EMC_TRAINING_OPT_DQS_IB_VREF_RANK0, emc1_training_opt_dqs_ib_vref_rank0) \
        HANDLER(EMC0, EMC_TRAINING_OPT_DQS_IB_VREF_RANK1, emc0_training_opt_dqs_ib_vref_rank1) \
        HANDLER(EMC1, EMC_TRAINING_OPT_DQS_IB_VREF_RANK1, emc1_training_opt_dqs_ib_vref_rank1)

    #define FOREACH_PER_CHANNEL_TRAINING_MOD_REG(HANDLER)                                \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_IB_BYTE0, emc0_training_rw_offset_ib_byte0) \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_IB_BYTE0, emc1_training_rw_offset_ib_byte0) \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_IB_BYTE1, emc0_training_rw_offset_ib_byte1) \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_IB_BYTE1, emc1_training_rw_offset_ib_byte1) \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_IB_BYTE2, emc0_training_rw_offset_ib_byte2) \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_IB_BYTE2, emc1_training_rw_offset_ib_byte2) \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_IB_BYTE3, emc0_training_rw_offset_ib_byte3) \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_IB_BYTE3, emc1_training_rw_offset_ib_byte3) \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_IB_MISC, emc0_training_rw_offset_ib_misc)   \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_IB_MISC, emc1_training_rw_offset_ib_misc)   \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_OB_BYTE0, emc0_training_rw_offset_ob_byte0) \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_OB_BYTE0, emc1_training_rw_offset_ob_byte0) \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_OB_BYTE1, emc0_training_rw_offset_ob_byte1) \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_OB_BYTE1, emc1_training_rw_offset_ob_byte1) \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_OB_BYTE2, emc0_training_rw_offset_ob_byte2) \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_OB_BYTE2, emc1_training_rw_offset_ob_byte2) \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_OB_BYTE3, emc0_training_rw_offset_ob_byte3) \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_OB_BYTE3, emc1_training_rw_offset_ob_byte3) \
        HANDLER(EMC0, EMC_TRAINING_RW_OFFSET_OB_MISC, emc0_training_rw_offset_ob_misc)   \
        HANDLER(EMC1, EMC_TRAINING_RW_OFFSET_OB_MISC, emc1_training_rw_offset_ob_misc)

    #define FOREACH_BURST_MC_REG(HANDLER)                                               \
        HANDLER(MC, MC_EMEM_ARB_CFG, mc_emem_arb_cfg)                                   \
        HANDLER(MC, MC_EMEM_ARB_OUTSTANDING_REQ, mc_emem_arb_outstanding_req)           \
        HANDLER(MC, MC_EMEM_ARB_REFPB_HP_CTRL, mc_emem_arb_refpb_hp_ctrl)               \
        HANDLER(MC, MC_EMEM_ARB_REFPB_BANK_CTRL, mc_emem_arb_refpb_bank_ctrl)           \
        HANDLER(MC, MC_EMEM_ARB_TIMING_RCD, mc_emem_arb_timing_rcd)                     \
        HANDLER(MC, MC_EMEM_ARB_TIMING_RP, mc_emem_arb_timing_rp)                       \
        HANDLER(MC, MC_EMEM_ARB_TIMING_RC, mc_emem_arb_timing_rc)                       \
        HANDLER(MC, MC_EMEM_ARB_TIMING_RAS, mc_emem_arb_timing_ras)                     \
        HANDLER(MC, MC_EMEM_ARB_TIMING_FAW, mc_emem_arb_timing_faw)                     \
        HANDLER(MC, MC_EMEM_ARB_TIMING_RRD, mc_emem_arb_timing_rrd)                     \
        HANDLER(MC, MC_EMEM_ARB_TIMING_RAP2PRE, mc_emem_arb_timing_rap2pre)             \
        HANDLER(MC, MC_EMEM_ARB_TIMING_WAP2PRE, mc_emem_arb_timing_wap2pre)             \
        HANDLER(MC, MC_EMEM_ARB_TIMING_R2R, mc_emem_arb_timing_r2r)                     \
        HANDLER(MC, MC_EMEM_ARB_TIMING_W2W, mc_emem_arb_timing_w2w)                     \
        HANDLER(MC, MC_EMEM_ARB_TIMING_R2W, mc_emem_arb_timing_r2w)                     \
        HANDLER(MC, MC_EMEM_ARB_TIMING_CCDMW, mc_emem_arb_timing_ccdmw)                 \
        HANDLER(MC, MC_EMEM_ARB_TIMING_W2R, mc_emem_arb_timing_w2r)                     \
        HANDLER(MC, MC_EMEM_ARB_TIMING_RFCPB, mc_emem_arb_timing_rfcpb)                 \
        HANDLER(MC, MC_EMEM_ARB_DA_TURNS, mc_emem_arb_da_turns)                         \
        HANDLER(MC, MC_EMEM_ARB_DA_COVERS, mc_emem_arb_da_covers)                       \
        HANDLER(MC, MC_EMEM_ARB_MISC0, mc_emem_arb_misc0)                               \
        HANDLER(MC, MC_EMEM_ARB_MISC1, mc_emem_arb_misc1)                               \
        HANDLER(MC, MC_EMEM_ARB_MISC2, mc_emem_arb_misc2)                               \
        HANDLER(MC, MC_EMEM_ARB_RING1_THROTTLE, mc_emem_arb_ring1_throttle)             \
        HANDLER(MC, MC_EMEM_ARB_DHYST_CTRL, mc_emem_arb_dhyst_ctrl)                     \
        HANDLER(MC, MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_0, mc_emem_arb_dhyst_timeout_util_0) \
        HANDLER(MC, MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_1, mc_emem_arb_dhyst_timeout_util_1) \
        HANDLER(MC, MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_2, mc_emem_arb_dhyst_timeout_util_2) \
        HANDLER(MC, MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_3, mc_emem_arb_dhyst_timeout_util_3) \
        HANDLER(MC, MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_4, mc_emem_arb_dhyst_timeout_util_4) \
        HANDLER(MC, MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_5, mc_emem_arb_dhyst_timeout_util_5) \
        HANDLER(MC, MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_6, mc_emem_arb_dhyst_timeout_util_6) \
        HANDLER(MC, MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_7, mc_emem_arb_dhyst_timeout_util_7)

    #define FOREACH_LA_SCALE_REG(HANDLER)                                           \
        HANDLER(MC, MC_MLL_MPCORER_PTSA_RATE, mc_mll_mpcorer_ptsa_rate)             \
        HANDLER(MC, MC_FTOP_PTSA_RATE, mc_ftop_ptsa_rate)                           \
        HANDLER(MC, MC_PTSA_GRANT_DECREMENT, mc_ptsa_grant_decrement)               \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_XUSB_0, mc_latency_allowance_xusb_0)       \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_XUSB_1, mc_latency_allowance_xusb_1)       \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_TSEC_0, mc_latency_allowance_tsec_0)       \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_SDMMCA_0, mc_latency_allowance_sdmmca_0)   \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_SDMMCAA_0, mc_latency_allowance_sdmmcaa_0) \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_SDMMC_0, mc_latency_allowance_sdmmc_0)     \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_SDMMCAB_0, mc_latency_allowance_sdmmcab_0) \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_PPCS_0, mc_latency_allowance_ppcs_0)       \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_PPCS_1, mc_latency_allowance_ppcs_1)       \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_MPCORE_0, mc_latency_allowance_mpcore_0)   \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_HC_0, mc_latency_allowance_hc_0)           \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_HC_1, mc_latency_allowance_hc_1)           \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_AVPC_0, mc_latency_allowance_avpc_0)       \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_GPU_0, mc_latency_allowance_gpu_0)         \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_GPU2_0, mc_latency_allowance_gpu2_0)       \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_NVENC_0, mc_latency_allowance_nvenc_0)     \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_NVDEC_0, mc_latency_allowance_nvdec_0)     \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_VIC_0, mc_latency_allowance_vic_0)         \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_VI2_0, mc_latency_allowance_vi2_0)         \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_ISP2_0, mc_latency_allowance_isp2_0)       \
        HANDLER(MC, MC_LATENCY_ALLOWANCE_ISP2_1, mc_latency_allowance_isp2_1)

}