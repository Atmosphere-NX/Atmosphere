#include <stdint.h>

#include "utils.h"
#include "configitem.h"
#include "cpu_context.h"
#include "masterkey.h"
#include "mc.h"
#include "mmu.h"
#include "pmc.h"
#include "randomcache.h"
#include "sealedkeys.h"
#include "smc_api.h"
#include "smc_user.h"
#include "se.h"
#include "userpage.h"
#include "titlekey.h"

#define SMC_USER_HANDLERS 0x13
#define SMC_PRIV_HANDLERS 0x9

/* User SMC prototypes */
uint32_t smc_set_config(smc_args_t *args);
uint32_t smc_get_config(smc_args_t *args);
uint32_t smc_check_status(smc_args_t *args);
uint32_t smc_get_result(smc_args_t *args);
uint32_t smc_exp_mod(smc_args_t *args);
uint32_t smc_get_random_bytes_for_user(smc_args_t *args);
uint32_t smc_generate_aes_kek(smc_args_t *args);
uint32_t smc_load_aes_key(smc_args_t *args);
uint32_t smc_crypt_aes(smc_args_t *args);
uint32_t smc_generate_specific_aes_key(smc_args_t *args);
uint32_t smc_compute_cmac(smc_args_t *args);
uint32_t smc_load_rsa_oaep_key(smc_args_t *args);
uint32_t smc_decrypt_rsa_private_key(smc_args_t *args);
uint32_t smc_load_secure_exp_mod_key(smc_args_t *args);
uint32_t smc_secure_exp_mod(smc_args_t *args);
uint32_t smc_unwrap_rsa_oaep_wrapped_titlekey(smc_args_t *args);
uint32_t smc_load_titlekey(smc_args_t *args);
uint32_t smc_unwrap_aes_wrapped_titlekey(smc_args_t *args);

/* Privileged SMC prototypes */
uint32_t smc_cpu_suspend(smc_args_t *args);
uint32_t smc_cpu_off(smc_args_t *args);
uint32_t smc_cpu_on(smc_args_t *args);
/* uint32_t smc_get_config(smc_args_t *args); */
uint32_t smc_get_random_bytes_for_priv(smc_args_t *args);
uint32_t smc_panic(smc_args_t *args);
uint32_t smc_configure_carveout(smc_args_t *args);
uint32_t smc_read_write_register(smc_args_t *args);

typedef struct {
    uint32_t id;
    uint32_t (*handler)(smc_args_t *args);
} smc_table_entry_t;

typedef struct {
    smc_table_entry_t *handlers;
    uint32_t num_handlers;
} smc_table_t;

smc_table_entry_t g_smc_user_table[SMC_USER_HANDLERS] = {
    {0, NULL},
    {0xC3000401, smc_set_config},
    {0xC3000002, smc_get_config},
    {0xC3000003, smc_check_status},
    {0xC3000404, smc_get_result},
    {0xC3000E05, smc_exp_mod},
    {0xC3000006, smc_get_random_bytes_for_user},
    {0xC3000007, smc_generate_aes_kek},
    {0xC3000008, smc_load_aes_key},
    {0xC3000009, smc_crypt_aes},
    {0xC300000A, smc_generate_specific_aes_key},
    {0xC300040B, smc_compute_cmac},
    {0xC300100C, smc_load_rsa_oaep_key},
    {0xC300100D, smc_decrypt_rsa_private_key},
    {0xC300100E, smc_load_secure_exp_mod_key},
    {0xC300060F, smc_secure_exp_mod},
    {0xC3000610, smc_unwrap_rsa_oaep_wrapped_titlekey},
    {0xC3000011, smc_load_titlekey},
    {0xC3000012, smc_unwrap_aes_wrapped_titlekey}
};

