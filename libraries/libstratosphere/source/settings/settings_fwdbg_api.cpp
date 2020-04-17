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
#include <stratosphere.hpp>

namespace ams::settings::fwdbg {

    bool IsDebugModeEnabled() {
        bool value = false;
        R_ABORT_UNLESS(::setsysGetDebugModeFlag(std::addressof(value)));
        return value;
    }

    size_t WEAK_SYMBOL GetSettingsItemValueSize(const char *name, const char *key) {
        u64 size = 0;
        R_ABORT_UNLESS(setsysGetSettingsItemValueSize(name, key, &size));
        return size;
    }

    size_t WEAK_SYMBOL GetSettingsItemValue(void *dst, size_t dst_size, const char *name, const char *key) {
        u64 size = 0;
        R_ABORT_UNLESS(setsysGetSettingsItemValue(name, key, dst, dst_size, &size));
        return size;
    }

}
