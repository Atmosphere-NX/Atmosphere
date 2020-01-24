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

#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "exocfg.h"
#include "mmu.h"
#include "memory_map.h"

static exosphere_config_t g_exosphere_cfg = {MAGIC_EXOSPHERE_CONFIG, ATMOSPHERE_TARGET_FIRMWARE_CURRENT, EXOSPHERE_FLAGS_DEFAULT};
static bool g_has_loaded_config = false;

#define EXOSPHERE_CHECK_FLAG(flag) ((g_exosphere_cfg.flags & flag) != 0)


/* Read config out of IRAM, return target firmware version. */
unsigned int exosphere_load_config(void) {
    if (g_has_loaded_config) {
        generic_panic();
    }
    g_has_loaded_config = true;

    const unsigned int magic = MAILBOX_EXOSPHERE_CONFIG.magic;

    if (magic == MAGIC_EXOSPHERE_CONFIG) {
        g_exosphere_cfg = MAILBOX_EXOSPHERE_CONFIG;
    }

    return g_exosphere_cfg.target_firmware;
}

unsigned int exosphere_get_target_firmware(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }

    return g_exosphere_cfg.target_firmware;
}

unsigned int exosphere_should_perform_620_keygen(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }

    return false;
}

unsigned int exosphere_should_override_debugmode_priv(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }

    return EXOSPHERE_CHECK_FLAG(EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV);
}

unsigned int exosphere_should_override_debugmode_user(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }

    return EXOSPHERE_CHECK_FLAG(EXOSPHERE_FLAG_IS_DEBUGMODE_USER);
}

unsigned int exosphere_should_disable_usermode_exception_handlers(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }

    return EXOSPHERE_CHECK_FLAG(EXOSPHERE_FLAG_DISABLE_USERMODE_EXCEPTION_HANDLERS);
}

unsigned int exosphere_should_enable_usermode_pmu_access(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }

    return EXOSPHERE_CHECK_FLAG(EXOSPHERE_FLAG_ENABLE_USERMODE_PMU_ACCESS);
}

const exo_emummc_config_t *exosphere_get_emummc_config(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }

    return &g_exosphere_cfg.emummc_cfg;
}
