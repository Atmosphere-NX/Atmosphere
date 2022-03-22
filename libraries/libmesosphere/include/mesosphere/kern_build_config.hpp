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

#if defined(AMS_BUILD_FOR_AUDITING)
#define MESOSPHERE_BUILD_FOR_AUDITING
#endif

#if defined(MESOSPHERE_BUILD_FOR_AUDITING) || defined(AMS_BUILD_FOR_DEBUGGING)
#define MESOSPHERE_BUILD_FOR_DEBUGGING
#endif

#ifdef  MESOSPHERE_BUILD_FOR_DEBUGGING
#define MESOSPHERE_ENABLE_ASSERTIONS
#define MESOSPHERE_ENABLE_DEBUG_PRINT
#define MESOSPHERE_ENABLE_KERNEL_STACK_USAGE
#endif

//#define MESOSPHERE_BUILD_FOR_TRACING
#define MESOSPHERE_ENABLE_PANIC_REGISTER_DUMP
#define MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP

/* NOTE: This enables fast class token storage using a class member. */
/* This saves a virtual call when doing KAutoObject->DynCast<>(), */
/* at the cost of storing class tokens inside the class object. */
/* However, as of (10/16/2021) KAutoObject has an unused class member */
/* of the right side, and so this does not actually cost any space. */
#define MESOSPHERE_ENABLE_DEVIRTUALIZED_DYNAMIC_CAST

/* NOTE: This enables usage of KDebug handles as parameter for svc::GetInfo */
/* calls which require a process parameter. This enables a debugger to obtain */
/* address space/layout information, for example. However, it changes abi, and so */
/* this define allows toggling the extension. */
#define MESOSPHERE_ENABLE_GET_INFO_OF_DEBUG_PROCESS

/* NOTE: This uses currently-reserved bits inside the MapRange capability */
/* in order to support large physical addresses (40-bit instead of 36). */
/* This is toggleable in order to disable it if N ever uses those bits. */
#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
//#define MESOSPHERE_ENABLE_LARGE_PHYSICAL_ADDRESS_CAPABILITIES
#else
#define MESOSPHERE_ENABLE_LARGE_PHYSICAL_ADDRESS_CAPABILITIES
#endif