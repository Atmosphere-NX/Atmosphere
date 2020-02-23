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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_scoped_lock.hpp>

#if defined(ATMOSPHERE_ARCH_ARM64)

    #include <mesosphere/arch/arm64/kern_k_spin_lock.hpp>
    namespace ams::kern {
        using ams::kern::arch::arm64::KAlignedSpinLock;
        using ams::kern::arch::arm64::KNotAlignedSpinLock;
        using ams::kern::arch::arm64::KSpinLock;
    }

#else

    #error "Unknown architecture for KInterruptManager"

#endif


namespace ams::kern {

    using KScopedSpinLock           = KScopedLock<KSpinLock>;
    using KScopedAlignedSpinLock    = KScopedLock<KAlignedSpinLock>;
    using KScopedNotAlignedSpinLock = KScopedLock<KNotAlignedSpinLock>;

}
