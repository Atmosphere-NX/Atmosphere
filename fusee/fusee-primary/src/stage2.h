#ifndef FUSEE_STAGE2_H
#define FUSEE_STAGE2_H

#include "utils.h"

/* TODO: Is there a more concise way to do this? */
#define STAGE2_ARGV_VERSION 0
#define STAGE2_ARGV_CONFIG 1
#define STAGE2_ARGV_LFB 2
#define STAGE2_ARGC 3


#define STAGE2_NAME_KEY "stage2_file"
#define STAGE2_ADDRESS_KEY "stage2_addr"
#define STAGE2_ENTRYPOINT_KEY "stage2_entrypoint"

typedef void (*stage2_entrypoint_t)(int argc, void **argv);

typedef struct {
    char filename[0x300];
    uintptr_t load_address;
    stage2_entrypoint_t entrypoint;
} stage2_config_t;

stage2_entrypoint_t load_stage2(const char *bct0);

#endif