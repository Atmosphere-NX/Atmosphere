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

#define ATMOSPHERE_IS_TROPOSPHERE
#define ATMOSPHERE_OS_HORIZON
#define ATMOSPHERE_BOARD_NINTENDO_NX
#define ATMOSPHERE_ARCH_ARM64
#define ATMOSPHERE_ARCH_ARM_V8A

#include <algorithm>
#include <cstring>
#include <bit>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#include <switch.h>
#include <haze/results.hpp>
#include <haze/assert.hpp>

#include <vapours/literals.hpp>
#include <vapours/svc/svc_common.hpp>
#include <vapours/svc/svc_types_common.hpp>

namespace haze {

    using namespace ::ams::literals;
    using namespace ::ams;

    using Result = ::ams::Result;

}
