/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>

struct MemoryRegionExtents {
    u64 base;
    u64 size;
};

struct CheatProcessMetadata {
    u64 process_id;
    u64 title_id;
    MemoryRegionExtents main_nso_extents;
    MemoryRegionExtents heap_extents;
    MemoryRegionExtents alias_extents;
    MemoryRegionExtents address_space_extents;
    u8 main_nso_build_id[0x20];
};

struct CheatDefinition {
    char readable_name[0x40];
    uint32_t num_opcodes;
    uint32_t opcodes[0x100];
};

struct CheatEntry {
    bool enabled;
    uint32_t cheat_id;
    CheatDefinition definition;
};

struct FrozenAddressValue {
    u64 value;
    u8 width;
};

struct FrozenAddressEntry {
    u64 address;
    FrozenAddressValue value;
};