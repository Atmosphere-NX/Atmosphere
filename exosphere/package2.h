#ifndef EXOSPHERE_PACKAGE2_H
#define EXOSPHERE_PACKAGE2_H

/* This is code responsible for validating a package2. Actual file reading is done by bootloader. */

#include <stdint.h>
#include "bootconfig.h"

/* Physaddr 0x40002EF8 */
#define MAILBOX_NX_BOOTLOADER_SETUP_STATE (*((volatile uint32_t *)(0x1F009FEF8ULL)))

#define NX_BOOTLOADER_STATE_INIT 0
#define NX_BOOTLOADER_STATE_MOVED_BOOTCONFIG 1
#define NX_BOOTLOADER_STATE_LOADED_PACKAGE2 2
#define NX_BOOTLOADER_STATE_FINISHED 3 

/* Physaddr 0x40002EFC */
#define MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE (*((volatile uint32_t *)(0x1F009FEFCULL)))

#define NX_BOOTLOADER_BOOTCONFIG_POINTER ((void *)(0x4003D000ULL));

void load_package2(void);

#endif