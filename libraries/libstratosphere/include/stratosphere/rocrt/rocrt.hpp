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

namespace ams::rocrt {

    constexpr inline const u32 ModuleHeaderVersion = util::FourCC<'M','O','D','0'>::Code;

    struct ModuleHeader {
        u32 signature;
        u32 dynamic_offset;
        u32 bss_start_offset;
        u32 bss_end_offset;
        u32 exception_info_start_offset;
        u32 exception_info_end_offset;
        u32 module_offset;
    };

    struct ModuleHeaderLocation {
        u32 pad;
        u32 header_offset;
    };

    constexpr inline u32 CheckModuleHeaderSignature(const ModuleHeader *header) {
        if (header->signature == ModuleHeaderVersion) {
            return header->signature;
        } else {
            return 0;
        }
    }

    constexpr inline ModuleHeader *GetModuleHeader(const ModuleHeaderLocation *loc) {
        return reinterpret_cast<ModuleHeader *>(reinterpret_cast<uintptr_t>(loc) + loc->header_offset);
    }

    constexpr inline uintptr_t GetDynamicOffset(const ModuleHeader *header, const ModuleHeaderLocation *loc) {
        return reinterpret_cast<uintptr_t>(loc) + loc->header_offset + header->dynamic_offset;
    }


    constexpr inline uintptr_t GetBssStartAddress(const ModuleHeader *header, const ModuleHeaderLocation *loc) {
        return reinterpret_cast<uintptr_t>(loc) + loc->header_offset + header->bss_start_offset;
    }

    constexpr inline uintptr_t GetBssEndAddress(const ModuleHeader *header, const ModuleHeaderLocation *loc) {
        return reinterpret_cast<uintptr_t>(loc) + loc->header_offset + header->bss_end_offset;
    }

    constexpr inline uintptr_t GetModuleOffset(const ModuleHeader *header, const ModuleHeaderLocation *loc) {
        return reinterpret_cast<uintptr_t>(loc) + loc->header_offset + header->module_offset;
    }

}
