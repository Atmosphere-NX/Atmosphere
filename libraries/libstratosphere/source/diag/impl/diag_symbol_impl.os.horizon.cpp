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
#include <stratosphere.hpp>
#include "diag_symbol_impl.hpp"

namespace ams::diag::impl {

    uintptr_t GetSymbolNameImpl(char *dst, size_t dst_size, uintptr_t address) {
        AMS_UNUSED(dst, dst_size, address);
        AMS_ABORT("TODO");
    }

    size_t GetSymbolSizeImpl(uintptr_t address) {
        AMS_UNUSED(address);
        AMS_ABORT("TODO");
    }

}
