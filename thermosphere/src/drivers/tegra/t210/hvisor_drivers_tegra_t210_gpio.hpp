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

#include "../../../defines.hpp"

namespace ams::hvisor::drivers::tegra::t210 {

    class Gpio final {
        private:
            static constexpr size_t numBanks = 8;
            static constexpr size_t portsPerBank = 4;
            struct Bank {
                u32 cnf[portsPerBank];
                u32 oe[portsPerBank];
                u32 out[portsPerBank];
                u32 in[portsPerBank];
                u32 int_sta[portsPerBank];
                u32 int_enb[portsPerBank];
                u32 int_lvl[portsPerBank];
                u32 int_clr[portsPerBank];
                u32 msk_cnf[portsPerBank];
                u32 msk_oe[portsPerBank];
                u32 msk_out[portsPerBank];
                u32 db_ctrl_p[portsPerBank];
                u32 msk_int_sta[portsPerBank];
                u32 msk_int_enb[portsPerBank];
                u32 msk_int_lvl[portsPerBank];
                u32 db_cnt_p[portsPerBank];
            };

            struct Registers {
                Bank bank[numBanks];
            };
            static_assert(std::is_standard_layout_v<Registers>);
            static_assert(std::is_trivial_v<Registers>);

        public:
            enum Port {
                PORT_A = 0,
                PORT_B = 1,
                PORT_C = 2,
                PORT_D = 3,
                PORT_E = 4,
                PORT_F = 5,
                PORT_G = 6,
                PORT_H = 7,
                PORT_I = 8,
                PORT_J = 9,
                PORT_K = 10,
                PORT_L = 11,
                PORT_M = 12,
                PORT_N = 13,
                PORT_O = 14,
                PORT_P = 15,
                PORT_Q = 16,
                PORT_R = 17,
                PORT_S = 18,
                PORT_T = 19,
                PORT_U = 20,
                PORT_V = 21,
                PORT_W = 22,
                PORT_X = 23,
                PORT_Y = 24,
                PORT_Z = 25,
                PORT_AA = 26,
                PORT_BB = 27,
                PORT_CC = 28,
                PORT_DD = 29,
                PORT_EE = 30,
                PORT_FF = 31,
            };

            enum class Mode {
                Sfio = 0,
                Gpio = 1,
            };

            enum class Direction {
                Tristate    = 0,   // Input
                Driven      = 1,     // Output

                Input       = Tristate,
                Output      = Driven, 
            };

            enum class Level {
                Low = 0,
                High = 1,
            };

        private:
            // For msk_* fields
            static constexpr u32 MakeMaskedWriteValue(u32 pos, bool value)
            {
                return BIT(8 + pos) | ((value ? 1 : 0) << pos);
            }

            static constexpr u32 MakeMaskedWriteValueContiguous(u32 pos, size_t n, bool value)
            {
                u32 msk = MASK2(pos + n - 1, pos);
                return (msk << 8) | (value ? msk : 0);
            }

        private:
            struct Pin {
                Port port;
                u8 pos;
            };
            static_assert(std::is_standard_layout_v<Pin>);
            static_assert(std::is_trivial_v<Pin>);
            static_assert(sizeof(Pin) <= 8);


        public:
            static constexpr Pin uart3Tx             = {PORT_D, 1};
            static constexpr Pin uart3Rx             = {PORT_D, 2};
            static constexpr Pin uart3Rts            = {PORT_D, 3};
            static constexpr Pin uart3Cts            = {PORT_D, 4};
            static constexpr Pin uart2Tx             = {PORT_G, 0};
            static constexpr Pin uart2Rx             = {PORT_G, 1};
            static constexpr Pin uart2Rts            = {PORT_G, 2};
            static constexpr Pin uart2Cts            = {PORT_G, 3};
            static constexpr Pin uart4Tx             = {PORT_I, 4};
            static constexpr Pin uart4Rx             = {PORT_I, 5};
            static constexpr Pin uart4Rts            = {PORT_I, 6};
            static constexpr Pin uart4Cts            = {PORT_I, 7};
            static constexpr Pin uart1Tx             = {PORT_U, 0};
            static constexpr Pin uart1Rx             = {PORT_U, 1};
            static constexpr Pin uart1Rts            = {PORT_U, 2};
            static constexpr Pin uart1Cts            = {PORT_U, 3};

            static constexpr Pin volUp               = {PORT_X, 6};
            static constexpr Pin volDown             = {PORT_X, 7};
            static constexpr Pin microSdCardDetect   = {PORT_Z, 1};
            static constexpr Pin microSdWriteProtect = {PORT_Z, 4};
            static constexpr Pin microSdSupplyEnable = {PORT_E, 4};
            static constexpr Pin lcdBlP5v            = {PORT_I, 0};
            static constexpr Pin lcdBlN5v            = {PORT_I, 1};
            static constexpr Pin lcdBlPwm            = {PORT_V, 0};
            static constexpr Pin lcdBlEn             = {PORT_V, 1};
            static constexpr Pin lcdBlRst            = {PORT_V, 2};

        private:
            volatile Registers *m_regs = nullptr;

        public:
            void SetMode(Pin pin, Mode mode) const
            {
                m_regs->bank[pin.port / portsPerBank].msk_cnf[pin.port % portsPerBank] = MakeMaskedWriteValue(pin.pos, mode == Mode::Gpio);
            }

            void SetModeContiguous(Pin pin, size_t n, Mode mode) const
            {
                m_regs->bank[pin.port / portsPerBank].msk_cnf[pin.port % portsPerBank] = MakeMaskedWriteValueContiguous(pin.pos, n, mode == Mode::Gpio);
            }

            // Only valid for GPIO (not SFIO)
            void SetDirection(Pin pin, Direction direction) const
            {
                m_regs->bank[pin.port / portsPerBank].msk_oe[pin.port % portsPerBank] = MakeMaskedWriteValue(pin.pos, direction == Direction::Output);
            }

            // Only valid for GPIO (not SFIO)
            void SetDirectionContiguous(Pin pin, size_t n, Direction direction) const
            {
                m_regs->bank[pin.port / portsPerBank].msk_oe[pin.port % portsPerBank] = MakeMaskedWriteValueContiguous(pin.pos, n, direction == Direction::Output);
            }

            // Only valid for GPIO (not SFIO)
            void Write(Pin pin, Level level) const
            {
                m_regs->bank[pin.port / portsPerBank].msk_out[pin.port % portsPerBank] = MakeMaskedWriteValue(pin.pos, level == Level::High);
            }

            // Only valid for GPIO (not SFIO)
            Level Read(Pin pin) const
            {
                return static_cast<Level>((m_regs->bank[pin.port / portsPerBank].in[pin.port % portsPerBank] >> pin.pos) & 1);
            }

            void ConfigureUartPins()
            {
                constexpr Pin uartPins[] = {uart1Tx, uart2Tx, uart3Tx, uart4Tx};

                // Set SFIO to all the 4 contiguous pins (tx, rx, rts, cts)
                for (Pin pin : uartPins) {
                    SetModeContiguous(pin, 4, Mode::Sfio);
                }
            }
    };

}
