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

#ifndef FUSEE_EXOSPHERE_CONFIG_H
#define FUSEE_EXOSPHERE_CONFIG_H

#include <stdint.h>
#include <vapours/ams_version.h>
#include "emummc_cfg.h"

/* This serves to set configuration for *exosphere itself*, separate from the SecMon Exosphere mimics. */

/* "EXO0" */
#define MAGIC_EXOSPHERE_CONFIG (0x304F5845)

#define EXOSPHERE_FLAG_PERFORM_620_KEYGEN                   (1 << 0u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV                    (1 << 1u)
#define EXOSPHERE_FLAG_IS_DEBUGMODE_USER                    (1 << 2u)
#define EXOSPHERE_FLAG_DISABLE_USERMODE_EXCEPTION_HANDLERS  (1 << 3u)
#define EXOSPHERE_FLAG_ENABLE_USERMODE_PMU_ACCESS           (1 << 4u)
#define EXOSPHERE_FLAG_BLANK_PRODINFO                       (1 << 5u)
#define EXOSPHERE_FLAG_ALLOW_WRITING_TO_CAL_SYSMMC          (1 << 6u)

#define EXOSPHERE_LOG_FLAG_INVERTED (1 << 0u)

typedef struct {
    uint32_t magic;
    uint32_t target_firmware;
    uint32_t flags[2];
    uint16_t lcd_vendor;
    uint8_t  log_port;
    uint8_t  log_flags;
    uint32_t log_baud_rate;
    uint32_t reserved1[2];
    exo_emummc_config_t emummc_cfg;
} exosphere_config_t;

_Static_assert(sizeof(exosphere_config_t) == 0x20 + sizeof(exo_emummc_config_t), "exosphere config definition");

#define MAILBOX_EXOSPHERE_CONFIGURATION ((volatile exosphere_config_t *)(0x8000F000ull))

#define EXOSPHERE_TARGETFW_KEY "target_firmware"
#define EXOSPHERE_DEBUGMODE_PRIV_KEY "debugmode"
#define EXOSPHERE_DEBUGMODE_USER_KEY "debugmode_user"
#define EXOSPHERE_DISABLE_USERMODE_EXCEPTION_HANDLERS_KEY "disable_user_exception_handlers"
#define EXOSPHERE_ENABLE_USERMODE_PMU_ACCESS_KEY "enable_user_pmu_access"
#define EXOSPHERE_BLANK_PRODINFO_SYSMMC_KEY "blank_prodinfo_sysmmc"
#define EXOSPHERE_BLANK_PRODINFO_EMUMMC_KEY "blank_prodinfo_emummc"
#define EXOSPHERE_ALLOW_WRITING_TO_CAL_SYSMMC_KEY "allow_writing_to_cal_sysmmc"
#define EXOSPHERE_LOG_PORT_KEY "log_port"
#define EXOSPHERE_LOG_BAUD_RATE_KEY "log_baud_rate"
#define EXOSPHERE_LOG_INVERTED_KEY "log_inverted"

typedef struct {
    int debugmode;
    int debugmode_user;
    int disable_user_exception_handlers;
    int enable_user_pmu_access;
    int blank_prodinfo_sysmmc;
    int blank_prodinfo_emummc;
    int allow_writing_to_cal_sysmmc;
    int log_port;
    int log_baud_rate;
    int log_inverted;
} exosphere_parse_cfg_t;

#endif
