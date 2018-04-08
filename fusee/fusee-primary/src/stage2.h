#ifndef FUSEE_STAGE2_H
#define FUSEE_STAGE2_H

#include "utils.h"

/* TODO: Is there a more concise way to do this? */
#define STAGE2_ARGV_PROGRAM_PATH 0
#define STAGE2_ARGV_ARGUMENT_STRUCT 1
#define STAGE2_ARGC 2

#define STAGE2_NAME_KEY "stage2_path"
#define STAGE2_ADDRESS_KEY "stage2_addr"
#define STAGE2_ENTRYPOINT_KEY "stage2_entrypoint"

typedef void (*stage2_entrypoint_t)(int argc, void **argv);

typedef struct {
    char path[0x300];
    uintptr_t load_address;
    stage2_entrypoint_t entrypoint;
} stage2_config_t;

typedef struct {
    uint32_t version;
    const char *bct0;
    uint32_t *lfb;
    uint32_t console_row;
    uint32_t console_col;
} stage2_args_t;

const char *stage2_get_program_path(void);
stage2_entrypoint_t load_stage2(const char *bct0);

#endif