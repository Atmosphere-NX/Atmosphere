#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "userpage.h"

uint64_t g_secure_page_user_address = NULL;

/* Create a user page reference for the desired address. */
/* Returns 1 on success, 0 on failure. */
int upage_init(upage_ref_t *upage, void *user_address) {
    upage->user_page = get_page_for_address(user_address);
    upage->secure_page = 0ULL;
    
    if (g_secure_page_user_address != NULL) {
        /* Different physical address indicate SPL was rebooted, or another process got access to svcCallSecureMonitor. Panic. */
        if (g_secure_page_user_address != upage->user_page) {
            panic();
        }
        upage->secure_page = SECURE_USER_PAGE_ADDR;
    } else {
        /* Weakly validate SPL's physically random address is in DRAM. */
        if (upage->user_page >> 31) {
            g_secure_page_user_address = upage->user_page;
            /* TODO: Map this page into the MMU and invalidate the TLB. */
            upage->secure_page = SECURE_USER_PAGE_ADDR;
        }
    }
    
    return upage->secure_page != 0ULL;
}

int user_copy_to_secure(upage_ref_t *upage, void *secure_dst, void *user_src, size_t size) {    
    /* Fail if the page doesn't match. */
    if (get_page_for_address(user_src) != upage->user_page) {
        return 0;
    }
    
    /* Fail if we go past the page boundary. */
    if (size != 0 && get_page_for_address(user_src + size - 1) != upage->user_page) {
        return 0;
    }
    
    void *secure_src = (void *)(upage->secure_page + ((uint64_t)user_src - upage->user_page));
    memcpy(secure_dst, secure_src, size);
    return 1;
}

int secure_copy_to_user(upage_ref_t *upage, void *user_dst, void *secure_src, size_t size) {    
    /* Fail if the page doesn't match. */
    if (get_page_for_address(user_dst) != upage->user_page) {
        return 0;
    }
    
    /* Fail if we go past the page boundary. */
    if (size != 0 && get_page_for_address(user_dst + size - 1) != upage->user_page) {
        return 0;
    }

    void *secure_dst = (void *)(upage->secure_page + ((uint64_t)user_dst - upage->user_page));
    memcpy(secure_dst, secure_src, size);
    return 1;
}