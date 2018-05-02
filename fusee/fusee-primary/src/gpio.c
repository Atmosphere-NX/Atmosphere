#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "gpio.h"
#include "lib/printk.h"

enum tegra_gpio_shifts {
    GPIO_BANK_SHIFT = 5,  
    GPIO_PORT_SHIFT = 3,
};

enum tegra_gpio_masks {
    GPIO_PORT_MASK = 0x3,
    GPIO_PIN_MASK  = 0x7,
};

/**
 * Returns a GPIO bank object that corresponds to the given GPIO pin,
 * which can be created using the TEGRA_GPIO macro or passed from the name macro.
 *
 * @param pin The GPIO to get the bank for.
 * @return The GPIO bank object to use for working with the given bank.
 */
static volatile struct tegra_gpio_bank *gpio_get_bank(enum tegra_named_gpio pin)
{
    volatile struct tegra_gpio *gpio = gpio_get_regs();
    int bank_number = pin >> GPIO_BANK_SHIFT;

    return &gpio->bank[bank_number];
}


/**
 * @return the port number for working with the given GPIO.
 */
static volatile int gpio_get_port(enum tegra_named_gpio pin)
{
    return (pin >> GPIO_PORT_SHIFT) & GPIO_PORT_MASK;
}


/**
 * @return a mask to be used to work with the given GPIO
 */
static volatile uint32_t gpio_get_mask(enum tegra_named_gpio pin)
{
    uint32_t pin_number = pin & GPIO_PIN_MASK;
    return (1 << pin_number);
}



/**
 * Performs a simple GPIO configuration operation.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param should_be_set True iff the relevant bit should be set; or false if it should be cleared.
 * @param offset The offset into a gpio_bank structure 
 */
static void gpio_simple_register_set(enum tegra_named_gpio pin, bool should_be_set, size_t offset)
{
    // Retrieve the register set that corresponds to the given pin and offset.
    uintptr_t cluster_addr = (uintptr_t)gpio_get_bank(pin) + offset;
    uint32_t *cluster = (uint32_t *)cluster_addr;

    // Figure out the offset into the cluster,
    // and the mask to be used.
    int port      = gpio_get_port(pin);
    uint32_t mask = gpio_get_mask(pin);


    // Set or clear the bit, as appropriate.
    if (should_be_set)
        cluster[port] |=  mask;
    else
        cluster[port] &= ~mask;
}


/**
 * Performs a simple GPIO configuration operation.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param should_be_set True iff the relevant bit should be set; or false if it should be cleared.
 * @param offset The offset into a gpio_bank structure 
 */
static bool gpio_simple_register_get(enum tegra_named_gpio pin, size_t offset)
{
    // Retrieve the register set that corresponds to the given pin and offset.
    uintptr_t cluster_addr = (uintptr_t)gpio_get_bank(pin) + offset;
    uint32_t *cluster = (uint32_t *)cluster_addr;

    // Figure out the offset into the cluster,
    // and the mask to be used.
    int port      = gpio_get_port(pin);
    uint32_t mask = gpio_get_mask(pin);

    // Convert the given value to a boolean.
    return !!(cluster[port] & mask);
}


/**
 * Configures a given pin as either GPIO or SFIO.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param mode The relevant mode.
 */
void gpio_configure_mode(enum tegra_named_gpio pin, enum tegra_gpio_mode mode)
{
    gpio_simple_register_set(pin, mode == GPIO_MODE_GPIO, offsetof(struct tegra_gpio_bank, config));
}


/**
 * Configures a given pin as either INPUT or OUPUT.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param direction The relevant direction.
 */
void gpio_configure_direction(enum tegra_named_gpio pin, enum tegra_gpio_direction dir)
{
    gpio_simple_register_set(pin, dir == GPIO_DIRECTION_OUTPUT, offsetof(struct tegra_gpio_bank, direction)); 
}


/**
 * Drives a relevant GPIO pin as either HIGH or LOW.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param mode The relevant mode.
 */
void gpio_write(enum tegra_named_gpio pin, enum tegra_gpio_value value)
{
    gpio_simple_register_set(pin, value == GPIO_LEVEL_HIGH, offsetof(struct tegra_gpio_bank, out)); 
}


/**
 * Drives a relevant GPIO pin as either HIGH or LOW.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param mode The relevant mode.
 */
enum tegra_gpio_value gpio_read(enum tegra_named_gpio pin)
{
    return gpio_simple_register_get(pin, offsetof(struct tegra_gpio_bank, in)); 
}
