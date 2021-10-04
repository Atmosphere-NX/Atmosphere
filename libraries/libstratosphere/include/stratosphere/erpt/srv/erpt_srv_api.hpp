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

namespace ams::erpt::srv {

    Result Initialize(u8 *mem, size_t mem_size);
    Result InitializeAndStartService();

    Result SetSerialNumberAndOsVersion(const char *sn, u32 sn_len, const char *os, u32 os_len, const char *os_priv, u32 os_priv_len);
    Result SetProductModel(const char *model, u32 model_len);
    Result SetRegionSetting(const char *region, u32 region_len);

    /* Atmosphere extension. */
    Result SetRedirectNewReportsToSdCard(bool redirect);

    void Wait();

}