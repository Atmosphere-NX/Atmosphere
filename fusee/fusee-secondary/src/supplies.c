/**
 * Fusée power supply control code
 *  ~ktemkin
 */

#include <stdio.h>
#include "supplies.h"

// FIXME: replace hwinit with our own code
#include "hwinit/max7762x.h"

/**
 * Enables a given power supply.
 *
 * @param supply The power domain on the Switch that is to be enabled.
 */
void supply_enable(enum switch_power_supply supply)
{
    switch(supply) {
        case SUPPLY_MICROSD:
            max77620_regulator_set_voltage(SUPPLY_MICROSD_REGULATOR, SUPPLY_MICROSD_VOLTAGE);
            max77620_regulator_enable(SUPPLY_MICROSD_REGULATOR, true);
            return;

        default:
            printf("ERROR: could not enable unknown supply %d!\n", supply);
            return;
    }
}

