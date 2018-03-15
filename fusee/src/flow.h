#ifndef FUSEE_FLOW_CTLR_H
#define FUSEE_FLOW_CTLR_H

#include "utils.h"

#define FLOW_BASE 0x60007000

#define MAKE_FLOW_REG(ofs) MAKE_REG32(FLOW_BASE + ofs)

#define FLOW_CTLR_HALT_COP_EVENTS_0 MAKE_FLOW_REG(0x004)
#define FLOW_CTLR_FLOW_DBG_QUAL_0   MAKE_FLOW_REG(0x050)
#define FLOW_CTLR_L2FLUSH_CONTROL_0 MAKE_FLOW_REG(0x094)
#define FLOW_CTLR_BPMP_CLUSTER_CONTROL_0 MAKE_FLOW_REG(0x098)


static const struct {
    unsigned int CPUN_CSR_OFS;
    unsigned int HALT_CPUN_EVENTS_OFS;
    unsigned int CC4_COREN_CTRL_OFS;
} g_flow_core_offsets[NUM_CPU_CORES] = {
    {0x008, 0x000, 0x06C},
    {0x018, 0x014, 0x070},
    {0x020, 0x01C, 0x074},
    {0x028, 0x024, 0x078},
};

static inline void flow_set_cc4_ctrl(uint32_t core, uint32_t cc4_ctrl) {
    MAKE_FLOW_REG(g_flow_core_offsets[core].CC4_COREN_CTRL_OFS) = cc4_ctrl;
}

static inline void flow_set_halt_events(uint32_t core, bool halt_events) {
    MAKE_FLOW_REG(g_flow_core_offsets[core].HALT_CPUN_EVENTS_OFS) = (halt_events ? 0x40000F00 : 0x40000000);
}

static inline void flow_set_csr(uint32_t core, uint32_t csr) {
    MAKE_FLOW_REG(g_flow_core_offsets[core].CPUN_CSR_OFS) = (0x100 << core) | (csr << 12) | 0xC001;
}

static inline void flow_clear_csr0_and_events(uint32_t core) {
    MAKE_FLOW_REG(g_flow_core_offsets[core].CPUN_CSR_OFS) = 0;
    MAKE_FLOW_REG(g_flow_core_offsets[core].HALT_CPUN_EVENTS_OFS) = 0;
}

#endif
