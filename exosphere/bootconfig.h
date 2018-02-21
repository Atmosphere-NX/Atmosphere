#ifndef EXOSPHERE_BOOTCONFIG_H
#define EXOSPHERE_BOOTCONFIG_H

#include <stdint.h>

/* This provides management for Switch BootConfig. */

typedef struct {
    uint8_t unsigned_config[0x200];
    uint8_t signature[0x100];
    uint8_t signed_config[0x100];
    uint8_t unknown_config[0x240];
} bootconfig_t;

void bootconfig_load_and_verify(const bootconfig_t *bootconfig);
void bootconfig_clear(void);


/* Actual configuration getters. */
int bootconfig_is_package2_plaintext(void);
void bootconfig_is_package2_unsigned(void);

#endif