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
#include <mesosphere.hpp>

namespace ams::kern {

    WEAK_SYMBOL KScopedInterruptDisable::KScopedInterruptDisable() {
        /* TODO: Disable interrupts. */
    }

    WEAK_SYMBOL KScopedInterruptDisable::~KScopedInterruptDisable() {
        /* TODO: un-disable interrupts. */
    }

    WEAK_SYMBOL KScopedInterruptEnable::KScopedInterruptEnable() {
        /* TODO: Enable interrupts. */
    }

    WEAK_SYMBOL KScopedInterruptEnable::~KScopedInterruptEnable() {
        /* TODO: un-enable interrupts. */
    }

}
