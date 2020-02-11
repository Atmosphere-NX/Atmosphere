/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "hvisor_cpu_sysreg_general.hpp"

namespace ams::hvisor::cpu {

    static inline u64 MaskIrq()
    {
        u64 daif = THERMOSPHERE_GET_SYSREG(daif);
        THERMOSPHERE_SET_SYSREG_IMM(daifset, BIT(1));
        return daif;
    }

    static inline u64 UnmaskIrq()
    {
        u64 daif = THERMOSPHERE_GET_SYSREG(daif);
        THERMOSPHERE_SET_SYSREG_IMM(daifclr, BIT(1));
        return daif;
    }

    static inline void RestoreInterruptFlags(u64 flags)
    {
        THERMOSPHERE_SET_SYSREG(daif, flags);
    }

    class InterruptMaskGuard final {
        NON_COPYABLE(InterruptMaskGuard);
        NON_MOVEABLE(InterruptMaskGuard);
        private:
            u64 m_flags;
        public:
            ALWAYS_INLINE InterruptMaskGuard() : m_flags(MaskIrq()) {}
            ALWAYS_INLINE ~InterruptMaskGuard()
            {
                RestoreInterruptFlags(m_flags);
            }
    };
}
