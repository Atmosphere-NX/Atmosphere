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
#include <vapours/svc/svc_common.hpp>

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)

    #include <vapours/svc/board/nintendo/nx/svc_io_pool_type.hpp>
    namespace ams::svc {

        using IoPoolType = ::ams::svc::board::nintendo::nx::IoPoolType;
        using enum ::ams::svc::board::nintendo::nx::IoPoolType;

    }

#else

    #define AMS_SVC_IO_POOL_NOT_SUPPORTED

    namespace ams::svc {

        enum IoPoolType : u32 {
            /* Not supported. */
            IoPoolType_Count = 0,
        };

    }

#endif
