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
#include <stratosphere.hpp>

namespace ams::i2c::driver::board::nintendo_nx::impl {

    struct I2cRegisters {
        volatile u32 cnfg;
        volatile u32 cmd_addr0;
        volatile u32 cmd_addr1;
        volatile u32 cmd_data1;
        volatile u32 cmd_data2;
        volatile u32 _14;
        volatile u32 _18;
        volatile u32 status;
        volatile u32 sl_cnfg;
        volatile u32 sl_rcvd;
        volatile u32 sl_status;
        volatile u32 sl_addr1;
        volatile u32 sl_addr2;
        volatile u32 tlow_sext;
        volatile u32 _38;
        volatile u32 sl_delay_count;
        volatile u32 sl_int_mask;
        volatile u32 sl_int_source;
        volatile u32 sl_int_set;
        volatile u32 _4c;
        volatile u32 tx_packet_fifo;
        volatile u32 rx_fifo;
        volatile u32 packet_transfer_status;
        volatile u32 fifo_control;
        volatile u32 fifo_status;
        volatile u32 interrupt_mask_register;
        volatile u32 interrupt_status_register;
        volatile u32 clk_divisor_register;
        volatile u32 interrupt_source_register;
        volatile u32 interrupt_set_register;
        volatile u32 slv_tx_packet_fifo;
        volatile u32 slv_rx_fifo;
        volatile u32 slv_packet_status;
        volatile u32 bus_clear_config;
        volatile u32 bus_clear_status;
        volatile u32 config_load;
        volatile u32 _90;
        volatile u32 interface_timing_0;
        volatile u32 interface_timing_1;
        volatile u32 hs_interface_timing_0;
        volatile u32 hs_interface_timing_1;
    };
    static_assert(sizeof(I2cRegisters) == 0xA4);

}
