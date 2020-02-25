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

constexpr InitialConfig InitialConfigsCalcio[] = {
    {0x50, GpioDirection_Output, GpioValue_Low},
    {0x51, GpioDirection_Output, GpioValue_Low},
    {0x52, GpioDirection_Output, GpioValue_Low},
    {0x53, GpioDirection_Output, GpioValue_Low},
    {0x02, GpioDirection_Output, GpioValue_Low},
    {0x0B, GpioDirection_Input,  GpioValue_Low},
    {0x14, GpioDirection_Input,  GpioValue_High},
    {0x18, GpioDirection_Input,  GpioValue_Low},
    {0x19, GpioDirection_Input,  GpioValue_High},
    {0x1A, GpioDirection_Input,  GpioValue_High},
    {0x1C, GpioDirection_Input,  GpioValue_High},
    {0x20, GpioDirection_Output, GpioValue_Low},
    {0x38, GpioDirection_Input,  GpioValue_High},
    {0x23, GpioDirection_Input,  GpioValue_High},
    {0x25, GpioDirection_Input,  GpioValue_Low},
    {0x26, GpioDirection_Input,  GpioValue_Low},
    {0x27, GpioDirection_Input,  GpioValue_Low},
    {0x28, GpioDirection_Input,  GpioValue_High},
    {0x4F, GpioDirection_Input,  GpioValue_High},
    {0x48, GpioDirection_Output, GpioValue_Low},
    {0x4C, GpioDirection_Input,  GpioValue_High},
    {0x4A, GpioDirection_Output, GpioValue_Low},
    {0x2D, GpioDirection_Output, GpioValue_Low},
    {0x2E, GpioDirection_Output, GpioValue_Low},
    {0x37, GpioDirection_Input,  GpioValue_Low},
    {0x2F, GpioDirection_Output, GpioValue_Low},
    {0x03, GpioDirection_Output, GpioValue_Low},
    {0x30, GpioDirection_Input,  GpioValue_Low},
    {0x31, GpioDirection_Output, GpioValue_Low},
    {0x49, GpioDirection_Output, GpioValue_Low},
    {0x4E, GpioDirection_Input,  GpioValue_Low},
};

constexpr u32 NumInitialConfigsCalcio = util::size(InitialConfigsCalcio);
