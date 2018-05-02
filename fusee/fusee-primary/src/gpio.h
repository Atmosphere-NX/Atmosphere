/*
 * Struct defintiions lifted from NVIDIA sample code.
 * (C) Copyright 2013-2015 NVIDIA Corporation <www.nvidia.com>
 *
 * adapted for Fusée by Kate Temkin <k@ktemkin.com.
 */


#ifndef __FUSEE_GPIO_H__
#define __FUSEE_GPIO_H__

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"

enum tegra_gpio_port {
    TEGRA_GPIO_PORT_A = 0,
    TEGRA_GPIO_PORT_B = 1,
    TEGRA_GPIO_PORT_C = 2,
    TEGRA_GPIO_PORT_D = 3,
    TEGRA_GPIO_PORT_E = 4,
    TEGRA_GPIO_PORT_F = 5,
    TEGRA_GPIO_PORT_G = 6,
    TEGRA_GPIO_PORT_H = 7,
    TEGRA_GPIO_PORT_I = 8,
    TEGRA_GPIO_PORT_J = 9,
    TEGRA_GPIO_PORT_K = 10,
    TEGRA_GPIO_PORT_L = 11,
    TEGRA_GPIO_PORT_M = 12,
    TEGRA_GPIO_PORT_N = 13,
    TEGRA_GPIO_PORT_O = 14,
    TEGRA_GPIO_PORT_P = 15,
    TEGRA_GPIO_PORT_Q = 16,
    TEGRA_GPIO_PORT_R = 17,
    TEGRA_GPIO_PORT_S = 18,
    TEGRA_GPIO_PORT_T = 19,
    TEGRA_GPIO_PORT_U = 20,
    TEGRA_GPIO_PORT_V = 21,
    TEGRA_GPIO_PORT_W = 22,
    TEGRA_GPIO_PORT_X = 23,
    TEGRA_GPIO_PORT_Y = 24,
    TEGRA_GPIO_PORT_Z = 25,
    TEGRA_GPIO_PORT_AA = 26,
    TEGRA_GPIO_PORT_BB = 27,
    TEGRA_GPIO_PORT_CC = 28,
    TEGRA_GPIO_PORT_DD = 29,
    TEGRA_GPIO_PORT_EE = 30,
    TEGRA_GPIO_PORT_FF = 31,
};

/**
 * Convenince macro for computing a GPIO port number.
 */
#define TEGRA_GPIO(port, offset) \
    ((TEGRA_GPIO_PORT_##port * 8) + offset)

/*
 * The Tegra210 GPIO controller has 256 GPIOS in 8 banks of 4 ports,
 * each with 8 GPIOs.
 */
enum {
    TEGRA_GPIO_PORTS = 4,   /* number of ports per bank */
    TEGRA_GPIO_BANKS = 8,   /* number of banks */
};

/* GPIO Controller registers for a single bank */
struct tegra_gpio_bank {
    uint32_t config[TEGRA_GPIO_PORTS];
    uint32_t direction[TEGRA_GPIO_PORTS];
    uint32_t out[TEGRA_GPIO_PORTS];
    uint32_t in[TEGRA_GPIO_PORTS];
    uint32_t int_status[TEGRA_GPIO_PORTS];
    uint32_t int_enable[TEGRA_GPIO_PORTS];
    uint32_t int_level[TEGRA_GPIO_PORTS];
    uint32_t int_clear[TEGRA_GPIO_PORTS];
    uint32_t masked_config[TEGRA_GPIO_PORTS];
    uint32_t masked_dir_out[TEGRA_GPIO_PORTS];
    uint32_t masked_out[TEGRA_GPIO_PORTS];
    uint32_t masked_in[TEGRA_GPIO_PORTS];
    uint32_t masked_int_status[TEGRA_GPIO_PORTS];
    uint32_t masked_int_enable[TEGRA_GPIO_PORTS];
    uint32_t masked_int_level[TEGRA_GPIO_PORTS];
    uint32_t masked_int_clear[TEGRA_GPIO_PORTS];
};


/**
 * Representation of Tegra GPIO controllers.
 */
struct tegra_gpio {
    struct tegra_gpio_bank bank[TEGRA_GPIO_BANKS];
};

/**
 * GPIO pins that have a more detailed functional name,
 * specialized for the Switch.
 */
enum tegra_named_gpio {
    GPIO_MICROSD_CARD_DETECT   = TEGRA_GPIO(Z, 1),
    GPIO_MICROSD_WRITE_PROTECT = TEGRA_GPIO(Z, 4),
    GPIO_MICROSD_SUPPLY_ENABLE = TEGRA_GPIO(E, 4),
};


/**
 * Mode select for GPIO or SFIO.
 */
enum tegra_gpio_mode {
    GPIO_MODE_GPIO = 0,
    GPIO_MODE_SFIO = 1
};


/**
 * GPIO direction values
 */
enum tegra_gpio_direction {
    GPIO_DIRECTION_INPUT  = 0,
    GPIO_DIRECTION_OUTPUT = 1
};


/**
 * Active-high GPIO logic
 */
enum tegra_gpio_value {
    GPIO_LEVEL_LOW  = 0,
    GPIO_LEVEL_HIGH = 1
};


/**
 * Utility function that grabs the Tegra pinmux registers.
 */
static inline struct tegra_gpio *gpio_get_regs(void)
{
    return (struct tegra_gpio *)0x6000d000;
}

/**
 * Configures a given pin as either GPIO or SFIO.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param mode The relevant mode.
 */
void gpio_configure_mode(enum tegra_named_gpio pin, enum tegra_gpio_mode mode);


/**
 * Configures a given pin as either INPUT or OUPUT.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param direction The relevant direction.
 */
void gpio_configure_direction(enum tegra_named_gpio pin, enum tegra_gpio_direction dir);


/**
 * Drives a relevant GPIO pin as either HIGH or LOW.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param mode The relevant value.
 */
void gpio_write(enum tegra_named_gpio pin, enum tegra_gpio_value value);

/**
 * Drives a relevant GPIO pin as either HIGH or LOW.
 *
 * @param pin The GPIO pin to work with, as created with TEGRA_GPIO, or a named GPIO.
 * @param mode The relevant mode.
 */
enum tegra_gpio_value gpio_read(enum tegra_named_gpio pin);

#endif
