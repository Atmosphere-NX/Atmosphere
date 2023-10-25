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

            Result GetSdCardSpeedMode(ams::sf::Out<s64> out) {
                R_RETURN(fsDeviceOperatorGetSdCardSpeedMode(std::addressof(m_operator), out.GetPointer()));
            }

            Result GetSdCardCid(ams::sf::OutBuffer out, s64 size) {
                R_RETURN(fsDeviceOperatorGetSdCardCid(std::addressof(m_operator), out.GetPointer(), out.GetSize(), size));
            }

            Result GetSdCardUserAreaSize(ams::sf::Out<s64> out) {
                R_RETURN(fsDeviceOperatorGetSdCardUserAreaSize(std::addressof(m_operator), out.GetPointer()));
            }

            Result GetSdCardProtectedAreaSize(ams::sf::Out<s64> out) {
                R_RETURN(fsDeviceOperatorGetSdCardProtectedAreaSize(std::addressof(m_operator), out.GetPointer()));
            }

            Result GetAndClearSdCardErrorInfo(ams::sf::Out<fs::StorageErrorInfo> out_sei, ams::sf::Out<s64> out_size, ams::sf::OutBuffer out_buf, s64 size) {
                static_assert(sizeof(::FsStorageErrorInfo) == sizeof(fs::StorageErrorInfo));
                R_RETURN(fsDeviceOperatorGetAndClearSdCardErrorInfo(std::addressof(m_operator), reinterpret_cast<::FsStorageErrorInfo *>(out_sei.GetPointer()), out_size.GetPointer(), out_buf.GetPointer(), out_buf.GetSize(), size));
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

            Result GetAndClearMmcErrorInfo(ams::sf::Out<fs::StorageErrorInfo> out_sei, ams::sf::Out<s64> out_size, ams::sf::OutBuffer out_buf, s64 size) {
                static_assert(sizeof(::FsStorageErrorInfo) == sizeof(fs::StorageErrorInfo));
                R_RETURN(fsDeviceOperatorGetAndClearMmcErrorInfo(std::addressof(m_operator), reinterpret_cast<::FsStorageErrorInfo *>(out_sei.GetPointer()), out_size.GetPointer(), out_buf.GetPointer(), out_buf.GetSize(), size));
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

            Result GetGameCardIdSet(ams::sf::OutBuffer out, s64 size) {
                R_RETURN(fsDeviceOperatorGetGameCardIdSet(std::addressof(m_operator), out.GetPointer(), out.GetSize(), size));
            }

            Result GetGameCardErrorReportInfo(ams::sf::Out<fs::GameCardErrorReportInfo> out) {
                static_assert(sizeof(::FsGameCardErrorReportInfo) == sizeof(fs::GameCardErrorReportInfo));
                R_RETURN(fsDeviceOperatorGetGameCardErrorReportInfo(std::addressof(m_operator), reinterpret_cast<::FsGameCardErrorReportInfo *>(out.GetPointer())));
            }

            Result GetGameCardDeviceId(ams::sf::OutBuffer out, s64 size) {
                R_RETURN(fsDeviceOperatorGetGameCardDeviceId(std::addressof(m_operator), out.GetPointer(), out.GetSize(), size));
            }
    };
    static_assert(fssrv::sf::IsIDeviceOperator<RemoteDeviceOperator>);
    #endif

}
