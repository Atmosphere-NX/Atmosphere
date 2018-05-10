/**
 * Fusée power supply control code
 *  ~ktemkin
 */

#include "lib/driver_utils.h"
#include "supplies.h"

// FIXME: replace hwinit with our own code
#include "hwinit/max7762x.h"

/**
 * Enables a given power supply.
 *
 * @param supply The power domain on the Switch that is to be enabled.
 * @param use_low_voltage If the supply supports multiple voltages, use the lower one.
 *      Some devices start in a high power mode, but an can be switched to a lower one.
 *      Set this to false unless you know what you're doing.
 */
void supply_enable(enum switch_power_supply supply, bool use_low_voltage)
{
    uint32_t voltage = 0;

    switch(supply) {
        case SUPPLY_MICROSD:
            voltage = use_low_voltage ? SUPPLY_MICROSD_LOW_VOLTAGE : SUPPLY_MICROSD_VOLTAGE;

            max77620_regulator_set_voltage(SUPPLY_MICROSD_REGULATOR, voltage);
            max77620_regulator_enable(SUPPLY_MICROSD_REGULATOR, true);
            return;

        default:
            printk("ERROR: could not enable unknown supply %d!\n", supply);
            return;
    }
}

