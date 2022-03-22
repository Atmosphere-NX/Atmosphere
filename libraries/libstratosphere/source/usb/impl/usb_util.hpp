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
#include <stratosphere.hpp>

namespace ams::usb::impl {

    constexpr int GetEndpointIndex(u8 address) {
        int idx = address & UsbEndpointAddressMask_EndpointNumber;
        if ((address & UsbEndpointAddressMask_Dir) == UsbEndpointAddressMask_DirDevicetoHost) {
            idx += 0x10;
        }
        return idx;
    }

    template<typename T>
    class ScopedRefCount {
        NON_COPYABLE(ScopedRefCount);
        NON_MOVEABLE(ScopedRefCount);
        private:
            T &m_obj;
        public:
            ALWAYS_INLINE ScopedRefCount(T &o) : m_obj(o) {
                ++m_obj;
            }

            ALWAYS_INLINE ~ScopedRefCount() {
                --m_obj;
            }
    };

}
