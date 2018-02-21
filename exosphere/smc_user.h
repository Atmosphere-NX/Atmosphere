#ifndef EXOSPHERE_SMC_USER_H
#define EXOSPHERE_SMC_USER_H

#include "smc_api.h"

uint32_t user_exp_mod(smc_args_t *args);
uint32_t user_get_random_bytes(smc_args_t *args);
uint32_t user_generate_aes_kek(smc_args_t *args);
uint32_t user_load_aes_key(smc_args_t *args);
uint32_t user_crypt_aes(smc_args_t *args);
uint32_t user_generate_specific_aes_key(smc_args_t *args);
uint32_t user_compute_cmac(smc_args_t *args);
uint32_t user_load_rsa_oaep_key(smc_args_t *args);
uint32_t user_decrypt_rsa_private_key(smc_args_t *args);
uint32_t user_load_secure_exp_mod_key(smc_args_t *args);
uint32_t user_secure_exp_mod(smc_args_t *args);
uint32_t user_unwrap_rsa_oaep_wrapped_titlekey(smc_args_t *args);
uint32_t user_load_titlekey(smc_args_t *args);
uint32_t user_unwrap_aes_wrapped_titlekey(smc_args_t *args);


void set_crypt_aes_done(int done);
int get_crypt_aes_done(void);

void set_exp_mod_done(int done);
int get_exp_mod_done(void);

#endif