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

#ifdef ATMOSPHERE_ARCH_ARM64
    #include <mesosphere/arch/arm64/kern_k_thread_context.hpp>

    namespace ams::kern {
        using ams::kern::arch::arm64::KThreadContext;

        using ams::kern::arch::arm64::GetUserContext;
    }
#else
    #error "Unknown architecture for KThreadContext"
#endif