smc_table_entry_t g_smc_priv_table[SMC_PRIV_HANDLERS] = {
    {0, NULL},
    {0xC4000001, smc_cpu_suspend},
    {0x84000002, smc_cpu_off},
    {0xC4000003, smc_cpu_on},
    {0xC3000004, smc_get_config}, /* NOTE: Same function as for USER */
    {0xC3000005, smc_get_random_bytes_for_priv},
    {0xC3000006, smc_panic},
    {0xC3000007, smc_configure_carveout},
    {0xC3000008, smc_read_write_register}
};

smc_table_t g_smc_tables[2] = {
    { /* SMC_HANDLER_USER */
        g_smc_user_table,
        SMC_USER_HANDLERS
    },
    { /* SMC_HANDLER_PRIV */
        g_smc_priv_table,
        SMC_PRIV_HANDLERS
    }
};

bool g_is_smc_in_progress = false;
uint32_t (*g_smc_callback)(void *, uint64_t) = NULL;
uint64_t g_smc_callback_key = 0;

uint64_t try_set_smc_callback(uint32_t (*callback)(void *, uint64_t)) {
    uint64_t key;
    /* TODO: Atomics... */
    if (g_smc_callback_key) {
        return 0;
    }
    
    se_generate_random(KEYSLOT_SWITCH_RNGKEY, &key, sizeof(uint64_t));
    g_smc_callback_key = key;
    g_smc_callback = callback;
    return key;
}

void clear_smc_callback(uint64_t key) {
    /* TODO: Atomics... */
    if (g_smc_callback_key == key) {
        g_smc_callback_key = 0;
    }
}

void call_smc_handler(uint32_t handler_id, smc_args_t *args) {
    unsigned char smc_id;
    unsigned int result;
    unsigned int (*smc_handler)(smc_args_t *args);
    
    /* Validate top-level handler. */
    if (handler_id != SMC_HANDLER_USER && handler_id != SMC_HANDLER_PRIV) {
        panic();
    }
    
    /* Validate core is appropriate for handler. */
    if (handler_id == SMC_HANDLER_USER && get_core_id() != 3) {
        /* USER SMCs must be called via svcCallSecureMonitor on core 3 (where spl runs) */
        panic();
    }
    
    /* Validate sub-handler index */
    if ((smc_id = (unsigned char)args->X[0]) >= g_smc_tables[handler_id].num_handlers) {
        panic();
    }
    
    /* Validate sub-handler */
    if (g_smc_tables[handler_id].handlers[smc_id].id != args->X[0]) {
        panic();
    }
    
    /* Validate handler. */
    if ((smc_handler = g_smc_tables[handler_id].handlers[smc_id].handler) == NULL) {
        panic();
    }
    
    /* Call function. */
    args->X[0] = smc_handler(args);
}

uint32_t smc_wrapper_sync(smc_args_t *args, uint32_t (*handler)(smc_args_t *)) {
    uint32_t result;
    /* TODO: Make g_is_smc_in_progress atomic. */
    if (g_is_smc_in_progress) {
        return 3;
    }
    g_is_smc_in_progress = true;
    result = handler(args);
    g_is_smc_in_progress = false;
    return result;
}

uint32_t smc_wrapper_async(smc_args_t *args, uint32_t (*handler)(smc_args_t *), uint32_t (*callback)(void *, uint64_t)) {
    uint32_t result;
    uint64_t key;
    /* TODO: Make g_is_smc_in_progress atomic. */
    if (g_is_smc_in_progress) {
        return 3;
    }
    g_is_smc_in_progress = 1;
    if ((key = try_set_smc_callback(callback)) != 0) {
        result = handler(args);
        if (result == 0) {
            /* Pass the status check key back to userland. */
            args->X[1] = key;
            /* Early return, leaving g_is_smc_in_progress == 1 */
            return result;
        } else {
            /* No status to check. */
            clear_smc_callback(key);
        }
    } else {
        /* smcCheckStatus needs to be called. */
        result = 3;
    }
    g_is_smc_in_progress = false;
    return result;
}

