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

#include "bct0.h"

#include "lib/ini.h"

#include <string.h>

#define CONFIG_LOG_LEVEL_KEY "log_level"

#define STAGE2_NAME_KEY "stage2_path"
#define STAGE2_MTC_NAME_KEY "stage2_mtc_path"
#define STAGE2_ADDRESS_KEY "stage2_addr"
#define STAGE2_ENTRYPOINT_KEY "stage2_entrypoint"

#define FASTBOOT_FORCE_ENABLE_KEY "force_enable"
#define FASTBOOT_BUTTON_TIMEOUT_KEY "button_timeout_ms"

static int bct0_ini_handler(void *user, const char *section, const char *name, const char *value) {
	bct0_t *bct0 = (bct0_t*) user;
	
    if (strcmp(section, "config") == 0) {
        if (strcmp(name, CONFIG_LOG_LEVEL_KEY) == 0) {
            int log_level = bct0->log_level;
            sscanf(value, "%d", &log_level);
            bct0->log_level = (ScreenLogLevel) log_level;
        } else {
            return 0;
        }
    } else if (strcmp(section, "stage1") == 0) {
	    if (strcmp(name, STAGE2_NAME_KEY) == 0) {
		    strncpy(bct0->stage2_path, value, sizeof(bct0->stage2_path) - 1);
		    bct0->stage2_path[sizeof(bct0->stage2_path) - 1] = '\0';
	    } else if (strcmp(name, STAGE2_MTC_NAME_KEY) == 0) {
		    strncpy(bct0->stage2_mtc_path, value, sizeof(bct0->stage2_mtc_path) - 1);
		    bct0->stage2_mtc_path[sizeof(bct0->stage2_mtc_path) - 1] = '\0';
	    } else if (strcmp(name, STAGE2_ADDRESS_KEY) == 0) {
            /* Read in load address as a hex string. */
		    uintptr_t x;
            sscanf(value, "%x", &x);
            bct0->stage2_load_address = x;
            if (bct0->stage2_entrypoint == 0) {
                bct0->stage2_entrypoint = bct0->stage2_load_address;
            }
	    } else if (strcmp(name, STAGE2_ENTRYPOINT_KEY) == 0) {
            /* Read in entrypoint as a hex string. */
		    uintptr_t x;
            sscanf(value, "%x", &x);
            bct0->stage2_entrypoint = x;
	    } else {
		    return 0;
	    }
    } else if (strcmp(section, "fastboot") == 0) {
	    if (strcmp(name, FASTBOOT_FORCE_ENABLE_KEY) == 0) {
		    int tmp = 0;
		    sscanf(value, "%d", &tmp);
		    bct0->fastboot_force_enable = (tmp != 0);
	    } else if (strcmp(name, FASTBOOT_BUTTON_TIMEOUT_KEY) == 0) {
		    int tmp = 0;
		    sscanf(value, "%d", &tmp);
		    bct0->fastboot_button_timeout = tmp;
	    } else {
		    return 0;
	    }
    } else {
        return 0;
    }
    
    return 1;
}

int bct0_parse(const char *ini, bct0_t *out) {
	/* Initialize some default configuration. */
	out->log_level = SCREEN_LOG_LEVEL_NONE;

	strcpy(out->stage2_path, "atmosphere/fusee-secondary.bin");
	strcpy(out->stage2_mtc_path, "atmosphere/fusee-mtc.bin");
	out->stage2_load_address = 0xf0000000;
	out->stage2_entrypoint = 0xf0000000;

	out->fastboot_force_enable = false;
	out->fastboot_button_timeout = 3000;
	
	return ini_parse_string(ini, bct0_ini_handler, out);
}
