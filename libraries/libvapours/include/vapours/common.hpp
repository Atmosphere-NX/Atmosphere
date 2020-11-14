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
#include <vapours/includes.hpp>
#include <vapours/defines.hpp>

#if defined(AMS_FORCE_DISABLE_DETAILED_ASSERTIONS) && defined(AMS_FORCE_ENABLE_DETAILED_ASSERTIONS)
    #error "Invalid detailed assertions state"
#endif

#ifdef  AMS_BUILD_FOR_AUDITING

    #define AMS_BUILD_FOR_DEBUGGING

    #if !defined(AMS_FORCE_DISABLE_DETAILED_ASSERTIONS)
        #define AMS_ENABLE_DETAILED_ASSERTIONS
    #endif

#endif

#ifdef  AMS_BUILD_FOR_DEBUGGING

    #define AMS_ENABLE_ASSERTIONS

    #if !defined(AMS_ENABLE_DETAILED_ASSERTIONS) && !defined(AMS_FORCE_DISABLE_DETAILED_ASSERTIONS)

        #if !defined(ATMOSPHERE_IS_EXOSPHERE) || defined(AMS_FORCE_ENABLE_DETAILED_ASSERTIONS)

            #define AMS_ENABLE_DETAILED_ASSERTIONS

        #endif

    #endif


#endif
