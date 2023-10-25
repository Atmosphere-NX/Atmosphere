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

namespace ams::fs::impl {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteDeviceOperator {
        private:
            ::FsDeviceOperator m_operator;
        public:
            RemoteDeviceOperator(::FsDeviceOperator &o) : m_operator(o) { /* ... */ }

            virtual ~RemoteDeviceOperator() {
                fsDeviceOperatorClose(std::addressof(m_operator));
            }
        public:
            Result IsSdCardInserted(ams::sf::Out<bool> out) {
                R_RETURN(fsDeviceOperatorIsSdCardInserted(std::addressof(m_operator), out.GetPointer()));
            }

            Result GetMmcCid(ams::sf::OutBuffer out, s64 size) {
                R_RETURN(fsDeviceOperatorGetMmcCid(std::addressof(m_operator), out.GetPointer(), out.GetSize(), size));
            }

            Result GetMmcSpeedMode(ams::sf::Out<s64> out) {
                R_RETURN(fsDeviceOperatorGetMmcSpeedMode(std::addressof(m_operator), out.GetPointer()));
            }

            Result GetMmcPatrolCount(ams::sf::Out<u32> out) {
                R_RETURN(fsDeviceOperatorGetMmcPatrolCount(std::addressof(m_operator), out.GetPointer()));
            }

            Result GetMmcExtendedCsd(ams::sf::OutBuffer out, s64 size) {
                R_RETURN(fsDeviceOperatorGetMmcExtendedCsd(std::addressof(m_operator), out.GetPointer(), out.GetSize(), size));
            }

            Result IsGameCardInserted(ams::sf::Out<bool> out) {
                R_RETURN(fsDeviceOperatorIsGameCardInserted(std::addressof(m_operator), out.GetPointer()));
            }

            Result GetGameCardHandle(ams::sf::Out<u32> out) {
                static_assert(sizeof(::FsGameCardHandle) == sizeof(u32));
                R_RETURN(fsDeviceOperatorGetGameCardHandle(std::addressof(m_operator), reinterpret_cast<::FsGameCardHandle *>(out.GetPointer())));
            }
    };
    static_assert(fssrv::sf::IsIDeviceOperator<RemoteDeviceOperator>);
    #endif

}
