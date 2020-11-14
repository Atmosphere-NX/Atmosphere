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
#include <exosphere.hpp>

namespace ams::i2c {

    namespace {

        constexpr inline size_t MaxTransferSize = sizeof(u32);

        constinit std::array<uintptr_t, Port_Count> g_register_addresses = [] {
            std::array<uintptr_t, Port_Count> arr = {};

            arr[Port_1] = secmon::MemoryRegionPhysicalDeviceI2c1.GetAddress();
            arr[Port_5] = secmon::MemoryRegionPhysicalDeviceI2c5.GetAddress();

            return arr;
        }();

        void LoadConfig(uintptr_t address) {
            /* Configure for TIMEOUT and MSTR config load. */
            /* NOTE: Nintendo writes value 1 to reserved bit 5 here. This bit is documented as having no meaning. */
            /* We will reproduce the write just in case it is undocumented. */
            reg::Write(address + I2C_CONFIG_LOAD, I2C_REG_BITS_VALUE(CONFIG_LOAD_RESERVED_BIT_5, 1),
                                                  I2C_REG_BITS_ENUM (CONFIG_LOAD_TIMEOUT_CONFIG_LOAD, ENABLE),
                                                  I2C_REG_BITS_ENUM (CONFIG_LOAD_SLV_CONFIG_LOAD,     DISABLE),
                                                  I2C_REG_BITS_ENUM (CONFIG_LOAD_MSTR_CONFIG_LOAD,    ENABLE));

            /* Wait up to 20 microseconds for the master config to be loaded. */
            for (int i = 0; i < 20; ++i) {
                if (reg::HasValue(address + I2C_CONFIG_LOAD, I2C_REG_BITS_ENUM(CONFIG_LOAD_MSTR_CONFIG_LOAD, DISABLE))) {
                    return;
                }
                util::WaitMicroSeconds(1);
            }
        }

        void ClearBus(uintptr_t address) {
            /* Configure the bus clear register. */
            reg::Write(address + I2C_BUS_CLEAR_CONFIG, I2C_REG_BITS_VALUE(BUS_CLEAR_CONFIG_BC_SCLK_THRESHOLD,         9),
                                                       I2C_REG_BITS_ENUM (BUS_CLEAR_CONFIG_BC_STOP_COND,        NO_STOP),
                                                       I2C_REG_BITS_ENUM (BUS_CLEAR_CONFIG_BC_TERMINATE,      IMMEDIATE),
                                                       I2C_REG_BITS_ENUM (BUS_CLEAR_CONFIG_BC_ENABLE,            ENABLE));

            /* Load the config. */
            LoadConfig(address);

            /* Wait up to 250us (in 25 us increments) until the bus clear is done. */
            for (int i = 0; i < 10; ++i) {
                if (reg::HasValue(address + I2C_INTERRUPT_STATUS_REGISTER, I2C_REG_BITS_ENUM(INTERRUPT_STATUS_REGISTER_BUS_CLEAR_DONE, SET))) {
                    break;
                }

                util::WaitMicroSeconds(25);
            }

            /* Read the bus clear status. */
            reg::Read(address + I2C_BUS_CLEAR_STATUS);
        }

        void InitializePort(uintptr_t address) {
            /* Calculate the divisor. */
            constexpr int Divisor = util::DivideUp(19200, 8 * 400);

            /* Set the divisor. */
            reg::Write(address + I2C_CLK_DIVISOR_REGISTER, I2C_REG_BITS_VALUE(CLK_DIVISOR_REGISTER_STD_FAST_MODE, Divisor - 1),
                                                           I2C_REG_BITS_VALUE(CLK_DIVISOR_REGISTER_HSMODE,        1));

            /* Clear the bus. */
            ClearBus(address);

            /* Clear the status. */
            reg::Write(address + I2C_INTERRUPT_STATUS_REGISTER, reg::Read(address + I2C_INTERRUPT_STATUS_REGISTER));
        }

