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

namespace sts::reg {

    inline void Write(volatile u32 *reg, u32 val) {
        *reg = val;
    }

    inline void Write(uintptr_t reg, u32 val) {
        Write(reinterpret_cast<volatile u32 *>(reg), val);
    }

    inline u32 Read(volatile u32 *reg) {
        return *reg;
    }

    inline u32 Read(uintptr_t reg) {
        return Read(reinterpret_cast<volatile u32 *>(reg));
    }

    inline void SetBits(volatile u32 *reg, u32 mask) {
        *reg |= mask;
    }

    inline void SetBits(uintptr_t reg, u32 mask) {
        SetBits(reinterpret_cast<volatile u32 *>(reg), mask);
    }

    inline void ClearBits(volatile u32 *reg, u32 mask) {
        *reg &= ~mask;
    }

    inline void ClearBits(uintptr_t reg, u32 mask) {
        ClearBits(reinterpret_cast<volatile u32 *>(reg), mask);
    }

    inline void MaskBits(volatile u32 *reg, u32 mask) {
        *reg &= mask;
    }

    inline void MaskBits(uintptr_t reg, u32 mask) {
        MaskBits(reinterpret_cast<volatile u32 *>(reg), mask);
    }

    inline void ReadWrite(volatile u32 *reg, u32 val, u32 mask) {
        *reg = (*reg & (~mask)) | (val & mask);
    }

    inline void ReadWrite(uintptr_t reg, u32 val, u32 mask) {
        ReadWrite(reinterpret_cast<volatile u32 *>(reg), val, mask);
    }

}