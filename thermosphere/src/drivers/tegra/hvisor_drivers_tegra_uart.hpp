/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "../../defines.hpp"

namespace ams::hvisor::drivers::tegra {

    class Uart final {
        private:
            struct Registers {
                union {
                    // UART_THR_DLAB_0
                    u32 thr;
                    u32 rbr;
                    u32 dll;
                };
                union {
                    // UART_IER_DLAB_0
                    u32 ier;
                    u32 dlh;
                };
                union {
                    // UART_IIR_FCR_0
                    u32 iir;
                    u32 fcr;
                };
                u32 lcr;
                u32 mcr;
                u32 lsr;
                u32 msr;
                u32 spr;
                u32 irda_csr;
                u32 rx_fifo_cfg;
                u32 mie;
                u32 vendor_status;
                u8 _0x30[0x0C];
                u32 asr;
            };
            static_assert(std::is_standard_layout_v<Registers>);
            static_assert(std::is_trivial_v<Registers>);

            // 36.3.12 UART_VENDOR_STATUS_0_0
            enum VendorStatusFlags : u32 {
                STATUS_TX_FIFO_COUNTER   = 0b111111 << 24, // reflects number of current entries in TX FIFO
                STATUS_RX_FIFO_COUNTER   = 0b111111 << 16, // reflects number of current entries in RX FIFO

                /*
                    This bit is set to 1 when write data is issued to the TX FIFO when
                    it is already full and gets cleared on register read (sticky bit until read):
                        0 = NO_OVERRUN
                        1 = OVERRUN
                 */
                STATUS_TX_OVERRUN        = BIT(3),

                /*
                    This bit is set to 1 when a read is issued to an empty FIFO and
                    gets cleared on register read (sticky bit until read):
                        0 = NO_UNDERRUN
                        1 = UNDERRUN
                 */
                STATUS_RX_UNDERRUN       = BIT(2),
                STATUS_RX_IDLE           = BIT(1),
                STATUS_TX_IDLE           = BIT(0),
            };

            // 36.3.6 UART_LSR_0
            enum LsrFlags : u32 {
                LSR_RX_FIFO_EMPTY = BIT(9), // Receiver FIFO empty status
                LSR_TX_FIFO_FULL  = BIT(8), // Transmitter FIFO full status
                LSR_FIFOE         = BIT(7), // Receive FIFO Error
                LSR_TMTY          = BIT(6), // Transmit Shift Register empty status
                LSR_THRE          = BIT(5), // Transmit Holding Register is Empty -- OK to write data
                LSR_BRK           = BIT(4), // BREAK condition detected on line
                LSR_FERR          = BIT(3), // Framing Error
                LSR_PERR          = BIT(2), // Parity Error
                LSR_OVRF          = BIT(1), // Receiver Overrun Error
                LSR_RDR           = BIT(0), // Receiver Data Ready
            };

            // 36.3.4 UART_LCR_0
            enum LcrFlags : u32 {
                /*
                    STOP:
                        0 = Transmit 1 stop bit
                        1 = Transmit 2 stop bits (receiver always checks for 1 stop bit)
                 */
                LCR_DLAB        = BIT(7), // Divisor Latch Access Bit (set to allow programming of the DLH, DLM Divisors)
                LCR_SET_B       = BIT(6), // Set BREAK condition -- Transmitter sends all zeroes to indicate BREAK
                LCR_SET_P       = BIT(5), // Set (force) parity to value in LCR[4]
                LCR_EVEN        = BIT(4), // Even parity format. There will always be an even number of 1s in the binary representation (PAR = 1)
                LCR_PAR         = BIT(3), // Parity enabled
                LCR_STOP        = BIT(2),
                
                LCR_WD_LENGTH_5 = 0 << 0, // word length 5
                LCR_WD_LENGTH_6 = 1 << 0, // word length 6
                LCR_WD_LENGTH_7 = 2 << 0, // word length 7
                LCR_WD_LENGTH_8 = 3 << 0, // word length 8
            };

            // 36.3.3 UART_IIR_FCR_0
            enum FcrFlags : u32{
                // RX_TRIG
                FCR_RX_TRIG_MASK                  = 3 << 6,
                FCR_RX_TRIG_FIFO_COUNT_GREATER_1  = 0 << 6,
                FCR_RX_TRIG_FIFO_COUNT_GREATER_4  = 1 << 6,
                FCR_RX_TRIG_FIFO_COUNT_GREATER_8  = 2 << 6,
                FCR_RX_TRIG_FIFO_COUNT_GREATER_16 = 3 << 6,

