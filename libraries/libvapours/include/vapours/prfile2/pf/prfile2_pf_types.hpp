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
#include <vapours/prfile2/prfile2_build_config.hpp>

namespace ams::prfile2::pf {

    enum Error {
        Error_NoError        =   0,
        Error_Ok             =   Error_NoError,

        Error_EPERM          =   1,
        Error_ENOENT         =   2,
        Error_ESRCH          =   3,
        Error_EIO            =   5,
        Error_ENOEXEC        =   8,
        Error_EBADF          =   9,
        Error_ENOMEM         =  12,
        Error_EACCES         =  13,
        Error_EBUSY          =  16,
        Error_EEXIST         =  17,
        Error_ENODEV         =  19,
        Error_EISDIR         =  21,
        Error_EINVAL         =  22,
        Error_ENFILE         =  23,
        Error_EMFILE         =  24,
        Error_EFBIG          =  27,
        Error_ENOSPC         =  28,
        Error_ENOLCK         =  46,
        Error_ENOSYS         =  88,
        Error_ENOTEMPTY      =  90,

        Error_EMOD_NOTREG    = 100,
        Error_EMOD_NOTSPRT   = 101,
        Error_EMOD_FCS       = 102,
        Error_EMOD_SAFE      = 103,

        Error_ENOMEDIUM      = 123,

        Error_EEXFAT_NOTSPRT = 200,

        Error_DFNC           = 0x1000,

        Error_SYSTEM         = -1,
    };

    constexpr inline const int ReturnValueNoError = 0;
    constexpr inline const int ReturnValueError   = -1;

    constexpr inline int ConvertReturnValue(Error err) {
        if (AMS_LIKELY(err == Error_NoError)) {
            return ReturnValueNoError;
        } else {
            return ReturnValueError;
        }
    }

}
