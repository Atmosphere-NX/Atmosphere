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
#include <mesosphere.hpp>

namespace ams::kern::arch::arm {

    void KInterruptController::SetupInterruptLines(s32 core_id) const {
        const size_t ITLines = (core_id == 0) ? 32 * ((m_gicd->typer & 0x1F) + 1) : NumLocalInterrupts;

        for (size_t i = 0; i < ITLines / 32; i++) {
            m_gicd->icenabler[i] = 0xFFFFFFFF;
            m_gicd->icpendr[i]   = 0xFFFFFFFF;
            m_gicd->icactiver[i] = 0xFFFFFFFF;
            m_gicd->igroupr[i]   = 0;
        }

        for (size_t i = 0; i < ITLines; i++) {
            m_gicd->ipriorityr.bytes[i] = 0xFF;
            m_gicd->itargetsr.bytes[i]  = 0x00;
        }

        for (size_t i = 0; i < ITLines / 16; i++) {
            m_gicd->icfgr[i] = 0x00000000;
        }
    }

    void KInterruptController::Initialize(s32 core_id) {
        /* Setup pointers to ARM mmio. */
        m_gicd = GetPointer<volatile GicDistributor >(KMemoryLayout::GetDeviceVirtualAddress(KMemoryRegionType_InterruptDistributor));
        m_gicc = GetPointer<volatile GicCpuInterface>(KMemoryLayout::GetDeviceVirtualAddress(KMemoryRegionType_InterruptCpuInterface));

        /* Clear CTLRs. */
        m_gicc->ctlr = 0;
        if (core_id == 0) {
            m_gicd->ctlr = 0;
        }

        m_gicc->pmr = 0;
        m_gicc->bpr = 7;

        /* Setup all interrupt lines. */
        SetupInterruptLines(core_id);

        /* Set CTLRs. */
        if (core_id == 0) {
            m_gicd->ctlr = 1;
        }
        m_gicc->ctlr = 1;

        /* Set the mask for this core. */
        SetGicMask(core_id);

        /* Set the priority level. */
        SetPriorityLevel(PriorityLevel_Low);
    }

    void KInterruptController::Finalize(s32 core_id) {
        /* Clear CTLRs. */
        if (core_id == 0) {
            m_gicd->ctlr = 0;
        }
        m_gicc->ctlr = 0;

        /* Set the priority level. */
        SetPriorityLevel(PriorityLevel_High);

        /* Setup all interrupt lines. */
        SetupInterruptLines(core_id);

        /* Clear pointers, if needed. */
        if (core_id == 0) {
            m_gicd = nullptr;
            m_gicc = nullptr;
        }
    }

    void KInterruptController::SaveCoreLocal(LocalState *state) const {
        /* Save isenabler. */
        for (size_t i = 0; i < util::size(state->isenabler); ++i) {
            constexpr size_t Offset = 0;
            state->isenabler[i] = m_gicd->isenabler[i + Offset];
            m_gicd->isenabler[i + Offset] = 0xFFFFFFFF;
        }

        /* Save ipriorityr. */
        for (size_t i = 0; i < util::size(state->ipriorityr); ++i) {
            constexpr size_t Offset = 0;
            state->ipriorityr[i] = m_gicd->ipriorityr.words[i + Offset];
            m_gicd->ipriorityr.words[i + Offset] = 0xFFFFFFFF;
        }

        /* Save itargetsr. */
        for (size_t i = 0; i < util::size(state->itargetsr); ++i) {
            constexpr size_t Offset = 0;
            state->itargetsr[i] = m_gicd->itargetsr.words[i + Offset];
        }

        /* Save icfgr. */
        for (size_t i = 0; i < util::size(state->icfgr); ++i) {
            constexpr size_t Offset = 0;
            state->icfgr[i] = m_gicd->icfgr[i + Offset];
        }

        /* Save spendsgir. */
        for (size_t i = 0; i < util::size(state->spendsgir); ++i) {
            state->spendsgir[i] = m_gicd->spendsgir[i];
        }
    }

