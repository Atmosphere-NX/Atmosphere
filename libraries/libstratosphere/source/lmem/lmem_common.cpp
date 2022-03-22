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
#include "impl/lmem_impl_common_heap.hpp"

namespace ams::lmem {

    u32 GetDebugFillValue(FillType fill_type) {
        return impl::GetDebugFillValue(fill_type);
    }

    void SetDebugFillValue(FillType fill_type, u32 value) {
        impl::SetDebugFillValue(fill_type, value);
    }

    size_t GetTotalSize(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetHeapTotalSize(handle);
    }

    void *GetStartAddress(HeapHandle handle) {
        impl::ScopedHeapLock lk(handle);
        return impl::GetHeapStartAddress(handle);
    }

    bool ContainsAddress(HeapHandle handle, const void *address) {
        impl::ScopedHeapLock lk(handle);
        return impl::ContainsAddress(handle, address);
    }

}
