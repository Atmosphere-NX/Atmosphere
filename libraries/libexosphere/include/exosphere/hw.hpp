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

#if   defined(ATMOSPHERE_ARCH_ARM64)
    #include <exosphere/hw/hw_arm64.hpp>
    namespace ams::hw { using namespace ams::hw::arch::arm64; }
#elif defined(ATMOSPHERE_ARCH_ARM)
    #include <exosphere/hw/hw_arm.hpp>
    namespace ams::hw { using namespace ams::hw::arch::arm; }
#else
    #error "Unknown architecture for hw!"
#endif