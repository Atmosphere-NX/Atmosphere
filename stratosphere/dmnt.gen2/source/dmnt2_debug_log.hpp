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

namespace ams::dmnt {

    void InitializeDebugLog();

    void DebugLog(const char *prefix, const char *fmt, ...) __attribute((format(printf, 2, 3)));

    #define AMS_DMNT2_DEBUG_LOG(fmt, ...) ::ams::dmnt::DebugLog("[dmnt2] ", fmt, ## __VA_ARGS__)

    #define AMS_DMNT2_GDB_LOG_INFO(fmt, ...)  ::ams::dmnt::DebugLog("[gdb-i] ", fmt, ## __VA_ARGS__)
    #define AMS_DMNT2_GDB_LOG_WARN(fmt, ...)  ::ams::dmnt::DebugLog("[gdb-w] ", fmt, ## __VA_ARGS__)
    #define AMS_DMNT2_GDB_LOG_ERROR(fmt, ...) ::ams::dmnt::DebugLog("[gdb-e] ", fmt, ## __VA_ARGS__)
    #define AMS_DMNT2_GDB_LOG_DEBUG(fmt, ...) ::ams::dmnt::DebugLog("[gdb-d] ", fmt, ## __VA_ARGS__)

}