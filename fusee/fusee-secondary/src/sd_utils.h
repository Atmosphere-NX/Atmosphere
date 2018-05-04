#ifndef FUSEE_SD_UTILS_H
#define FUSEE_SD_UTILS_H

#include "utils.h"
#include "sdmmc.h"
#include "ff.h"

void save_sd_state(void **mmc, void **ff);
void resume_sd_state(void *mmc, void *ff);

size_t read_sd_file(void *dst, size_t dst_size, const char *filename);

#endif