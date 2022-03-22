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

namespace ams::lm::impl {

    enum LogDataChunkKey {
        LogDataChunkKey_LogSessionBegin    =  0,
        LogDataChunkKey_LogSessionEnd      =  1,
        LogDataChunkKey_TextLog            =  2,
        LogDataChunkKey_LineNumber         =  3,
        LogDataChunkKey_FileName           =  4,
        LogDataChunkKey_FunctionName       =  5,
        LogDataChunkKey_ModuleName         =  6,
        LogDataChunkKey_ThreadName         =  7,
        LogDataChunkKey_LogPacketDropCount =  8,
        LogDataChunkKey_UserSystemClock    =  9,
        LogDataChunkKey_ProcessName        = 10,
    };

}
