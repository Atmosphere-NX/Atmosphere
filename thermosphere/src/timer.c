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

#include "timer.h"
#include "irq.h"

u64 g_timerFreq = 0;

void timerInit(void)
{
    timerConfigure(false, false);
    if (currentCoreCtx->isBootCore) {
        g_timerFreq = GET_SYSREG(cntfrq_el0);
    }
}

void timerInterruptHandler(void)
{
    // Disable timer programming until reprogrammed
    timerConfigure(false, false);

    // For fun
    DEBUG("EL2 [core %d]: Timer interrupt at %lums\n", (int)currentCoreCtx->coreId, timerGetSystemTimeMs());
    timerSetTimeoutMs(1000);
}
