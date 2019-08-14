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
#include "data_abort.h"
#include "sysreg.h"
#include "debug_log.h"
#include "irq.h"

// Lower el

void dumpUnhandledDataAbort(DataAbortIss dabtIss, u64 far, const char *msg)
{
    DEBUG("Unhandled");
    DEBUG(" %s ", msg);
    DEBUG("%s at ", dabtIss.wnr ? "write" : "read");
    if (dabtIss.fnv) {
        DEBUG("<unk>");
    } else {
        DEBUG("%016llx", far);
    }

    if (dabtIss.isv) {
        DEBUG(" size %u Rt=%u", BIT(dabtIss.sas), dabtIss.srt);
    }

    DEBUG("\n");
}

void handleLowerElDataAbortException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    DataAbortIss dabtIss;
    memcpy(&dabtIss, &esr, 4);

    u64 far = GET_SYSREG(far_el2);
    u64 farpg = far & ~0xFFFull;

    if (!dabtIss.isv || dabtIss.fnv) {
        dumpUnhandledDataAbort(dabtIss, far, "");
    }

    // TODO

    if (farpg == (uintptr_t)g_irqManager.gic.gicd) {
        // TODO
    } else if (farpg == (uintptr_t)g_irqManager.gic.gich) {
        dumpUnhandledDataAbort(dabtIss, far, "GICH");
    } else {
        dumpUnhandledDataAbort(dabtIss, far, "(fallback)");
    }
}