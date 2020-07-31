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
#include <mesosphere.hpp>

namespace ams::kern::arch::arm64 {

    namespace {

        constexpr inline u64 ForbiddenBreakPointFlagsMask = (((1ul << 40) - 1) << 24) | /* Reserved upper bits. */
                                                            (((1ul <<  1) - 1) << 23) | /* Match VMID BreakPoint Type. */
                                                            (((1ul <<  2) - 1) << 14) | /* Security State Control. */
                                                            (((1ul <<  1) - 1) << 13) | /* Hyp Mode Control. */
                                                            (((1ul <<  4) - 1) <<  9) | /* Reserved middle bits. */
                                                            (((1ul <<  2) - 1) <<  3) | /* Reserved lower bits. */
                                                            (((1ul <<  2) - 1) <<  1);  /* Privileged Mode Control. */

        static_assert(ForbiddenBreakPointFlagsMask == 0xFFFFFFFFFF80FE1Eul);

        constexpr inline u64 ForbiddenWatchPointFlagsMask = (((1ul << 32) - 1) << 32) | /* Reserved upper bits. */
                                                            (((1ul <<  4) - 1) << 20) | /* WatchPoint Type. */
                                                            (((1ul <<  2) - 1) << 14) | /* Security State Control. */
                                                            (((1ul <<  1) - 1) << 13) | /* Hyp Mode Control. */
                                                            (((1ul <<  2) - 1) <<  1);  /* Privileged Access Control. */

        static_assert(ForbiddenWatchPointFlagsMask == 0xFFFFFFFF00F0E006ul);
    }

    #define MESOSPHERE_SET_HW_BREAK_POINT(ID, FLAGS, VALUE) \
        ({                                                  \
            cpu::SetDbgBcr##ID##El1(0);                     \
            cpu::EnsureInstructionConsistency();            \
            cpu::SetDbgBvr##ID##El1(VALUE);                 \
            cpu::EnsureInstructionConsistency();            \
            cpu::SetDbgBcr##ID##El1(FLAGS);                 \
            cpu::EnsureInstructionConsistency();            \
        })

    #define MESOSPHERE_SET_HW_WATCH_POINT(ID, FLAGS, VALUE) \
        ({                                                  \
            cpu::SetDbgWcr##ID##El1(0);                     \
            cpu::EnsureInstructionConsistency();            \
            cpu::SetDbgWvr##ID##El1(VALUE);                 \
            cpu::EnsureInstructionConsistency();            \
            cpu::SetDbgWcr##ID##El1(FLAGS);                 \
            cpu::EnsureInstructionConsistency();            \
        })

