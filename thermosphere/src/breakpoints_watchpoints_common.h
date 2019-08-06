/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "types.h"
#include "sysreg.h"

/// BT[3:1] or res0. BT[0]/WT[0] is "is linked"
typedef enum BreakpointType {
    BreakpointType_AddressMatch = 0,
    BreakpointType_VheContextIdMatch = 1,
    BreakpointType_ContextIdMatch = 3,
    BreakpointType_VmidMatch = 4,
    BreakpointType_VmidContextIdMatch = 5,
    BreakpointType_VmidVheContextIdMatch = 6,
    BreakpointType_FullVheContextIdMatch = 7,
} BreakpointType;

// Note: some SSC HMC PMC combinations are invalid
// Refer to "Table D2-9 Summary of breakpoint HMC, SSC, and PMC encodings"

/// Debug Security State Control
typedef enum DebugSsc {
    DebugSsc_Both                  = 0,
    DebugSsc_NonSecure             = 1,
    DebugSsc_Secure                = 2,
    DebugSsc_SecureIfLowerOrBoth   = 3,
} DebugSsc;

/// Debug Higher Mode Control
typedef enum DebugHmc {
    DebugHmc_LowerEl   = 0,
    DebugHmc_HigherEl  = 1,
} DebugHmc;

/// Debug Privilege Mode Control (called PAC for watchpoints)
typedef enum DebugPmc {
    DebugPmc_NeitherEl1Nor0    = 0,
    DebugPmc_El1               = 1,
    DebugPmc_El0               = 2,
    DebugPmc_El1And0           = 3,
} DebugPmc;

typedef enum WatchpointLoadStoreControl {
    WatchpointLoadStoreControl_Load         = 1,
    WatchpointLoadStoreControl_Store        = 2,
    WatchpointLoadStoreControl_LoadStore    = 3,
} WatchpointLoadStoreControl;

// bas only 4 bits for breakpoints, other bits res0.
// lsc, mask only for watchpoints, res0 for breakpoints
// bt only from breakpoints, res0 for watchpoints
typedef struct DebugControlRegister {
    bool enabled                        : 1;
    DebugPmc pmc                        : 2;
    WatchpointLoadStoreControl lsc      : 2;
    u32 bas                             : 8;
    DebugHmc hmc                        : 1;
    DebugSsc ssc                        : 2;
    u32 lbn                             : 4;
    bool linked                         : 1;
    BreakpointType bt                   : 3;
    u32 mask                            : 5;
    u64 res0                            : 35;
} DebugControlRegister;

typedef struct DebugRegisterPair {
    DebugControlRegister cr;
    u64 vr;
} DebugRegisterPair;

static inline void enableBreakpointsAndWatchpoints(void)
{
    SET_SYSREG(mdscr_el1, GET_SYSREG(mdscr_el1) | MDSCR_EL1_MDE);
}
