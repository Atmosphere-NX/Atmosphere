#ifndef EXOSPHERE_USERPAGE_H
#define EXOSPHERE_USERPAGE_H

#include <stdbool.h>
#include <stdint.h>

#define SECURE_USER_PAGE_ADDR (0x1F01F4000ULL)

typedef struct {
    uint64_t user_page;
    uint64_t secure_page;
} upage_ref_t;

bool upage_init(upage_ref_t *user_page, void *user_address);

bool user_copy_to_secure(upage_ref_t *user_page, void *secure_dst, void *user_src, size_t size);
bool secure_copy_to_user(upage_ref_t *user_page, void *user_dst, void *secure_src, size_t size);

static inline uint64_t get_page_for_address(void *address) {
    return ((uint64_t)(address)) & 0xFFFFFFFFFFFFF000ULL;
}

#endif