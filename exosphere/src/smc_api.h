/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#ifndef EXOSPHERE_SMC_API_H
#define EXOSPHERE_SMC_API_H

#include <stdint.h>

#define SMC_HANDLER_USER 0
#define SMC_HANDLER_PRIV 1

typedef struct {
    uint64_t X[8];
} smc_args_t;

bool try_set_user_smc_in_progress(void);
void set_user_smc_in_progress(void);
void clear_user_smc_in_progress(void);

void set_priv_smc_in_progress(void);
void clear_priv_smc_in_progress(void);

uintptr_t get_smc_core012_stack_address(void);
uintptr_t get_exception_entry_stack_address(unsigned int core_id);

void set_version_specific_smcs(void);


void set_suspend_for_debug(void);

void call_smc_handler(unsigned int handler_id, smc_args_t *args);

#endif
