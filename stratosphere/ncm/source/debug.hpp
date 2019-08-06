/*
 * Copyright (c) 2019 Adubbz
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
#include <switch.h>
#include <stratosphere.hpp>
#include <inttypes.h>

namespace sts::debug {
    #define STR_(X) #X
    #define STR(X) STR_(X)

    #define D_LOG(format, ...) \
        debug::DebugLog("%s:" STR(__LINE__) " " format, __FUNCTION__, ##__VA_ARGS__);
    #define R_DEBUG_START \
        Result rc = [&]() -> Result {
    #define R_DEBUG_END \
        }(); \
        D_LOG(" -> 0x%08" PRIX32 "\n", rc); \
        return rc;

    Result Initialize();
    void DebugLog(const char* format, ...);
    void LogBytes(const void* buf, size_t len);

}