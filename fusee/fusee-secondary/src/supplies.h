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
    SUPPLY_MICROSD_LOW_VOLTAGE = 1800000,

};


/**
 * Enables a given power supply.
 *
 * @param supply The power domain on the Switch that is to be enabled.
 * @param use_low_voltage If the supply supports multiple voltages, use the lower one.
 *      Some devices start in a high power mode, but an can be switched to a lower one.
 *      Set this to false unless you know what you're doing.
 */
void supply_enable(enum switch_power_supply supply, bool use_low_voltage);

#endif
