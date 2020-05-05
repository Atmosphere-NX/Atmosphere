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

#if 1 || defined(AMS_BUILD_FOR_AUDITING)
#define EXOSPHERE_BUILD_FOR_AUDITING
#endif

#if defined(EXOSPHERE_BUILD_FOR_AUDITING) || defined(AMS_BUILD_FOR_DEBUGGING)
#define EXOSPHERE_BUILD_FOR_DEBUGGING
#endif

#ifdef  EXOSPHERE_BUILD_FOR_DEBUGGING
#define EXOSPHERE_ENABLE_ASSERTIONS
#define EXOSPHERE_ENABLE_DEBUG_PRINT
#endif
