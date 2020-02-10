/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "../defines.hpp"


namespace ams::hvisor::cpu {

    // TODO GCC 10, use enum class.
    // Would be nice if gcc didn't take 9+ years to fix a trivial bug ("too small to fit")
    struct DebugRegisterPair {
        // For breakpoints only
        /// BT[3:1] or res0. BT[0]/WT[0] is "is linked"
        enum BreakpointType {
            AddressMatch = 0,
            VheContextIdMatch = 1,
            ContextIdMatch = 3,
            VmidMatch = 4,
            VmidContextIdMatch = 5,
            VmidVheContextIdMatch = 6,
            FullVheContextIdMatch = 7,
        };

        // Note: some SSC HMC PMC combinations are invalid
        // Refer to "Table D2-9 Summary of breakpoint HMC, SSC, and PMC encodings"

        /// Security State Control
        enum SecurityStateControl {
            Both                  = 0,
            NonSecure             = 1,
            Secure                = 2,
            SecureIfLowerOrBoth   = 3,
        };

        /// Higher Mode Control
        enum HigherModeControl {
            LowerEl   = 0,
            HigherEl  = 1,
        };

        /// Privilege Mode Control (called PAC for watchpoints)
        enum PrivilegeModeControl {
            NeitherEl1Nor0    = 0,
            El1               = 1,
            El0               = 2,
            El1And0           = 3,
        };

        // Watchpoints only
        enum LoadStoreControl {
            NotAWatchpoint  = 0,
            Load            = 1,
            Store           = 2,
            LoadStore       = 3,
        };

        // bas only 4 bits for breakpoints, other bits res0.
        // lsc, mask only for watchpoints, res0 for breakpoints
        // bt only from breakpoints, res0 for watchpoints
        struct ControlRegister {
            union {
                struct {
                    bool enabled                        : 1;
                    PrivilegeModeControl pmc            : 2;
                    LoadStoreControl lsc                : 2;
                    u32 bas                             : 8;
                    HigherModeControl hmc               : 1;
                    SecurityStateControl ssc            : 2;
                    u32 lbn                             : 4;
                    bool linked                         : 1;
                    BreakpointType bt                   : 3;
                    u32 mask                            : 5;
                    u64 res0                            : 35;
                };
                u64 raw;
            };
        };

        ControlRegister cr;
        u64             vr;

        constexpr void SetDefaults()
        {
            cr.linked = false;

            // NS EL1&0 only
            cr.hmc = LowerEl;
            cr.ssc = NonSecure;
            cr.pmc = El1And0;
        }
    };

    static_assert(std::is_pod_v<DebugRegisterPair>);
}
