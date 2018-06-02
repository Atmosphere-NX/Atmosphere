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

typedef struct {
    uint64_t _0x00;
    uint32_t _0x08;
    uint32_t is_recovery_boot;
    uint32_t boot_reason_low;
    uint32_t boot_reason_high;
} boot_reason_t;

void bootconfig_load_and_verify(const bootconfig_t *bootconfig);
void bootconfig_clear(void);

void bootconfig_load_boot_reason(volatile boot_reason_t *boot_reason);

void bootconfig_set_package2_hash_for_recovery(const void *package2, size_t package2_size);
void bootconfig_get_package2_hash_for_recovery(uint64_t *out_hash);

/* Actual configuration getters. */
bool bootconfig_is_package2_plaintext(void);
bool bootconfig_is_package2_unsigned(void);
void bootconfig_set_package2_plaintext_and_unsigned(void);
bool bootconfig_disable_program_verification(void);
bool bootconfig_is_debug_mode(void);

bool bootconfig_take_extabt_serror_to_el3(void);

uint64_t bootconfig_get_value_for_sysctr0(void);

uint64_t bootconfig_get_memory_arrangement(void);
uint64_t bootconfig_get_kernel_memory_configuration(void);

bool bootconfig_is_recovery_boot(void);
uint64_t bootconfig_get_boot_reason(void);

#endif
