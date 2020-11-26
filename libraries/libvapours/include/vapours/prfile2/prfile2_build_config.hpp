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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/results.hpp>
#include <vapours/util.hpp>
#include <vapours/svc.hpp>
#include <vapours/dd.hpp>

#if defined(ATMOSPHERE_IS_EXOSPHERE) || defined(ATMOSPHERE_IS_MESOSPHERE)

    //#define AMS_PRFILE2_THREAD_SAFE

#elif defined(ATMOSPHERE_IS_MESOSPHERE)

    //#define AMS_PRFILE2_THREAD_SAFE

#elif defined(ATMOSPHERE_IS_STRATOSPHERE)

    #define AMS_PRFILE2_THREAD_SAFE

#else
    #error "Unknown execution context for ams::prfile2!"
#endif
