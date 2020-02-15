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

    void KInterruptController::SetupInterruptLines(s32 core_id) const {
        const size_t ITLines = (core_id == 0) ? 32 * ((this->gicd->typer & 0x1F) + 1) : NumLocalInterrupts;

        for (size_t i = 0; i < ITLines / 32; i++) {
            this->gicd->icenabler[i] = 0xFFFFFFFF;
            this->gicd->icpendr[i]   = 0xFFFFFFFF;
            this->gicd->icactiver[i] = 0xFFFFFFFF;
            this->gicd->igroupr[i]   = 0;
        }

        for (size_t i = 0; i < ITLines; i++) {
            this->gicd->ipriorityr.bytes[i] = 0xFF;
            this->gicd->itargetsr.bytes[i]  = 0x00;
        }

        for (size_t i = 0; i < ITLines / 16; i++) {
            this->gicd->icfgr[i] = 0x00000000;
        }
    }

    void KInterruptController::Initialize(s32 core_id) {
        /* Setup pointers to ARM mmio. */
        this->gicd = GetPointer<volatile  GicDistributor>(KMemoryLayout::GetInterruptDistributorAddress());
        this->gicc = GetPointer<volatile GicCpuInterface>(KMemoryLayout::GetInterruptCpuInterfaceAddress());

        /* Clear CTLRs. */
        this->gicc->ctlr = 0;
        if (core_id == 0) {
            this->gicd->ctlr = 0;
        }

        this->gicc->pmr = 0;
        this->gicc->bpr = 7;

        /* Setup all interrupt lines. */
        SetupInterruptLines(core_id);

        /* Set CTLRs. */
        if (core_id == 0) {
            this->gicd->ctlr = 1;
        }
        this->gicc->ctlr = 1;

        /* Set the mask for this core. */
        SetGicMask(core_id);

        /* Set the priority level. */
        SetPriorityLevel(PriorityLevel_Low);
    }

    void KInterruptController::Finalize(s32 core_id) {
        /* Clear CTLRs. */
        if (core_id == 0) {
            this->gicd->ctlr = 0;
        }
        this->gicc->ctlr = 0;

        /* Set the priority level. */
        SetPriorityLevel(PriorityLevel_High);

        /* Setup all interrupt lines. */
        SetupInterruptLines(core_id);

        this->gicd = nullptr;
        this->gicc = nullptr;
    }

}
