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

namespace ams::settings::factory {

    struct EccP256DeviceCertificate {
        u8 data[0x180];
    };
    static_assert(sizeof(EccP256DeviceCertificate) == 0x180);
    static_assert(util::is_pod<EccP256DeviceCertificate>::value);

    struct EccB233DeviceCertificate {
        u8 data[0x180];
    };
    static_assert(sizeof(EccB233DeviceCertificate) == 0x180);
    static_assert(util::is_pod<EccB233DeviceCertificate>::value);

    struct Rsa2048DeviceCertificate {
        u8 data[0x240];
    };
    static_assert(sizeof(Rsa2048DeviceCertificate) == 0x240);
    static_assert(util::is_pod<Rsa2048DeviceCertificate>::value);

}
