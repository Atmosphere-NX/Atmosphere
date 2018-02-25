#ifndef EXOSPHERE_USERPAGE_H
#define EXOSPHERE_USERPAGE_H

#include "utils.h"
#include "memory_map.h"

#define USER_PAGE_SECURE_MONITOR_ADDR (tzram_get_segment_address(TZRAM_SEGMENT_ID_USERPAGE))

typedef struct {
    uintptr_t user_address;
    uintptr_t secure_monitor_address;
} upage_ref_t;

bool upage_init(upage_ref_t *user_page, void *user_address);

bool user_copy_to_secure(upage_ref_t *user_page, void *secure_dst, void *user_src, size_t size);
bool secure_copy_to_user(upage_ref_t *user_page, void *user_dst, void *secure_src, size_t size);

#endif
