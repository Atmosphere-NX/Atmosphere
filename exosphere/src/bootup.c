#include <stdint.h>

#include "utils.h"
#include "bootup.h"

void bootup_misc_mmio(void) {
    /* TODO */
    /* This func will also be called on warmboot. */
    /* And will verify stored SE Test Vector, clear keyslots, */
    /* Generate an SRK, set the warmboot firmware location, */
    /* Configure the GPU uCode carveout, configure the Kernel default carveouts, */
    /* Initialize the PMC secure scratch registers, initialize MISC registers, */
    /* And assign "se_operation_completed" to Interrupt 0x5A. */
}

void setup_4x_mmio(void) {
    /* TODO */
}