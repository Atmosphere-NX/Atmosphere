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
#include <vapours.hpp>

namespace ams::i2c {

    enum Port {
        Port_1 = 0,
        /* TODO: Support other ports? */
        Port_5 = 4,

        Port_Count = 5,
    };

    void SetRegisterAddress(Port port, uintptr_t address);

    void Initialize(Port port);

    bool Query(void *dst, size_t dst_size, Port port, int address, int r);
    bool Send(Port port, int address, int r, const void *src, size_t src_size);

    inline u8 QueryByte(Port port, int address, int r) {
        u8 byte;
        Query(std::addressof(byte), sizeof(byte), port, address, r);
        return byte;
    }

    inline void SendByte(Port port, int address, int r, u8 byte) {
        Send(port, address, r, std::addressof(byte), sizeof(byte));
    }

}