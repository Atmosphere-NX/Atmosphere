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
#include <stratosphere.hpp>

/* TODO: How much of this should be namespaced under BOARD_NINTENDO_NX? */

namespace ams::wec {

    namespace {

        constexpr inline dd::PhysicalAddress ApbdevPmc = 0x7000E400;

        constinit bool g_initialized = false;

        void UpdateControlBit(dd::PhysicalAddress phys_addr, u32 mask, bool flag) {
            dd::ReadModifyWriteIoRegister(phys_addr, flag ? ~0u : 0u, mask);
            dd::ReadIoRegister(phys_addr);
        }

        void Initialize(bool blink) {
            /* Initialize WAKE_DEBOUNCE_EN. */
            dd::WriteIoRegister(ApbdevPmc + APBDEV_PMC_WAKE_DEBOUNCE_EN, 0);
            dd::ReadIoRegister(ApbdevPmc + APBDEV_PMC_WAKE_DEBOUNCE_EN);

            /* Initialize BLINK_TIMER. */
            dd::WriteIoRegister(ApbdevPmc + APBDEV_PMC_BLINK_TIMER, 0x08008800);
            dd::ReadIoRegister(ApbdevPmc + APBDEV_PMC_BLINK_TIMER);

            /* Set control configs. */
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL,  0x0800,  true);
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL,  0x0400, false);
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL,  0x0200,  true);
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL,  0x0100, false);
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL,  0x0040, false);
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL,  0x0020, false);
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL2, 0x4000,  true);
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL2, 0x0200, false);
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL2, 0x0001,  true);

            /* Update blink bit in APBDEV_PMC_CNTRL. */
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_CNTRL,  0x0080, blink);

            /* Update blink bit in APBDEV_PMC_DPD_PADS_ORIDE. */
            UpdateControlBit(ApbdevPmc + APBDEV_PMC_DPD_PADS_ORIDE, 0x100000, blink);
        }

    }

    void Initialize() {
        /* Set initial wake configuration. */
        if (!g_initialized) {
            Initialize(false);
            g_initialized = true;
        }
    }

    void ClearWakeEvents() {
        /* TODO */
        AMS_ABORT();
    }

    void WecRestoreForExitSuspend() {
        /* TODO */
        AMS_ABORT();
    }

    void SetWakeEventLevel(wec::WakeEvent event, wec::WakeEventLevel level) {
        if (g_initialized && event < wec::WakeEvent_Count) {
            /* Determine the event index. */
            const bool which = static_cast<u32>(event) < BITSIZEOF(u32);
            const u32  index = static_cast<u32>(event) & (BITSIZEOF(u32) - 1);

            /* Get the level and auto_mask offsets. */
            u32 level_ofs     = which ? APBDEV_PMC_WAKE_LVL : APBDEV_PMC_WAKE2_LVL;
            u32 auto_mask_ofs = which ? APBDEV_PMC_AUTO_WAKE_LVL_MASK : APBDEV_PMC_AUTO_WAKE2_LVL_MASK;

            /* If the level isn't auto, swap the offsets. */
            if (level != wec::WakeEventLevel_Auto) {
                std::swap(level_ofs, auto_mask_ofs);
            }

            /* Clear the bit in the level register. */
            UpdateControlBit(ApbdevPmc + level_ofs, (1u << index), false);

            /* Set or clear the bit in the auto mask register. */
            UpdateControlBit(ApbdevPmc + auto_mask_ofs, (1u << index), level != wec::WakeEventLevel_Low);
        }
    }

    void SetWakeEventEnabled(wec::WakeEvent event, bool en) {
        if (g_initialized && event < wec::WakeEvent_Count) {
            /* Set or clear the relevant enabled bit. */
            const u32 offset = static_cast<u32>(event) < BITSIZEOF(u32) ? APBDEV_PMC_WAKE_MASK : APBDEV_PMC_WAKE2_MASK;
            const u32 index  = static_cast<u32>(event) & (BITSIZEOF(u32) - 1);
            UpdateControlBit(ApbdevPmc + offset, (1u << index), en);
        }
    }

}
