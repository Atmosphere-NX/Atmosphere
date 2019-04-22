/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

struct ModuleId {
    u8 build_id[0x20];
};
static_assert(sizeof(ModuleId) == sizeof(LoaderModuleInfo::build_id), "ModuleId definition!");

struct Sha256Hash {
    u8 hash[0x20];

    bool operator==(const Sha256Hash &o) const {
        return std::memcmp(this, &o, sizeof(*this)) == 0;
    }
    bool operator!=(const Sha256Hash &o) const {
        return std::memcmp(this, &o, sizeof(*this)) != 0;
    }
    bool operator<(const Sha256Hash &o) const {
        return std::memcmp(this, &o, sizeof(*this)) < 0;
    }
    bool operator>(const Sha256Hash &o) const {
        return std::memcmp(this, &o, sizeof(*this)) > 0;
    }
};
static_assert(sizeof(Sha256Hash) == sizeof(Sha256Hash::hash), "Sha256Hash definition!");
