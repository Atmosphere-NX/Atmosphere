#ifndef EXOSPHERE_BOOTCONFIG_H
#define EXOSPHERE_BOOTCONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "memory_map.h"

/* This provides management for Switch BootConfig. */

#define LOADED_BOOTCONFIG (get_loaded_bootconfig())

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
    uint8_t unused_space[0x240]; /* remaining space in the evt page */
} bootconfig_t;

static inline bootconfig_t *get_loaded_bootconfig(void) {
    /* this is also get_exception_entry_stack_address(2) */
    return (bootconfig_t *)(TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x180);
}

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
