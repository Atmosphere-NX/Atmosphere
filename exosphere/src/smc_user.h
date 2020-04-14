/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

uint32_t user_encrypt_rsa_key_for_import(smc_args_t *args);
uint32_t user_decrypt_or_import_rsa_key(smc_args_t *args);


void set_crypt_aes_done(bool done);
bool get_crypt_aes_done(void);

void set_exp_mod_result(uint32_t result);
uint32_t get_exp_mod_result(void);

#endif