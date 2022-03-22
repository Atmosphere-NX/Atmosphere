/*
 * Copyright (c) Atmosphère-NX
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
#include <mesosphere.hpp>

namespace ams::kern::init::loader {

    struct SavedRegisterState {
        u64 x[(30 - 19) + 1];
        u64 sp;
        u64 xzr;
    };
    static_assert(sizeof(SavedRegisterState) == 0x70);

    int SaveRegistersToTpidrEl1(void *tpidr_el1);
    void VerifyAndClearTpidrEl1(void *tpidr_el1);

}
