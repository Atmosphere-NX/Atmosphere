#ifndef FUSEE_PACKAGE1_H
#define FUSEE_PACKAGE1_H

#include <stdio.h>
#include "key_derivation.h"

int package1_parse_boot0(void **package1, size_t *package1_size, nx_keyblob_t *keyblobs, uint32_t *revision, FILE *boot0);

#endif