uint32_t smc_set_config(smc_args_t *args) {
    /* Actual value presumed in X3 on hardware. */
    return configitem_set((enum ConfigItem)args->X[1], args->X[3]);
}

uint32_t smc_get_config(smc_args_t *args) {
    uint64_t out_item = 0;
    uint32_t result;
    result = configitem_get((enum ConfigItem)args->X[1], &out_item);
    args->X[1] = out_item;
    return result;
}

uint32_t smc_check_status(smc_args_t *args) {
    if (g_smc_callback_key == 0) {
        return 4;
    }
    
    if (args->X[1] != g_smc_callback_key) {
        return 5;
    }
    
    args->X[1] = g_smc_callback(NULL, 0);
    
    g_smc_callback_key = 0;
    return 0;
}

uint32_t smc_get_result(smc_args_t *args) {
    uint32_t status;
    unsigned char result_buf[0x400];
    upage_ref_t page_ref;
    
    void *user_address = (void *)args->X[2];
    
    if (g_smc_callback_key == 0) {
        return 4;
    }
    
    if (args->X[1] != g_smc_callback_key) {
        return 5;
    }
    
    /* Check result size */
    if (args->X[3] > 0x400) {
        return 2;
    }
    
    args->X[1] = g_smc_callback(result_buf, args->X[3]);
    g_smc_callback_key = 0;
    
    /* Initialize page reference. */
    if (upage_init(&page_ref, user_address) == 0) {
        return 2;
    }

    /* Copy result output back to user. */
    if (secure_copy_to_user(&page_ref, user_address, result_buf, (size_t)args->X[3]) == 0) {
        return 2;
    }
    
    return 0;
}

uint32_t smc_exp_mod_get_result(void *buf, uint64_t size) {
    if (get_exp_mod_done() != 1) {
        return 3;
    }
    
    if (size != 0x100) {
        return 2;
    }
    
    se_get_exp_mod_output(buf, 0x100);
    
    /* smc_exp_mod is done now. */
    g_is_smc_in_progress = false;
    return 0;
}

uint32_t smc_exp_mod(smc_args_t *args) {
    return smc_wrapper_async(args, user_exp_mod, smc_exp_mod_get_result);
}

uint32_t smc_get_random_bytes_for_user(smc_args_t *args) {
    return smc_wrapper_sync(args, user_get_random_bytes);
}

uint32_t smc_generate_aes_kek(smc_args_t *args) {
    return smc_wrapper_sync(args, user_generate_aes_kek);
}

uint32_t smc_load_aes_key(smc_args_t *args) {
    return smc_wrapper_sync(args, user_load_aes_key);
}

uint32_t smc_crypt_aes_status_check(void *buf, uint64_t size) {
    /* Buf and size are unused. */
    if (get_crypt_aes_done() != 1) {
        return 3;
    }
    /* smc_crypt_aes is done now. */
    g_is_smc_in_progress = false;
    return 0;
}

uint32_t smc_crypt_aes(smc_args_t *args) {
    return smc_wrapper_async(args, user_crypt_aes, smc_crypt_aes_status_check);
}

uint32_t smc_generate_specific_aes_key(smc_args_t *args) {
    return smc_wrapper_sync(args, user_generate_specific_aes_key);
}

uint32_t smc_compute_cmac(smc_args_t *args) {
    return smc_wrapper_sync(args, user_compute_cmac);
}

uint32_t smc_load_rsa_oaep_key(smc_args_t *args) {
    return smc_wrapper_sync(args, user_load_rsa_oaep_key);
}

uint32_t smc_decrypt_rsa_private_key(smc_args_t *args) {
    return smc_wrapper_sync(args, user_decrypt_rsa_private_key);
}

uint32_t smc_load_secure_exp_mod_key(smc_args_t *args) {
    return smc_wrapper_sync(args, user_load_secure_exp_mod_key);
}

uint32_t smc_secure_exp_mod(smc_args_t *args) {
    return smc_wrapper_async(args, user_secure_exp_mod, smc_exp_mod_get_result);
}

