#ifndef FUSEE_STAGE2_H
#define FUSEE_STAGE2_H

#include "utils.h"

/* TODO: Is there a more concise way to do this? */
#define STAGE2_ARGV_PROGRAM_PATH 0
#define STAGE2_ARGV_ARGUMENT_STRUCT 1
#define STAGE2_ARGC 2

typedef struct {
    uint32_t version;
    const char *bct0;
    uint32_t *lfb;
    uint32_t console_row;
    uint32_t console_col;
    void *sd_mmc;
} stage2_args_t;

#endif
