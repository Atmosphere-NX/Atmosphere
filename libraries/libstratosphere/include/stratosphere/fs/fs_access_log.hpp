/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours.hpp>

namespace ams::fs {

    enum AccessLogMode : u32 {
        AccessLogMode_None   = 0,
        AccessLogMode_Log    = 1,
        AccessLogMode_SdCard = 2,
    };

    Result GetGlobalAccessLogMode(u32 *out);
    Result SetGlobalAccessLogMode(u32 mode);

    void SetLocalAccessLog(bool enabled);
    void SetLocalSystemAccessLogForDebug(bool enabled);

}