uint32_t smc_unwrap_rsa_oaep_wrapped_titlekey_get_result(void *buf, uint64_t size) {
    uint64_t *p_sealed_key = (uint64_t *)buf;
    uint8_t rsa_wrapped_titlekey[0x100];
    uint8_t aes_wrapped_titlekey[0x10];
    uint8_t titlekey[0x10];
    uint64_t sealed_titlekey[2];
    if (get_exp_mod_done() != 1) {
        return 3;
    }
    
    if (size != 0x10) {
        return 2;
    }
    
    se_get_exp_mod_output(rsa_wrapped_titlekey, 0x100);
    if (tkey_rsa_oaep_unwrap(aes_wrapped_titlekey, 0x10, rsa_wrapped_titlekey, 0x100) != 0x10) {
        /* Failed to extract RSA OAEP wrapped key. */
        g_is_smc_in_progress = false;
        return 2;
    }
    
    tkey_aes_unwrap(titlekey, 0x10, aes_wrapped_titlekey, 0x10);
    seal_titlekey(sealed_titlekey, 0x10, titlekey, 0x10);
    
    p_sealed_key[0] = sealed_titlekey[0];
    p_sealed_key[1] = sealed_titlekey[1];
    
    /* smc_unwrap_rsa_oaep_wrapped_titlekey is done now. */
    g_is_smc_in_progress = false;
    return 0;
}

uint32_t smc_unwrap_rsa_oaep_wrapped_titlekey(smc_args_t *args) {
    return smc_wrapper_async(args, user_unwrap_rsa_oaep_wrapped_titlekey, smc_unwrap_rsa_oaep_wrapped_titlekey_get_result);
}

uint32_t smc_load_titlekey(smc_args_t *args) {
    return smc_wrapper_sync(args, user_load_titlekey);
}

uint32_t smc_unwrap_aes_wrapped_titlekey(smc_args_t *args) {
    return smc_wrapper_sync(args, user_unwrap_aes_wrapped_titlekey);
}

uint32_t smc_cpu_on(smc_args_t *args) {
    return cpu_on((uint32_t)args->X[1], args->X[2], args->X[3]);
}

uint32_t smc_cpu_off(smc_args_t *args) {
    return cpu_off();
}

/* Wrapper for cpu_suspend */
uint32_t cpu_suspend_wrapper(smc_args_t *args) {
    return cpu_suspend(args->X[1], args->X[2], args->X[3]);
}

uint32_t smc_cpu_suspend(smc_args_t *args) {
    return smc_wrapper_sync(args, cpu_suspend_wrapper);
}

uint32_t smc_get_random_bytes_for_priv(smc_args_t *args) {
    /* This is an interesting SMC. */
    /* The kernel must NEVER be unable to get random bytes, if it needs them */
    /* As such: */

    uint32_t result;

    /* TODO: Make atomic. */
    if (g_is_smc_in_progress) {
        if (args->X[1] > 0x38) {
            return 2;
        }
        /* Retrieve bytes from the cache. */
        size_t num_bytes = (size_t)args->X[1];
        randomcache_getbytes(&args->X[1], num_bytes);
        result = 0;
    } else {
        g_is_smc_in_progress = true;
        /* If the kernel isn't denied service by a usermode SMC, generate fresh random bytes. */
        result = user_get_random_bytes(args);
        /* Also, refill our cache while we have the chance in case we get denied later. */
        randomcache_refill();
        g_is_smc_in_progress = false;
    }
    return result;
}