    void KInterruptController::SaveGlobal(GlobalState *state) const {
        /* Save isenabler. */
        for (size_t i = 0; i < util::size(state->isenabler); ++i) {
            constexpr size_t Offset = util::size(LocalState{}.isenabler);
            state->isenabler[i] = m_gicd->isenabler[i + Offset];
            m_gicd->isenabler[i + Offset] = 0xFFFFFFFF;
        }

        /* Save ipriorityr. */
        for (size_t i = 0; i < util::size(state->ipriorityr); ++i) {
            constexpr size_t Offset = util::size(LocalState{}.ipriorityr);
            state->ipriorityr[i] = m_gicd->ipriorityr.words[i + Offset];
            m_gicd->ipriorityr.words[i + Offset] = 0xFFFFFFFF;
        }

        /* Save itargetsr. */
        for (size_t i = 0; i < util::size(state->itargetsr); ++i) {
            constexpr size_t Offset = util::size(LocalState{}.itargetsr);
            state->itargetsr[i] = m_gicd->itargetsr.words[i + Offset];
        }

        /* Save icfgr. */
        for (size_t i = 0; i < util::size(state->icfgr); ++i) {
            constexpr size_t Offset = util::size(LocalState{}.icfgr);
            state->icfgr[i] = m_gicd->icfgr[i + Offset];
        }
    }

    void KInterruptController::RestoreCoreLocal(const LocalState *state) const {
        /* Restore ipriorityr. */
        for (size_t i = 0; i < util::size(state->ipriorityr); ++i) {
            constexpr size_t Offset = 0;
            m_gicd->ipriorityr.words[i + Offset] = state->ipriorityr[i];
        }

        /* Restore itargetsr. */
        for (size_t i = 0; i < util::size(state->itargetsr); ++i) {
            constexpr size_t Offset = 0;
            m_gicd->itargetsr.words[i + Offset] = state->itargetsr[i];
        }

        /* Restore icfgr. */
        for (size_t i = 0; i < util::size(state->icfgr); ++i) {
            constexpr size_t Offset = 0;
            m_gicd->icfgr[i + Offset] = state->icfgr[i];
        }

        /* Restore isenabler. */
        for (size_t i = 0; i < util::size(state->isenabler); ++i) {
            constexpr size_t Offset = 0;
            m_gicd->icenabler[i + Offset] = 0xFFFFFFFF;
            m_gicd->isenabler[i + Offset] = state->isenabler[i];
        }

        /* Restore spendsgir. */
        for (size_t i = 0; i < util::size(state->spendsgir); ++i) {
            m_gicd->spendsgir[i] = state->spendsgir[i];
        }
    }

    void KInterruptController::RestoreGlobal(const GlobalState *state) const {
        /* Restore ipriorityr. */
        for (size_t i = 0; i < util::size(state->ipriorityr); ++i) {
            constexpr size_t Offset = util::size(LocalState{}.ipriorityr);
            m_gicd->ipriorityr.words[i + Offset] = state->ipriorityr[i];
        }

        /* Restore itargetsr. */
        for (size_t i = 0; i < util::size(state->itargetsr); ++i) {
            constexpr size_t Offset = util::size(LocalState{}.itargetsr);
            m_gicd->itargetsr.words[i + Offset] = state->itargetsr[i];
        }

        /* Restore icfgr. */
        for (size_t i = 0; i < util::size(state->icfgr); ++i) {
            constexpr size_t Offset = util::size(LocalState{}.icfgr);
            m_gicd->icfgr[i + Offset] = state->icfgr[i];
        }

        /* Restore isenabler. */
        for (size_t i = 0; i < util::size(state->isenabler); ++i) {
            constexpr size_t Offset = util::size(LocalState{}.isenabler);
            m_gicd->icenabler[i + Offset] = 0xFFFFFFFF;
            m_gicd->isenabler[i + Offset] = state->isenabler[i];
        }
    }

}
