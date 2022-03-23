/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2019 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#include "../utils/types.h"

#define GPIO_MODE_SPIO 0
#define GPIO_MODE_GPIO 1

#define GPIO_OUTPUT_DISABLE 0
#define GPIO_OUTPUT_ENABLE 1

#define GPIO_IRQ_DISABLE 0
#define GPIO_IRQ_ENABLE 1

#define GPIO_LOW 0
#define GPIO_HIGH 1
#define GPIO_FALLING 0
#define GPIO_RISING 1

#define GPIO_LEVEL 0
#define GPIO_EDGE 1

#define GPIO_CONFIGURED_EDGE 0
#define GPIO_ANY_EDGE_CHANGE 1

/*! GPIO pins (0-7 for each port). */
#define GPIO_PIN_0 (1 << 0)
#define GPIO_PIN_1 (1 << 1)
#define GPIO_PIN_2 (1 << 2)
#define GPIO_PIN_3 (1 << 3)
#define GPIO_PIN_4 (1 << 4)
#define GPIO_PIN_5 (1 << 5)
#define GPIO_PIN_6 (1 << 6)
#define GPIO_PIN_7 (1 << 7)

/*! GPIO ports (A-EE). */
#define GPIO_PORT_A 0
#define GPIO_PORT_B 1
#define GPIO_PORT_C 2
#define GPIO_PORT_D 3
#define GPIO_PORT_E 4
#define GPIO_PORT_F 5
#define GPIO_PORT_G 6
#define GPIO_PORT_H 7
#define GPIO_PORT_I 8
#define GPIO_PORT_J 9
#define GPIO_PORT_K 10
#define GPIO_PORT_L 11
#define GPIO_PORT_M 12
#define GPIO_PORT_N 13
#define GPIO_PORT_O 14
#define GPIO_PORT_P 15
#define GPIO_PORT_Q 16
#define GPIO_PORT_R 17
#define GPIO_PORT_S 18
#define GPIO_PORT_T 19
#define GPIO_PORT_U 20
#define GPIO_PORT_V 21
#define GPIO_PORT_W 22
#define GPIO_PORT_X 23
#define GPIO_PORT_Y 24
#define GPIO_PORT_Z 25
#define GPIO_PORT_AA 26
#define GPIO_PORT_BB 27
#define GPIO_PORT_CC 28
#define GPIO_PORT_DD 29
#define GPIO_PORT_EE 30

void gpio_config(u32 port, u32 pins, int mode);
void gpio_output_enable(u32 port, u32 pins, int enable);
void gpio_write(u32 port, u32 pins, int high);
int  gpio_read(u32 port, u32 pins);
int  gpio_interrupt_status(u32 port, u32 pins);
void gpio_interrupt_enable(u32 port, u32 pins, int enable);
void gpio_interrupt_level(u32 port, u32 pins, int high, int edge, int delta);
u32  gpio_get_bank_irq_id(u32 port);

#endif
