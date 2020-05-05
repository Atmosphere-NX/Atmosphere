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

namespace ams::pkg1 {

    enum ErrorReason {
        ErrorReason_None                            = 0,
        ErrorReason_InvalidPackage2Signature        = 1,
        ErrorReason_InvalidPackage2Meta             = 2,
        ErrorReason_InvalidPackage2Version          = 3,
        ErrorReason_InvalidPackage2Payload          = 4,
        ErrorReason_UnknownSmc                      = 5,
        ErrorReason_UnknownAbort                    = 6,
        ErrorReason_InvalidCoreContext              = 7,
        ErrorReason_InvalidSecurityEngineStickyBits = 8,
        ErrorReason_UnexpectedReset                 = 9,

        ErrorReason_Exception                       = 0x10,

        ErrorReason_TransitionToSafeMode            = 0x20,
        ErrorReason_SecureInitializerReboot         = 0x21,

        ErrorReason_SdmmcError                      = 0x30,
        ErrorReason_InvalidDramId                   = 0x31,
        ErrorReason_InvalidPackage2                 = 0x32,
        ErrorReason_InvalidBct                      = 0x33,
        ErrorReason_InvalidGpt                      = 0x34,
        ErrorReason_FailedToTransitionToSafeMode    = 0x35,
        ErrorReason_ActivityMonitorInterrupt        = 0x36,

        ErrorReason_KernelPanic                     = 0x40,
    };

    enum ErrorColor {
        ErrorColor_Black     = 0x000,

        ErrorColor_Red       = 0x00F,
        ErrorColor_Yellow    = 0x0FF,
        ErrorColor_Orange    = 0x07F,
        ErrorColor_Blue      = 0xF00,
        ErrorColor_LightBlue = 0xFF0,
        ErrorColor_Pink      = 0xF7F,
        ErrorColor_Purple    = 0xF0A,
    };

    enum ErrorInfo {
        ErrorInfo_ReasonMask = 0xFF,
        ErrorInfo_ColorShift = 20,

        #define MAKE_ERROR_INFO(_COLOR_, _DESC_) ((static_cast<u32>(ErrorColor_##_COLOR_) << ErrorInfo_ColorShift) | (ErrorReason_##_DESC_))

        ErrorInfo_None                            = MAKE_ERROR_INFO(Black,     None),

        ErrorInfo_InvalidPackage2Signature        = MAKE_ERROR_INFO(Blue,      InvalidPackage2Signature),
        ErrorInfo_InvalidPackage2Meta             = MAKE_ERROR_INFO(Blue,      InvalidPackage2Meta),
        ErrorInfo_InvalidPackage2Version          = MAKE_ERROR_INFO(Blue,      InvalidPackage2Version),
        ErrorInfo_InvalidPackage2Payload          = MAKE_ERROR_INFO(Blue,      InvalidPackage2Payload),

        ErrorInfo_UnknownSmc                      = MAKE_ERROR_INFO(LightBlue, UnknownSmc),

        ErrorInfo_UnknownAbort                    = MAKE_ERROR_INFO(Yellow,    UnknownAbort),

        ErrorInfo_InvalidCoreContext              = MAKE_ERROR_INFO(Pink,      InvalidCoreContext),
        ErrorInfo_InvalidSecurityEngineStickyBits = MAKE_ERROR_INFO(Pink,      InvalidSecurityEngineStickyBits),
        ErrorInfo_UnexpectedReset                 = MAKE_ERROR_INFO(Pink,      UnexpectedReset),

        ErrorInfo_Exception                       = MAKE_ERROR_INFO(Orange,    Exception),

        ErrorInfo_TransitionToSafeMode            = MAKE_ERROR_INFO(Black,     TransitionToSafeMode),
        ErrorInfo_SecureInitializerReboot         = MAKE_ERROR_INFO(Black,     SecureInitializerReboot),

        ErrorInfo_SdmmcError                      = MAKE_ERROR_INFO(Purple,    SdmmcError),
        ErrorInfo_InvalidDramId                   = MAKE_ERROR_INFO(Purple,    InvalidDramId),
        ErrorInfo_InvalidPackage2                 = MAKE_ERROR_INFO(Purple,    InvalidPackage2),
        ErrorInfo_InvalidBct                      = MAKE_ERROR_INFO(Purple,    InvalidBct),
        ErrorInfo_InvalidGpt                      = MAKE_ERROR_INFO(Purple,    InvalidGpt),
        ErrorInfo_FailedToTransitionToSafeMode    = MAKE_ERROR_INFO(Purple,    FailedToTransitionToSafeMode),
        ErrorInfo_ActivityMonitorInterrupt        = MAKE_ERROR_INFO(Purple,    ActivityMonitorInterrupt),

        #undef MAKE_ERROR_INFO
    };

    constexpr inline ErrorReason GetErrorReason(u32 info) {
        return static_cast<ErrorReason>(info & ErrorInfo_ReasonMask);
    }

    constexpr inline ErrorInfo MakeKernelPanicResetInfo(u32 color) {
        return static_cast<ErrorInfo>((color << ErrorInfo_ColorShift) | (ErrorReason_KernelPanic));
    }

    #define PKG1_SECURE_MONITOR_PMC_ERROR_SCRATCH (0x840)

}
