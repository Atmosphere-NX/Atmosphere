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
#include <vapours.hpp>
#include "kern_panic.hpp"

namespace ams::kern {

    /* TODO: Actually select between architecture-specific interrupt code. */


    /* Enable or disable interrupts for the lifetime of an object. */
    class KScopedInterruptDisable {
        NON_COPYABLE(KScopedInterruptDisable);
        NON_MOVEABLE(KScopedInterruptDisable);
        public:
            KScopedInterruptDisable();
            ~KScopedInterruptDisable();
    };

    class KScopedInterruptEnable {
        NON_COPYABLE(KScopedInterruptEnable);
        NON_MOVEABLE(KScopedInterruptEnable);
        public:
            KScopedInterruptEnable();
            ~KScopedInterruptEnable();
    };

}
