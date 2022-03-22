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
#include <vapours.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_typed_address.hpp>

#if 1

    #include <mesosphere/arch/arm/kern_generic_interrupt_controller.hpp>
    namespace ams::kern::arch::arm64 {

        using ams::kern::arch::arm::GicDistributor;
        using ams::kern::arch::arm::GicCpuInterface;
        using ams::kern::arch::arm::KInterruptController;

    }

#else

    #error "Unknown board for KInterruptController"

#endif