    Result KDebug::SetHardwareBreakPoint(ams::svc::HardwareBreakPointRegisterName name, u64 flags, u64 value) {
        /* Get the debug feature register. */
        cpu::DebugFeatureRegisterAccessor dfr0;

        /* Extract interesting info from the debug feature register. */
        const auto num_bp  = dfr0.GetNumBreakpoints();
        const auto num_wp  = dfr0.GetNumWatchpoints();
        const auto num_ctx = dfr0.GetNumContextAwareBreakpoints();

        if (ams::svc::HardwareBreakPointRegisterName_I0 <= name && name <= ams::svc::HardwareBreakPointRegisterName_I15) {
            /* Check that the name is a valid instruction breakpoint. */
            R_UNLESS((name - ams::svc::HardwareBreakPointRegisterName_I0) <= num_bp, svc::ResultNotSupported());

            /* We may be getting the process, so prepare a scoped reference holder. */
            KScopedAutoObject<KProcess> process;

            /* Configure flags/value. */
            if ((flags & 1) != 0) {
                /* We're enabling the breakpoint. Check that the flags are allowable. */
                R_UNLESS((flags & ForbiddenBreakPointFlagsMask) == 0, svc::ResultInvalidCombination());

                /* Require that the breakpoint be linked or match context id. */
                R_UNLESS((flags & ((1ul << 21) | (1ul << 20))) != 0, svc::ResultInvalidCombination());

                /* If the breakpoint matches context id, we need to get the context id. */
                if ((flags & (1ul << 21)) != 0) {
                    /* Ensure that the breakpoint is context-aware. */
                    R_UNLESS((name - ams::svc::HardwareBreakPointRegisterName_I0) <= (num_bp - num_ctx), svc::ResultNotSupported());

                    /* Check that the breakpoint does not have the mismatch bit. */
                    R_UNLESS((flags & (1ul << 22)) == 0, svc::ResultInvalidCombination());

                    /* Get the debug object from the current handle table. */
                    KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(static_cast<ams::svc::Handle>(value));
                    R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

                    /* Get the process from the debug object. */
                    process = debug->GetProcess();
                    R_UNLESS(process.IsNotNull(), svc::ResultProcessTerminated());

                    /* Set the value to be the context id. */
                    value = process->GetId() & 0xFFFFFFFF;
                }

                /* Set the breakpoint as non-secure EL0-only. */
                flags |= (1ul << 14) | (2ul << 1);
            } else {
                /* We're disabling the breakpoint. */
                flags = 0;
                value = 0;
            }

            /* Set the breakpoint. */
            switch (name) {
                case ams::svc::HardwareBreakPointRegisterName_I0:  MESOSPHERE_SET_HW_BREAK_POINT( 0, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I1:  MESOSPHERE_SET_HW_BREAK_POINT( 1, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I2:  MESOSPHERE_SET_HW_BREAK_POINT( 2, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I3:  MESOSPHERE_SET_HW_BREAK_POINT( 3, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I4:  MESOSPHERE_SET_HW_BREAK_POINT( 4, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I5:  MESOSPHERE_SET_HW_BREAK_POINT( 5, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I6:  MESOSPHERE_SET_HW_BREAK_POINT( 6, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I7:  MESOSPHERE_SET_HW_BREAK_POINT( 7, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I8:  MESOSPHERE_SET_HW_BREAK_POINT( 8, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I9:  MESOSPHERE_SET_HW_BREAK_POINT( 9, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I10: MESOSPHERE_SET_HW_BREAK_POINT(10, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I11: MESOSPHERE_SET_HW_BREAK_POINT(11, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I12: MESOSPHERE_SET_HW_BREAK_POINT(12, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I13: MESOSPHERE_SET_HW_BREAK_POINT(13, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I14: MESOSPHERE_SET_HW_BREAK_POINT(14, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I15: MESOSPHERE_SET_HW_BREAK_POINT(15, flags, value); break;
                default: break;
            }
        } else if (ams::svc::HardwareBreakPointRegisterName_D0 <= name && name <= ams::svc::HardwareBreakPointRegisterName_D15) {
            /* Check that the name is a valid data breakpoint. */
            R_UNLESS((name - ams::svc::HardwareBreakPointRegisterName_D0) <= num_wp, svc::ResultNotSupported());

            /* Configure flags/value. */
            if ((flags & 1) != 0) {
                /* We're enabling the watchpoint. Check that the flags are allowable. */
                R_UNLESS((flags & ForbiddenWatchPointFlagsMask) == 0, svc::ResultInvalidCombination());

                /* Set the breakpoint as linked non-secure EL0-only. */
                flags |= (1ul << 20) | (1ul << 14) | (2ul << 1);
            } else {
                /* We're disabling the watchpoint. */
                flags = 0;
                value = 0;
            }

            /* Set the watchkpoint. */
            switch (name) {
                case ams::svc::HardwareBreakPointRegisterName_D0:  MESOSPHERE_SET_HW_WATCH_POINT( 0, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D1:  MESOSPHERE_SET_HW_WATCH_POINT( 1, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D2:  MESOSPHERE_SET_HW_WATCH_POINT( 2, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D3:  MESOSPHERE_SET_HW_WATCH_POINT( 3, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D4:  MESOSPHERE_SET_HW_WATCH_POINT( 4, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D5:  MESOSPHERE_SET_HW_WATCH_POINT( 5, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D6:  MESOSPHERE_SET_HW_WATCH_POINT( 6, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D7:  MESOSPHERE_SET_HW_WATCH_POINT( 7, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D8:  MESOSPHERE_SET_HW_WATCH_POINT( 8, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D9:  MESOSPHERE_SET_HW_WATCH_POINT( 9, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D10: MESOSPHERE_SET_HW_WATCH_POINT(10, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D11: MESOSPHERE_SET_HW_WATCH_POINT(11, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D12: MESOSPHERE_SET_HW_WATCH_POINT(12, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D13: MESOSPHERE_SET_HW_WATCH_POINT(13, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D14: MESOSPHERE_SET_HW_WATCH_POINT(14, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D15: MESOSPHERE_SET_HW_WATCH_POINT(15, flags, value);  break;
                default: break;
            }
        } else {
            /* Invalid name. */
            return svc::ResultInvalidEnumValue();
        }

        return ResultSuccess();
    }

    #undef MESOSPHERE_SET_HW_WATCH_POINT
    #undef MESOSPHERE_SET_HW_BREAK_POINT

}
