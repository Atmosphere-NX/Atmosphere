/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_types.hpp"

static inline uintptr_t GetIoMapping(u64 io_addr, u64 io_size) {
    u64 vaddr;
    u64 aligned_addr = (io_addr & ~0xFFFul);
    u64 aligned_size = io_size + (io_addr - aligned_addr);
    if (R_FAILED(svcQueryIoMapping(&vaddr, aligned_addr, aligned_size))) {
        std::abort();
    }
    return static_cast<uintptr_t>(vaddr + (io_addr - aligned_addr));
}

struct I2cRegisters {
    volatile u32 I2C_I2C_CNFG_0;
    volatile u32 I2C_I2C_CMD_ADDR0_0;
    volatile u32 I2C_I2C_CMD_ADDR1_0;
    volatile u32 I2C_I2C_CMD_DATA1_0;
    volatile u32 I2C_I2C_CMD_DATA2_0;
    volatile u32 _0x14;
    volatile u32 _0x18;
    volatile u32 I2C_I2C_STATUS_0;
    volatile u32 I2C_I2C_SL_CNFG_0;
    volatile u32 I2C_I2C_SL_RCVD_0;
    volatile u32 I2C_I2C_SL_STATUS_0;
    volatile u32 I2C_I2C_SL_ADDR1_0;
    volatile u32 I2C_I2C_SL_ADDR2_0;
    volatile u32 I2C_I2C_TLOW_SEXT_0;
    volatile u32 _0x38;
    volatile u32 I2C_I2C_SL_DELAY_COUNT_0;
    volatile u32 I2C_I2C_SL_INT_MASK_0;
    volatile u32 I2C_I2C_SL_INT_SOURCE_0;
    volatile u32 I2C_I2C_SL_INT_SET_0;
    volatile u32 _0x4C;
    volatile u32 I2C_I2C_TX_PACKET_FIFO_0;
    volatile u32 I2C_I2C_RX_FIFO_0;
    volatile u32 I2C_PACKET_TRANSFER_STATUS_0;
    volatile u32 I2C_FIFO_CONTROL_0;
    volatile u32 I2C_FIFO_STATUS_0;
    volatile u32 I2C_INTERRUPT_MASK_REGISTER_0;
    volatile u32 I2C_INTERRUPT_STATUS_REGISTER_0;
    volatile u32 I2C_I2C_CLK_DIVISOR_REGISTER_0;
    volatile u32 I2C_I2C_INTERRUPT_SOURCE_REGISTER_0;
    volatile u32 I2C_I2C_INTERRUPT_SET_REGISTER_0;
    volatile u32 I2C_I2C_SLV_TX_PACKET_FIFO_0;
    volatile u32 I2C_I2C_SLV_RX_FIFO_0;
    volatile u32 I2C_I2C_SLV_PACKET_STATUS_0;
    volatile u32 I2C_I2C_BUS_CLEAR_CONFIG_0;
    volatile u32 I2C_I2C_BUS_CLEAR_STATUS_0;
    volatile u32 I2C_I2C_CONFIG_LOAD_0;
    volatile u32 _0x90;
    volatile u32 I2C_I2C_INTERFACE_TIMING_0_0;
    volatile u32 I2C_I2C_INTERFACE_TIMING_1_0;
    volatile u32 I2C_I2C_HS_INTERFACE_TIMING_0_0;
    volatile u32 I2C_I2C_HS_INTERFACE_TIMING_1_0;
};

static inline I2cRegisters *GetI2cRegisters(I2cBus bus) {
    static constexpr uintptr_t s_offsets[] = {
        0x0000, 0x0400, 0x0500, 0x0700, 0x1000, 0x1100
    };
    if (bus >= sizeof(s_offsets) / sizeof(s_offsets[0])) {
        std::abort();
    }

    const uintptr_t registers = GetIoMapping(0x7000c000ul, 0x2000) + s_offsets[static_cast<size_t>(bus)];
    return reinterpret_cast<I2cRegisters *>(registers);
}

struct ClkRstRegisters {
    public:
        volatile u32 *clk_src_reg;
        volatile u32 *clk_en_reg;
        volatile u32 *rst_reg;
        u32 mask;
    public:
        void SetBus(I2cBus bus) {
            static constexpr uintptr_t s_clk_src_offsets[] = {
                0x124, 0x198, 0x1b8, 0x3c4, 0x128, 0x65c
            };
            static constexpr uintptr_t s_clk_en_offsets[] = {
                0x010, 0x014, 0x018, 0x360, 0x014, 0x280
            };
            static constexpr uintptr_t s_rst_offsets[] = {
                0x004, 0x008, 0x00c, 0x358, 0x008, 0x28c
            };
            static constexpr size_t s_bit_shifts[] = {
                12, 22, 3, 7, 15, 6
            };
            if (bus >= sizeof(s_clk_src_offsets) / sizeof(s_clk_src_offsets[0])) {
                std::abort();
            }

            const uintptr_t registers = GetIoMapping(0x60006000ul, 0x1000);
            const size_t idx = static_cast<size_t>(bus);
            this->clk_src_reg = reinterpret_cast<volatile u32 *>(registers + s_clk_src_offsets[idx]);
            this->clk_en_reg  = reinterpret_cast<volatile u32 *>(registers + s_clk_en_offsets[idx]);
            this->rst_reg     = reinterpret_cast<volatile u32 *>(registers + s_rst_offsets[idx]);
            this->mask        = (1u << s_bit_shifts[idx]);
        }
};

static inline void WriteRegister(volatile u32 *reg, u32 val) {
    *reg = val;
}

static inline u32 ReadRegister(volatile u32 *reg) {
    u32 val = *reg;
    return val;
}

static inline void SetRegisterBits(volatile u32 *reg, u32 mask) {
    *reg |= mask;
}

static inline void ClearRegisterBits(volatile u32 *reg, u32 mask) {
    *reg &= mask;
}

static inline void ReadWriteRegisterBits(volatile u32 *reg, u32 val, u32 mask) {
    *reg = (*reg & (~mask)) | (val & mask);
}
