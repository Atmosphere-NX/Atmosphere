#include <stdint.h>

#include "utils.h"
#include "smc_api.h"
#include "smc_user.h"
#include "se.h"

uint32_t user_load_aes_key(smc_args_t *args) {
    uint64_t sealed_kek[2];
    uint64_t wrapped_key[2];
    
    uint32_t keyslot = (uint32_t)args->X[1];
    if (keyslot > 3) {
        return 2;
    }
    
    /* Copy keydata */
    sealed_kek[0] = args->X[2];
    sealed_kek[1] = args->X[3];
    wrapped_key[0] = args->X[4];
    wrapped_key[1] = args->X[5];
    
    /* TODO: Unseal the kek. */
    set_aes_keyslot(9, sealed_kek, 0x10);
    
    /* Unwrap the key. */
    decrypt_data_into_keyslot(keyslot, 9, wrapped_key, 0x10);
    return 0;
}
