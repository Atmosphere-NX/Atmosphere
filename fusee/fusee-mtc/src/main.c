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

#include <string.h>
#include "mtc.h"
#include "stage2.h"
#include "../../../fusee/common/display/video_fb.h"

static void *g_framebuffer;
static __attribute__((__aligned__(0x200))) stage2_mtc_args_t g_mtc_args_store;
static stage2_mtc_args_t *g_mtc_args;

/* Allow for main(int argc, void **argv) signature. */
#pragma GCC diagnostic ignored "-Wmain"

int main(int argc, void **argv) {
    ScreenLogLevel log_level = SCREEN_LOG_LEVEL_NONE;
    
    /* Check argc. */
    if (argc != MTC_ARGC) {
        return 1;
    }
    
    /* Extract arguments from argv. */
    g_mtc_args = &g_mtc_args_store;
    memcpy(g_mtc_args, (stage2_mtc_args_t *)argv[MTC_ARGV_ARGUMENT_STRUCT], sizeof(*g_mtc_args));
    log_level = g_mtc_args->log_level;
    
    /* Override the global logging level. */
    log_set_log_level(log_level);
    
    if (log_level != SCREEN_LOG_LEVEL_NONE) {
        /* Set framebuffer address. */
        g_framebuffer = (void *)0xC0000000;
    
        /* Zero-fill the framebuffer and register it as printk provider. */
        video_init(g_framebuffer);
    }
    
    /* Train DRAM. */
    train_dram();

    return 0;
}
