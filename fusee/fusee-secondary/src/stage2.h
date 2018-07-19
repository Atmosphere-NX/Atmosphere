#ifndef FUSEE_STAGE2_H
#define FUSEE_STAGE2_H

#include "utils.h"
#include "sdmmc/sdmmc.h"

/* TODO: Is there a more concise way to do this? */
#define STAGE2_ARGV_PROGRAM_PATH 0
#define STAGE2_ARGV_ARGUMENT_STRUCT 1
#define STAGE2_ARGC 2

#define BCTO_MAX_SIZE 0x5800

typedef struct {
    uint32_t version;
    bool display_initialized;
    char bct0[BCTO_MAX_SIZE];
} stage2_args_t;

#endif
