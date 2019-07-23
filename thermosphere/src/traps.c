#include "traps.h"
#include "sysreg.h"

void enableTraps(void)
{
    u64 hcr = GET_SYSREG(hcr_el2);

    // Trap *writes* to memory control registers
    hcr |= HCR_TVM;

    // Trap SMC instructions
    hcr |= HCR_TSC;

    // Reroute physical IRQ to EL2
    hcr |= HCR_IMO;

    // TODO debug exceptions

    SET_SYSREG(hcr_el2, hcr);
}