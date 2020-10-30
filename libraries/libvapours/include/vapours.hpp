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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/literals.hpp>

#include <vapours/allocator.hpp>
#include <vapours/device_code.hpp>
#include <vapours/timespan.hpp>
#include <vapours/span.hpp>

#include <vapours/util.hpp>
#include <vapours/results.hpp>
#include <vapours/reg.hpp>

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
#include <vapours/tegra.hpp>
#endif

#include <vapours/crypto.hpp>
#include <vapours/svc.hpp>

#include <vapours/ams/ams_fatal_error_context.hpp>

#include <vapours/dd.hpp>
#include <vapours/sdmmc.hpp>