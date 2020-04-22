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

#ifndef EXOSPHERE_EXOSPHERE_CONFIG_H
#define EXOSPHERE_EXOSPHERE_CONFIG_H

#include <stdint.h>
#include <vapours/ams_version.h>
#include "utils.h"

#include "memory_map.h"
#include "emummc_cfg.h"

/* This serves to set configuration for *exosphere itself*, separate from the SecMon Exosphere mimics. */

/* "EXO0" */
#define MAGIC_EXOSPHERE_CONFIG (0x304F5845)

#define EXOSPHERE_LOOSEN_PACKAGE2_RESTRICTIONS_FOR_DEBUG 1

#define MAILBOX_EXOSPHERE_CONFIG (*((volatile exosphere_config_t *)(0x8000F000ull)))

/* Exosphere config in DRAM shares physical/virtual mapping. */
#define MAILBOX_EXOSPHERE_CONFIG_PHYS MAILBOX_EXOSPHERE_CONFIG

#define EXOSPHERE_FLAG_PERFORM_620_KEYGEN_DEPRECATED        (1 << 0u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV                    (1 << 1u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_USER                    (1 << 2u)
#define EXOSPHERE_FLAG_DISABLE_USERMODE_EXCEPTION_HANDLERS  (1 << 3u)
#define EXOSPHERE_FLAG_ENABLE_USERMODE_PMU_ACCESS           (1 << 4u)
#define EXOSPHERE_FLAG_BLANK_PRODINFO                       (1 << 5u)
#define EXOSPHERE_FLAG_ALLOW_WRITING_TO_CAL_SYSMMC          (1 << 6u)
#define EXOSPHERE_FLAGS_DEFAULT (EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV)

typedef struct {
    uint32_t magic;
    uint32_t target_firmware;
    uint32_t flags;
    uint32_t reserved[5];
    exo_emummc_config_t emummc_cfg;
} exosphere_config_t;

_Static_assert(sizeof(exosphere_config_t) == 0x20 + sizeof(exo_emummc_config_t), "exosphere config definition");

unsigned int exosphere_load_config(void);
unsigned int exosphere_get_target_firmware(void);
unsigned int exosphere_should_perform_620_keygen(void);
unsigned int exosphere_should_override_debugmode_priv(void);
unsigned int exosphere_should_override_debugmode_user(void);
unsigned int exosphere_should_disable_usermode_exception_handlers(void);
unsigned int exosphere_should_enable_usermode_pmu_access(void);
unsigned int exosphere_should_blank_prodinfo(void);
unsigned int exosphere_should_allow_writing_to_cal(void);

const exo_emummc_config_t *exosphere_get_emummc_config(void);

static inline unsigned int exosphere_get_target_firmware_for_init(void) {
    const unsigned int magic = MAILBOX_EXOSPHERE_CONFIG_PHYS.magic;
    if (magic == MAGIC_EXOSPHERE_CONFIG) {
        return MAILBOX_EXOSPHERE_CONFIG_PHYS.target_firmware;
    } else {
        return ATMOSPHERE_TARGET_FIRMWARE_CURRENT;
    }
}

#endif