        bool Write(uintptr_t base_address, Port port, int address, const void *src, size_t src_size, bool unused) {
            AMS_UNUSED(port, unused);

            /* Ensure we don't write too much. */
            u32 data = 0;
            if (src_size > MaxTransferSize) {
                return false;
            }

            /* Copy the data to a transfer word. */
            std::memcpy(std::addressof(data), src, src_size);


            /* Configure the to write the 7-bit address. */
            reg::Write(base_address + I2C_I2C_CMD_ADDR0, I2C_REG_BITS_VALUE(I2C_CMD_ADDR0_7BIT_ADDR, address),
                                                         I2C_REG_BITS_ENUM (I2C_CMD_ADDR0_7BIT_RW,         WRITE));

            /* Configure to write the data. */
            reg::Write(base_address + I2C_I2C_CMD_DATA1, data);

            /* Configure to write the correct amount of data. */
            reg::Write(base_address + I2C_I2C_CNFG, I2C_REG_BITS_ENUM (I2C_CNFG_DEBOUNCE_CNT,    DEBOUNCE_4T),
                                                    I2C_REG_BITS_ENUM (I2C_CNFG_NEW_MASTER_FSM,       ENABLE),
                                                    I2C_REG_BITS_ENUM (I2C_CNFG_CMD1,                  WRITE),
                                                    I2C_REG_BITS_VALUE(I2C_CNFG_LENGTH,         src_size - 1));

            /* Load the configuration. */
            LoadConfig(base_address);

            /* Start the command. */
            reg::ReadWrite(base_address + I2C_I2C_CNFG, I2C_REG_BITS_ENUM(I2C_CNFG_SEND, GO));

            /* Wait for the command to be done. */
            while (!reg::HasValue(base_address + I2C_I2C_STATUS, I2C_REG_BITS_ENUM(I2C_STATUS_BUSY, NOT_BUSY))) { /* ... */ }

            /* Check if the transfer was successful. */
            return reg::HasValue(base_address + I2C_I2C_STATUS, I2C_REG_BITS_ENUM(I2C_STATUS_CMD1_STAT, SL1_XFER_SUCCESSFUL));
        }

        bool Read(uintptr_t base_address, Port port, void *dst, size_t dst_size, int address, bool unused) {
            AMS_UNUSED(port, unused);

            /* Ensure we don't read too much. */
            if (dst_size > MaxTransferSize) {
                return false;
            }

            /* Configure the to read the 7-bit address. */
            reg::Write(base_address + I2C_I2C_CMD_ADDR0, I2C_REG_BITS_VALUE(I2C_CMD_ADDR0_7BIT_ADDR, address),
                                                         I2C_REG_BITS_ENUM (I2C_CMD_ADDR0_7BIT_RW,          READ));

            /* Configure to read the correct amount of data. */
            reg::Write(base_address + I2C_I2C_CNFG, I2C_REG_BITS_ENUM (I2C_CNFG_DEBOUNCE_CNT,    DEBOUNCE_4T),
                                                    I2C_REG_BITS_ENUM (I2C_CNFG_NEW_MASTER_FSM,       ENABLE),
                                                    I2C_REG_BITS_ENUM (I2C_CNFG_CMD1,                   READ),
                                                    I2C_REG_BITS_VALUE(I2C_CNFG_LENGTH,         dst_size - 1));

            /* Load the configuration. */
            LoadConfig(base_address);

            /* Start the command. */
            reg::ReadWrite(base_address + I2C_I2C_CNFG, I2C_REG_BITS_ENUM(I2C_CNFG_SEND, GO));

            /* Wait for the command to be done. */
            while (!reg::HasValue(base_address + I2C_I2C_STATUS, I2C_REG_BITS_ENUM(I2C_STATUS_BUSY, NOT_BUSY))) { /* ... */ }

            /* Check that the transfer was successful. */
            if (!reg::HasValue(base_address + I2C_I2C_STATUS, I2C_REG_BITS_ENUM(I2C_STATUS_CMD1_STAT, SL1_XFER_SUCCESSFUL))) {
                return false;
            }

            /* Read and copy out the data. */
            u32 data = reg::Read(base_address + I2C_I2C_CMD_DATA1);
            std::memcpy(dst, std::addressof(data), dst_size);
            return true;
        }

    }

    void SetRegisterAddress(Port port, uintptr_t address) {
        g_register_addresses[port] = address;
    }

    void Initialize(Port port) {
        InitializePort(g_register_addresses[port]);
    }

    bool Query(void *dst, size_t dst_size, Port port, int address, int r) {
        const uintptr_t base_address = g_register_addresses[port];

        /* Select the register we want to read. */
        bool success = Write(base_address, port, address, std::addressof(r), 1, false);
        if (success) {
            /* If we successfully selected, read data from the register. */
            success = Read(base_address, port, dst, dst_size, address, true);
        }

        return success;
    }

    bool Send(Port port, int address, int r, const void *src, size_t src_size) {
        const uintptr_t base_address = g_register_addresses[port];

        /* Create a transfer buffer, make sure we can use it. */
        u8 buffer[MaxTransferSize];
        if (src_size > sizeof(buffer) - 1) {
            return false;
        }

        /* Copy data into the buffer. */
        buffer[0] = static_cast<u8>(r);
        std::memcpy(buffer + 1, src, src_size);

        return Write(base_address, port, address, buffer, src_size + 1, false);
    }

}
