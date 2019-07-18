/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

/* libnx already has: static constexpr u32 Module_Kernel = 1; */

static constexpr Result ResultKernelOutOfSessions                = MAKERESULT(Module_Kernel, KernelError_OutOfSessions);

static constexpr Result ResultKernelInvalidCapabilityDescriptor  = MAKERESULT(Module_Kernel, KernelError_InvalidCapabilityDescriptor);

static constexpr Result ResultKernelNotImplemented               = MAKERESULT(Module_Kernel, KernelError_NotImplemented);
static constexpr Result ResultKernelThreadTerminating            = MAKERESULT(Module_Kernel, KernelError_ThreadTerminating);

static constexpr Result ResultKernelOutOfDebugEvents             = MAKERESULT(Module_Kernel, KernelError_OutOfDebugEvents);

static constexpr Result ResultKernelInvalidSize                  = MAKERESULT(Module_Kernel, KernelError_InvalidSize);
static constexpr Result ResultKernelInvalidAddress               = MAKERESULT(Module_Kernel, KernelError_InvalidAddress);
static constexpr Result ResultKernelResourceExhausted            = MAKERESULT(Module_Kernel, KernelError_ResourceExhausted);
static constexpr Result ResultKernelOutOfMemory                  = MAKERESULT(Module_Kernel, KernelError_OutOfMemory);
static constexpr Result ResultKernelOutOfHandles                 = MAKERESULT(Module_Kernel, KernelError_OutOfHandles);
static constexpr Result ResultKernelInvalidMemoryState           = MAKERESULT(Module_Kernel, KernelError_InvalidMemoryState);
static constexpr Result ResultKernelInvalidMemoryPermissions     = MAKERESULT(Module_Kernel, KernelError_InvalidMemoryPermissions);
static constexpr Result ResultKernelInvalidMemoryRange           = MAKERESULT(Module_Kernel, KernelError_InvalidMemoryRange);
static constexpr Result ResultKernelInvalidPriority              = MAKERESULT(Module_Kernel, KernelError_InvalidPriority);
static constexpr Result ResultKernelInvalidCoreId                = MAKERESULT(Module_Kernel, KernelError_InvalidCoreId);
static constexpr Result ResultKernelInvalidHandle                = MAKERESULT(Module_Kernel, KernelError_InvalidHandle);
static constexpr Result ResultKernelInvalidUserBuffer            = MAKERESULT(Module_Kernel, KernelError_InvalidUserBuffer);
static constexpr Result ResultKernelInvalidCombination           = MAKERESULT(Module_Kernel, KernelError_InvalidCombination);
static constexpr Result ResultKernelTimedOut                     = MAKERESULT(Module_Kernel, KernelError_TimedOut);
static constexpr Result ResultKernelCancelled                    = MAKERESULT(Module_Kernel, KernelError_Cancelled);
static constexpr Result ResultKernelOutOfRange                   = MAKERESULT(Module_Kernel, KernelError_OutOfRange);
static constexpr Result ResultKernelInvalidEnumValue             = MAKERESULT(Module_Kernel, KernelError_InvalidEnumValue);
static constexpr Result ResultKernelNotFound                     = MAKERESULT(Module_Kernel, KernelError_NotFound);
static constexpr Result ResultKernelAlreadyExists                = MAKERESULT(Module_Kernel, KernelError_AlreadyExists);
static constexpr Result ResultKernelConnectionClosed             = MAKERESULT(Module_Kernel, KernelError_ConnectionClosed);
static constexpr Result ResultKernelUnhandledUserInterrupt       = MAKERESULT(Module_Kernel, KernelError_UnhandledUserInterrupt);
static constexpr Result ResultKernelInvalidState                 = MAKERESULT(Module_Kernel, KernelError_InvalidState);
static constexpr Result ResultKernelReservedValue                = MAKERESULT(Module_Kernel, KernelError_ReservedValue);
static constexpr Result ResultKernelInvalidHwBreakpoint          = MAKERESULT(Module_Kernel, KernelError_InvalidHwBreakpoint);
static constexpr Result ResultKernelFatalUserException           = MAKERESULT(Module_Kernel, KernelError_FatalUserException);
static constexpr Result ResultKernelOwnedByAnotherProcess        = MAKERESULT(Module_Kernel, KernelError_OwnedByAnotherProcess);
static constexpr Result ResultKernelConnectionRefused            = MAKERESULT(Module_Kernel, KernelError_ConnectionRefused);
static constexpr Result ResultKernelLimitReached                 = MAKERESULT(Module_Kernel, 132 /* KernelError_OutOfResource */);

static constexpr Result ResultKernelIpcMapFailed                 = MAKERESULT(Module_Kernel, KernelError_IpcMapFailed);
static constexpr Result ResultKernelIpcCmdBufTooSmall            = MAKERESULT(Module_Kernel, KernelError_IpcCmdbufTooSmall);

static constexpr Result ResultKernelNotDebugged                  = MAKERESULT(Module_Kernel, KernelError_NotDebugged);
