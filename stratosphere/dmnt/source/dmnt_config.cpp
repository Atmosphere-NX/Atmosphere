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
 
#include <switch.h>
#include <string.h>
#include <stratosphere.hpp>

#include "dmnt_hid.hpp"
#include "dmnt_config.hpp"
#include "ini.h"

/* Support variables. */
static OverrideKey g_default_cheat_enable_key = {
    .key_combination = KEY_L,
    .override_by_default = true
};

/* Static buffer for loader.ini contents at runtime. */
static char g_config_ini_data[0x800];

static OverrideKey ParseOverrideKey(const char *value) {
    OverrideKey cfg;
    
    /* Parse on by default. */
    if (value[0] == '!') {
        cfg.override_by_default = true;
        value++;
    } else {
        cfg.override_by_default = false;
    }
    
    /* Parse key combination. */
    if (strcasecmp(value, "A") == 0) {
        cfg.key_combination = KEY_A;
    } else if (strcasecmp(value, "B") == 0) {
        cfg.key_combination = KEY_B;
    } else if (strcasecmp(value, "X") == 0) {
        cfg.key_combination = KEY_X;
    } else if (strcasecmp(value, "Y") == 0) {
        cfg.key_combination = KEY_Y;
    } else if (strcasecmp(value, "LS") == 0) {
        cfg.key_combination = KEY_LSTICK;
    } else if (strcasecmp(value, "RS") == 0) {
        cfg.key_combination = KEY_RSTICK;
    } else if (strcasecmp(value, "L") == 0) {
        cfg.key_combination = KEY_L;
    } else if (strcasecmp(value, "R") == 0) {
        cfg.key_combination = KEY_R;
    } else if (strcasecmp(value, "ZL") == 0) {
        cfg.key_combination = KEY_ZL;
    } else if (strcasecmp(value, "ZR") == 0) {
        cfg.key_combination = KEY_ZR;
    } else if (strcasecmp(value, "PLUS") == 0) {
        cfg.key_combination = KEY_PLUS;
    } else if (strcasecmp(value, "MINUS") == 0) {
        cfg.key_combination = KEY_MINUS;
    } else if (strcasecmp(value, "DLEFT") == 0) {
        cfg.key_combination = KEY_DLEFT;
    } else if (strcasecmp(value, "DUP") == 0) {
        cfg.key_combination = KEY_DUP;
    } else if (strcasecmp(value, "DRIGHT") == 0) {
        cfg.key_combination = KEY_DRIGHT;
    } else if (strcasecmp(value, "DDOWN") == 0) {
        cfg.key_combination = KEY_DDOWN;
    } else if (strcasecmp(value, "SL") == 0) {
        cfg.key_combination = KEY_SL;
    } else if (strcasecmp(value, "SR") == 0) {
        cfg.key_combination = KEY_SR;
    } else {
        cfg.key_combination = 0;
    }
    
    return cfg;
}

static int DmntIniHandler(void *user, const char *section, const char *name, const char *value) {
    /* Taken and modified, with love, from Rajkosto's implementation. */
    if (strcasecmp(section, "default_config") == 0) {
        if (strcasecmp(name, "cheat_enable_key") == 0) {
            g_default_cheat_enable_key = ParseOverrideKey(value);
        }
    } else {
        return 0;
    }
    return 1;
}

static int DmntTitleSpecificIniHandler(void *user, const char *section, const char *name, const char *value) {
    /* We'll output an override key when relevant. */
    OverrideKey *user_cfg = reinterpret_cast<OverrideKey *>(user);
    
    if (strcasecmp(section, "override_config") == 0) {
        if (strcasecmp(name, "cheat_enable_key") == 0) {
            *user_cfg = ParseOverrideKey(value);
        }
    } else {
        return 0;
    }
    return 1;
}

void DmntConfigManager::RefreshConfiguration() {
    FILE *config = fopen("sdmc:/atmosphere/loader.ini", "r");
    if (config == NULL) {
        return;
    }
    
    memset(g_config_ini_data, 0, sizeof(g_config_ini_data));
    fread(g_config_ini_data, 1, sizeof(g_config_ini_data) - 1, config);
    fclose(config);
    
    ini_parse_string(g_config_ini_data, DmntIniHandler, NULL);
}

OverrideKey DmntConfigManager::GetTitleCheatEnableKey(u64 tid) {
    OverrideKey cfg = g_default_cheat_enable_key;
    char path[FS_MAX_PATH+1] = {0};
    snprintf(path, FS_MAX_PATH, "sdmc:/atmosphere/titles/%016lx/config.ini", tid); 
    
    
    FILE *config = fopen(path, "r");
    if (config != NULL) {
        ON_SCOPE_EXIT { fclose(config); };

        /* Parse current title ini. */
        ini_parse_file(config, DmntTitleSpecificIniHandler, &cfg);
    }
    
    return cfg;
}

static bool HasOverrideKey(OverrideKey *cfg) {
    u64 kDown = 0;
    bool keys_triggered = (R_SUCCEEDED(HidManagement::GetKeysDown(&kDown)) && ((kDown & cfg->key_combination) != 0));
    return (cfg->override_by_default ^ keys_triggered);
}

bool DmntConfigManager::HasCheatEnableButton(u64 tid) {
    /* Unconditionally refresh loader.ini contents. */
    RefreshConfiguration();
    
    OverrideKey title_cfg = GetTitleCheatEnableKey(tid);
    return HasOverrideKey(&title_cfg);
}
