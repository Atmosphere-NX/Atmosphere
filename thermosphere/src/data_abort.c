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
#include <stdio.h>
#include "data_abort.h"
#include "sysreg.h"
#include "debug_log.h"
#include "irq.h"
#include "vgic.h"

// Lower el

void dumpUnhandledDataAbort(DataAbortIss dabtIss, u64 far, const char *msg)
{
    char s1[64], s2[32], s3[64] = "";
    (void)s1; (void)s2; (void)s3;
    sprintf(s1, "Unhandled %s %s", msg , dabtIss.wnr ? "write" : "read");
    if (dabtIss.fnv) {
        sprintf(s2, "<unk>");
    } else {
        sprintf(s2, "%016lx", far);
    }

    if (dabtIss.isv) {
        sprintf(s3, ", size %u Rt=%u", BIT(dabtIss.sas), dabtIss.srt);
    }

    DEBUG("%s at %s%s\n", s1, s2, s3);
}

void handleLowerElDataAbortException(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
{
    DataAbortIss dabtIss;
    u32 iss = esr.iss;
    memcpy(&dabtIss, &iss, 4);

    u64 far = GET_SYSREG(far_el2);
    u64 farpg = far & ~0xFFFull;

    if (!dabtIss.isv || dabtIss.fnv) {
        dumpUnhandledDataAbort(dabtIss, far, "");
    }

    if (farpg == (uintptr_t)g_irqManager.gic.gicd) {
        handleVgicdMmio(frame, dabtIss, far & 0xFFF);
    } else if (farpg == (uintptr_t)g_irqManager.gic.gich) {
        dumpUnhandledDataAbort(dabtIss, far, "GICH");
    } else {
        dumpUnhandledDataAbort(dabtIss, far, "(fallback)");
    }

    // Skip instruction anyway?
    skipFaultingInstruction(frame, esr.il == 0 ? 2 : 4);
}
