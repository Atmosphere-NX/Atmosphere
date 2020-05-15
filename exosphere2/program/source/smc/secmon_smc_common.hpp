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
#include <exosphere.hpp>

namespace ams::secmon::smc {

    enum class SmcResult : u32 {
        Success               = 0,
        NotImplemented        = 1,
        InvalidArgument       = 2,
        Busy                  = 3,
        NoAsyncOperation      = 4,
        InvalidAsyncOperation = 5,
        NotPermitted          = 6,
        NotInitialized        = 7,

        PsciSuccess           = 0,
        PsciNotSupported      = static_cast<u32>(-1),
        PsciInvalidParameters = static_cast<u32>(-2),
        PsciDenied            = static_cast<u32>(-3),
        PsciAlreadyOn         = static_cast<u32>(-4),
    };

    #define SMC_R_SUCCEEEDED(res) (res == SmcResult::Success)
    #define SMC_R_FAILED(res)     (res != SmcResult::Success)

    #define SMC_R_TRY(res_expr)     ({ const auto _tmp_r_try_rc = (res_expr); if (SMC_R_FAILED(_tmp_r_try_rc)) { return _tmp_r_try_rc; } })
    #define SMC_R_UNLESS(cond, RES) ({ if (!(cond)) { return SmcResult::RES; }})

    struct SmcArguments {
        u64 r[8];
    };

    constexpr inline int SecurityEngineUserInterruptId = 44;
    constexpr inline u64 InvalidAsyncKey = 0;

}
