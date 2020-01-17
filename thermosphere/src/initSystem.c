/*
 * Copyright (c) 2019 Atmosphère-NX
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
#include "core_ctx.h"
#include "platform/stage2.h"
#include "platform/devices.h"
#include "sysreg.h"
#include "utils.h"

// BSS includes real bss and tmp bss
extern u8 __bss_start__[], __real_bss_end__[], __bss_end__[];

static void initSysregs(void)
{
    // Set system to sane defaults, aarch64 for el1, mmu&caches initially disabled for EL1, etc.
    SET_SYSREG(hcr_el2,         0x80000000);
    SET_SYSREG(dacr32_el2,      0xFFFFFFFF);    // unused
    SET_SYSREG(sctlr_el1,       0x00C50838);

    SET_SYSREG(mdcr_el2,        0x00000000);
    SET_SYSREG(mdscr_el1,       0x00000000);

    // Timer stuff
    SET_SYSREG(cntvoff_el2,     0x00000000);
    SET_SYSREG(cnthctl_el2,     0x00000003);    // Don't trap anything for now; event streams disabled
    SET_SYSREG(cntkctl_el1,     0x00000003);    // Don't trap anything for now; event streams disabled
    SET_SYSREG(cntp_ctl_el0,    0x00000000);
    SET_SYSREG(cntv_ctl_el0,    0x00000000);

    __dsb();
    __isb();
}

void initSystem(u32 coreId, bool isBootCore, u64 argument)
{
    coreCtxInit(coreId, isBootCore, argument);
    initSysregs();

    if (isBootCore) {
        if (!currentCoreCtx->warmboot) {
            memset(__bss_start__, 0, __real_bss_end__ - __bss_start__);
        }

        memset(__real_bss_end__, 0, __bss_end__ - __real_bss_end__);
    }

    stage2ConfigureAndEnable();
    if (isBootCore) {
        devicesMapAllExtra();
    }
}
