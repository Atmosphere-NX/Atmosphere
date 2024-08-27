/*
 * Copyright (C) Switch-OC-Suite
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

#include "../oc_common.hpp"

namespace ams::ldr::oc::ptm {

typedef struct {
    u32 conf_id;
    u32 cpu_freq_1; // min-max pair?
    u32 cpu_freq_2;
    u32 gpu_freq_1;
    u32 gpu_freq_2;
    u32 emc_freq_1;
    u32 emc_freq_2;
    u32 padding;
} perf_conf_entry;

constexpr u32 entryCnt       = 16;
constexpr u32 cpuPtmDefault  = 1020'000'000;
constexpr u32 cpuPtmDevOC    = 1224'000'000;
constexpr u32 cpuPtmBoost    = 1785'000'000;

constexpr u32 memPtmLimit    = 1600'000'000;
constexpr u32 memPtmAlt      = 1331'200'000;
constexpr u32 memPtmClamp    = 1065'600'000;

void Patch(uintptr_t mapped_nso, size_t nso_size);

}