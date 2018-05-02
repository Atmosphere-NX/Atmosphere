/**
 * Fusée power supply control code
 *  ~ktemkin
 */

#ifndef __FUSEE_SUPPLIES_H__
#define __FUSEE_SUPPLIES_H__

#include "utils.h"

enum switch_power_supply {
    SUPPLY_MICROSD,
};


enum switch_power_constants {

    /* MicroSD card */
    SUPPLY_MICROSD_REGULATOR = 6,
    SUPPLY_MICROSD_VOLTAGE = 3300000,

};

/**
 * Enables a given power supply.
 *
 * @param supply The power domain on the Switch that is to be enabled.
 */
void supply_enable(enum switch_power_supply supply);

#endif
