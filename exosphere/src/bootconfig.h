#ifndef EXOSPHERE_BOOTCONFIG_H
#define EXOSPHERE_BOOTCONFIG_H

#include <stdbool.h>
#include <stdint.h>

/* This provides management for Switch BootConfig. */

typedef struct {
    uint8_t data[0x200];
} bootconfig_unsigned_config_t;

typedef struct {
    uint8_t _0x0[8];
    uint8_t package2_config;
    uint8_t _0x9[7];
    uint8_t hardware_info[0x10];
    uint8_t disable_program_verification;
    uint8_t _0x21[0xDF];
} bootconfig_signed_config_t;

typedef struct {
    bootconfig_unsigned_config_t unsigned_config;
    uint8_t signature[0x100];
    bootconfig_signed_config_t signed_config;
    uint8_t unknown_config[0x240];
} bootconfig_t;

void bootconfig_load_and_verify(const bootconfig_t *bootconfig);
void bootconfig_clear(void);

/* Actual configuration getters. */
bool bootconfig_is_package2_plaintext(void);
bool bootconfig_is_package2_unsigned(void);
bool bootconfig_disable_program_verification(void);
bool bootconfig_is_debug_mode(void);

uint64_t bootconfig_get_memory_arrangement(void);
uint64_t bootconfig_get_kernel_memory_configuration(void);

#endif