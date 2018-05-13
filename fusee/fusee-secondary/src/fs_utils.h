#ifndef FUSEE_FS_UTILS_H
#define FUSEE_FS_UTILS_H

#include "utils.h"
#include "sdmmc.h"

size_t read_from_file(void *dst, size_t dst_size, const char *filename);

#endif