uint32_t smc_read_write_register(smc_args_t *args) {
    uint64_t address = args->X[1];
    uint32_t mask = (uint32_t)(args->X[2]);
    uint32_t value = (uint32_t)(args->X[3]);
    volatile uint32_t *p_mmio = NULL;
    /* Address must be aligned. */
    if (address & 3) {
        return 2;
    }
    /* Check for PMC registers. */
    if (0x7000E400ULL <= address && address <= 0x7000EFFFULL) {
        const uint8_t pmc_whitelist[0x28] = {0xB9, 0xF9, 0x07, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00, 0x00, 0x17, 0x00, 0xC4, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00};
        /* Offset = Address - PMC_BASE */
        uint32_t offset = (uint32_t)(address - 0x7000E400ULL);
        uint32_t wl_ind = (offset >> 5);
        /* If address is whitelisted, allow write. */
        if (wl_ind < sizeof(pmc_whitelist) && (pmc_whitelist[wl_ind] & (1 << ((offset >> 2) & 0x7)))) {
            p_mmio = (volatile uint32_t *)(PMC_BASE + offset);
        } else {
            return 2;
        }
    } else if (mkey_get_revision() >= MASTERKEY_REVISION_400_CURRENT && devices[MMIO_DEVID_MC].pa <= address && address < devices[MMIO_DEVID_MC].pa + devices[MMIO_DEVID_MC].size) {
        /* Memory Controller RW supported only on 4.0.0+ */
        const uint8_t mc_whitelist[0x68] = {0x9F, 0x31, 0x30, 0x00, 0xF0, 0xFF, 0xF7, 0x01, 0xCD, 0xFE, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x03, 0x40, 0x73, 0x3E, 0x2F, 0x00, 0x00, 0x6E, 0x30, 0x05, 0x06, 0xB0, 0x71, 0xC8, 0x43, 0x04, 0x80, 0x1F, 0x08, 0x80, 0x03, 0x00, 0x0E, 0x00, 0x08, 0x00, 0xE0, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0xF0, 0x03, 0x03, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x40, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0xE4, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0xFE, 0x0F, 0x01, 0x00, 0x80, 0x00, 0x00, 0x08, 0x00, 0x00};
        uint32_t offset = (uint32_t)(address - 0x70019000ULL);
        uint32_t wl_ind = (offset >> 5);
        /* If address is whitelisted, allow write. */
        if (wl_ind < sizeof(mc_whitelist) && (mc_whitelist[wl_ind] & (1 << ((offset >> 2) & 0x7)))) {
            p_mmio = (volatile uint32_t *)(mmio_get_device_address(MMIO_DEVID_MC) + offset);
        } else {
            /* These addresses are not allowed by the whitelist. */
            /* They correspond to SMMU DISABLE for the BPMP, and for APB-DMA. */
            /* However, smcReadWriteRegister returns 0 for these addresses despite not actually performing the write. */
            /* This is "probably" to fuck with hackers who got access to smcReadWriteRegister and are trying to get */
            /* control of the BPMP for jamais vu etc., since there's no other reason to return 0 despite failure. */
            if (address == 0x7001923C || address == 0x70019298) {
                return 0;
            }
            return 2;
        }
    }
    
    /* Perform actual write. */
    if (p_mmio != NULL) {
        uint32_t old_value;
        /* Write whole value. */
        if (mask == 0xFFFFFFFF) {
            old_value = 0;
        } else {
            old_value = *p_mmio;
        }
        if (mask) {
            *p_mmio = (old_value & ~mask) | (value & mask);
        }
        /* Return old value. */
        args->X[1] = old_value;
        return 0;
    }
    
    return 2;
}


uint32_t smc_configure_carveout(smc_args_t *args) {
    if (args->X[0] > 1) {
        return 2;
    }
    
    unsigned int carveout_id = (unsigned int)args->X[1];
    uint64_t address = args->X[2];
    uint64_t size = args->X[3];
    
    /* Ensure carveout isn't too big. */
    if (size > KERNEL_CARVEOUT_SIZE_MAX) {
        return 2;
    }
    
    /* Configuration is one-shot, and cannot be done multiple times. */
    static bool configured_carveouts[2] = {false, false};
    if (configured_carveouts[carveout_id]) {
        return 2;
    }
    
    configure_kernel_carveout(carveout_id + 4, address, size);
    configured_carveouts[carveout_id] = true;
    return 0;
}