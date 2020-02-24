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

struct WakeControlConfig {
    u32 reg_offset;
    u32 mask_val;
    bool flag_val;
};

constexpr WakeControlConfig WakeControlConfigs[] = {
    {APBDEV_PMC_CNTRL, 0x0800, true},
    {APBDEV_PMC_CNTRL, 0x0400, false},
    {APBDEV_PMC_CNTRL, 0x0200, true},
    {APBDEV_PMC_CNTRL, 0x0100, false},
    {APBDEV_PMC_CNTRL, 0x0040, false},
    {APBDEV_PMC_CNTRL, 0x0020, false},
    {APBDEV_PMC_CNTRL2, 0x4000, true},
    {APBDEV_PMC_CNTRL2, 0x0200, false},
    {APBDEV_PMC_CNTRL2, 0x0001, true},
};

constexpr size_t NumWakeControlConfigs = util::size(WakeControlConfigs);
