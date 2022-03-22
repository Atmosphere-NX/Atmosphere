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

namespace ams::ldr {

    class AutoCloseMap {
        NON_COPYABLE(AutoCloseMap);
        NON_MOVEABLE(AutoCloseMap);
        private:
            Result m_result;
            uintptr_t m_map_address;
            os::NativeHandle m_handle;
            u64 m_address;
            u64 m_size;
        public:
            AutoCloseMap(uintptr_t map, os::NativeHandle handle, u64 addr, u64 size) : m_map_address(map), m_handle(handle), m_address(addr), m_size(size) {
                m_result = svc::MapProcessMemory(m_map_address, m_handle, m_address, m_size);
            }

            ~AutoCloseMap() {
                if (m_handle != os::InvalidNativeHandle && R_SUCCEEDED(m_result)) {
                    R_ABORT_UNLESS(svc::UnmapProcessMemory(m_map_address, m_handle, m_address, m_size));
                }
            }

            Result GetResult() const {
                return m_result;
            }

            bool IsSuccess() const {
                return R_SUCCEEDED(m_result);
            }

            void Cancel() {
                m_handle = os::InvalidNativeHandle;
            }
    };

}
