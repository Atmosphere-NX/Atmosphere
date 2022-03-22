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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_system_control_base.hpp>

#ifdef ATMOSPHERE_BOARD_NINTENDO_NX
    #include <mesosphere/board/nintendo/nx/kern_k_system_control.hpp>

    namespace ams::kern {
        using ams::kern::board::nintendo::nx::KSystemControl;
    }

#elif defined(ATMOSPHERE_BOARD_QEMU_VIRT)
    #include <mesosphere/board/qemu/virt/kern_k_system_control.hpp>

    namespace ams::kern {
        using ams::kern::board::qemu::virt::KSystemControl;
    }

#else
    #error "Unknown board for KSystemControl"
#endif

namespace ams::kern {

    ALWAYS_INLINE u32 KSystemControlBase::ReadRegisterPrivileged(ams::svc::PhysicalAddress address) {
        u32 v;
        KSystemControl::ReadWriteRegisterPrivileged(std::addressof(v), address, 0x00000000u, 0);
        return v;
    }

    ALWAYS_INLINE void KSystemControlBase::WriteRegisterPrivileged(ams::svc::PhysicalAddress address, u32 value) {
        u32 v;
        KSystemControl::ReadWriteRegisterPrivileged(std::addressof(v), address, 0xFFFFFFFFu, value);
    }

}
