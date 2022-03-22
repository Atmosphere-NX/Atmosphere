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
#include <vapours.hpp>

#if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
    #define AMS_IMPL_ENABLE_SDK_LOG
#else
    //#define AMS_IMPL_ENABLE_SDK_LOG
#endif

#if defined(AMS_ENABLE_LOG)
    #define AMS_IMPL_ENABLE_LOG

    #if defined(AMS_DISABLE_LOG)
        #error "Incoherent log configuration"
    #endif
#elif defined(AMS_DISABLE_LOG)

#elif defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
    #define AMS_IMPL_ENABLE_LOG
#else
    //#define AMS_IMPL_ENABLE_LOG
#endif
