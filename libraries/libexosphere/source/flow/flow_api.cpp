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
#include <exosphere.hpp>

namespace ams::flow {

    namespace {

        struct FlowControllerRegisterOffset {
            u16 cpu_csr;
            u16 halt_cpu_events;
            u16 cc4_core_ctrl;
        };

        constinit uintptr_t g_register_address = secmon::MemoryRegionPhysicalDeviceFlowController.GetAddress();

        constexpr const FlowControllerRegisterOffset FlowControllerRegisterOffsets[] = {
            { FLOW_CTLR_CPU0_CSR, FLOW_CTLR_HALT_CPU0_EVENTS, FLOW_CTLR_CC4_CORE0_CTRL, },
            { FLOW_CTLR_CPU1_CSR, FLOW_CTLR_HALT_CPU1_EVENTS, FLOW_CTLR_CC4_CORE1_CTRL, },
            { FLOW_CTLR_CPU2_CSR, FLOW_CTLR_HALT_CPU2_EVENTS, FLOW_CTLR_CC4_CORE2_CTRL, },
            { FLOW_CTLR_CPU3_CSR, FLOW_CTLR_HALT_CPU3_EVENTS, FLOW_CTLR_CC4_CORE3_CTRL, },
        };

        constexpr u32 GetHaltCpuEventsValue(bool resume_on_irq) {
            if (resume_on_irq) {
                return reg::Encode(FLOW_REG_BITS_ENUM(HALT_CPUN_EVENTS_FLOW_MODE, WAITEVENT),
                                   FLOW_REG_BITS_ENUM(HALT_CPUN_EVENTS_LIC_IRQN,     ENABLE),
                                   FLOW_REG_BITS_ENUM(HALT_CPUN_EVENTS_LIC_FIQN,     ENABLE),
                                   FLOW_REG_BITS_ENUM(HALT_CPUN_EVENTS_GIC_IRQN,     ENABLE),
                                   FLOW_REG_BITS_ENUM(HALT_CPUN_EVENTS_GIC_FIQN,     ENABLE));
            } else {
                return reg::Encode(FLOW_REG_BITS_ENUM(HALT_CPUN_EVENTS_FLOW_MODE, WAITEVENT));
            }
        }

    }

    void SetRegisterAddress(uintptr_t address) {
        g_register_address = address;
    }

    void ResetCpuRegisters(int core) {
        AMS_ASSUME(core >= 0);

        const auto &offsets = FlowControllerRegisterOffsets[core];
        reg::Write(g_register_address + offsets.cpu_csr,         0);
        reg::Write(g_register_address + offsets.halt_cpu_events, 0);
    }

    void SetCpuCsr(int core, u32 enable_ext) {
        reg::Write(g_register_address + FlowControllerRegisterOffsets[core].cpu_csr, FLOW_REG_BITS_ENUM (CPUN_CSR_INTR_FLAG,               TRUE),
                                                                                     FLOW_REG_BITS_ENUM (CPUN_CSR_EVENT_FLAG,              TRUE),
                                                                                     FLOW_REG_BITS_VALUE(CPUN_CSR_ENABLE_EXT,        enable_ext),
                                                                                     FLOW_REG_BITS_VALUE(CPUN_CSR_WAIT_WFI_BITMAP, (1u << core)),
                                                                                     FLOW_REG_BITS_ENUM (CPUN_CSR_ENABLE,                ENABLE));
    }

    void SetHaltCpuEvents(int core, bool resume_on_irq) {
        reg::Write(g_register_address + FlowControllerRegisterOffsets[core].halt_cpu_events, GetHaltCpuEventsValue(resume_on_irq));
    }

    void SetCc4Ctrl(int core, u32 value) {
        reg::Write(g_register_address + FlowControllerRegisterOffsets[core].cc4_core_ctrl, value);
    }

    void ClearL2FlushControl() {
        reg::Write(g_register_address + FLOW_CTLR_L2FLUSH_CONTROL, 0);
    }

}
