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

namespace ams::updater {

    /* Types. */
    enum class BootImageUpdateType {
        Erista,
        Mariko,
    };

    enum class BootModeType {
        Normal,
        Safe,
    };

    struct VerificationState {
        bool needs_verify_normal;
        bool needs_verify_safe;
    };

    /* Convenience size definitions. */
    constexpr size_t BctSize = 0x4000;
    constexpr size_t EksSize = 0x4000;
    constexpr size_t EksEntrySize = 0x200;
    constexpr size_t EksBlobSize = 0xB0;

}