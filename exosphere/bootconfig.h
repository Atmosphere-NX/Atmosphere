#ifndef EXOSPHERE_BOOTCONFIG_H
#define EXOSPHERE_BOOTCONFIG_H

#include <stdint.h>

/* This provides management for Switch BootConfig. */

typedef struct {
    uint8_t unsigned_config[0x200];
    uint8_t signature[0x100];
    uint8_t header[0x100];
    uint8_t signed_config[0x24];
} bootconfig_t;

void bootconfig_load_and_verify(const bootconfig_t *bootconfig)
void bootconfig_clear(void);

#endif