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
#include "impl/diag_symbol_impl.hpp"

namespace ams::diag {

    uintptr_t GetSymbolName(char *dst, size_t dst_size, uintptr_t address) {
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(dst_size > 0);
        AMS_ASSERT(address > 0);

        return ::ams::diag::impl::GetSymbolNameImpl(dst, dst_size, address);
    }

    size_t GetSymbolSize(uintptr_t address) {
        AMS_ASSERT(address > 0);

        return ::ams::diag::impl::GetSymbolSizeImpl(address);
    }

}