                // TX_TRIG
                FCR_TX_TRIG_MASK                  = 3 << 4,
                FCR_TX_TRIG_FIFO_COUNT_GREATER_16 = 0 << 4,
                FCR_TX_TRIG_FIFO_COUNT_GREATER_8  = 1 << 4,
                FCR_TX_TRIG_FIFO_COUNT_GREATER_4  = 2 << 4,
                FCR_TX_TRIG_FIFO_COUNT_GREATER_1  = 3 << 4,

                /*
                    DMA:
                        0 = DMA_MODE_0
                        1 = DMA_MODE_1
                 */
                FCR_DMA         = BIT(3),

                /*
                    RX/TX_CLR:
                        Clears the contents of the receive (resp. transmit) FIFO and resets
                        its counter logic to 0.
                        The receive (resp. transmit) shift register is not cleared or altered.
                        This bit returns to 0 after clearing the FIFOs.
                 */
                FCR_TX_CLR      = BIT(2), // See above
                FCR_RX_CLR      = BIT(1), // See above
                FCR_FCR_EN_FIFO = BIT(0), // Enable the transmit and receive FIFOs. This bit should be enabled
            };

            // 36.3.2 UART_IER_DLAB_0_0
            enum IerFlags : u32 {
                IER_IE_EORD         = BIT(5), // Interrupt enable for Interrupt Enable for End of Received Data
                IER_IE_RX_TIMEOUT   = BIT(4), // Interrupt enable for RX FIFO timeout
                IER_IE_MSI          = BIT(3), // Interrupt enable for Modem Status Interrupt
                IER_IE_RXS          = BIT(2), // Interrupt enable for Receiver Line Status Interrupt
                IER_IE_THR          = BIT(1), // Interrupt enable for Transmitter Holding Register Empty interrupt
                IER_IE_RHR          = BIT(0), // Interrupt enable for Received Data Interrupt
            };

            // 6.3.3 UART_IIR_FCR_0
            enum IirFlags : u32 {
                IIR_EN_FIFO_MASK    = 3 << 6,
                IIR_MODE_16450      = 0 << 6,
                IIR_MODE_16550      = 1 << 6,

                IIR_IS_PRI2         = BIT(3), // Encoded Interrupt ID Refer to IIR[3:0] table
                IIR_IS_PRI1         = BIT(2), // Encoded Interrupt ID Refer to IIR[3:0] table
                IIR_IS_PRI0         = BIT(1), // Encoded Interrupt ID Refer to IIR[3:0] table [36.3.3]
                IIR_IS_STA          = BIT(0), // Interrupt Pending if ZERO
            };

            // 36.3.9 UART_IRDA_CSR_0
            enum IrdaCsrFlags : u32{
                IRDA_CSR_SIR_A                     = BIT(7),

                IRDA_CSR_PWT_A_BAUD_PULSE_3_14     = 0 << 6,
                IRDA_CSR_PWT_A_BAUD_PULSE_4_14     = 1 << 6,

                IRDA_CSR_INVERT_RTS                = BIT(3),
                IRDA_CSR_INVERT_CTS                = BIT(2),
                IRDA_CSR_INVERT_TXD                = BIT(1),
                IRDA_CSR_INVERT_RXD                = BIT(0),
            };

        private:
            // TODO friend
            volatile Registers *m_regs = nullptr;

        private:
            void Initialize(u32 baudRate, u32 clkRate, bool invertTx) const;
            void WaitIdle(u32 status) const
            {
                if (status & STATUS_TX_IDLE) {
                    while (!(m_regs->lsr & LSR_TMTY));
                }

                if (status & STATUS_RX_IDLE) {
                    while (m_regs->lsr & LSR_RDR);
                }
            }

        public:
            void WriteData(const void *buffer, size_t size) const;
            void ReadData(void *buffer, size_t size) const;
            size_t ReadDataMax(void *buffer, size_t maxSize) const;
            size_t ReadDataUntil(char *buffer, size_t maxSize, char delimiter) const;

            void SetRxInterruptEnabled(bool enabled) const;
    };
}