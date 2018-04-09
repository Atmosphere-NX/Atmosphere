#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include "types.h"

/*! Flow controller registers. */
#define FLOW_CTLR_RAM_REPAIR 0x40
#define FLOW_CTLR_BPMP_CLUSTER_CONTROL 0x98

void cluster_enable_cpu0(u64 entry, u32 ns_disable);

#endif
