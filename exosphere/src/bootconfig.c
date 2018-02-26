#include "bootconfig.h"

void bootconfig_load_and_verify(const bootconfig_t *bootconfig) {
    /* TODO */
}

void bootconfig_clear(void){
    /* TODO */
}

/* Actual configuration getters. */
bool bootconfig_is_package2_plaintext(void) {
    return false;
    /* TODO */
}

bool bootconfig_is_package2_unsigned(void) {
    return false;
    /* TODO */
}

bool bootconfig_disable_program_verification(void) {
    return false;
    /* TODO */
}

bool bootconfig_is_debug_mode(void) {
    return false;
    /* TODO */
}

uint64_t bootconfig_get_memory_arrangement(void) {
    return 0ull;
    /* TODO */
}

uint64_t bootconfig_get_kernel_memory_configuration(void) {
    return 0ull;
    /* TODO */
}
