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

#pragma once

#include "utils.h"
#include "sysreg.h"
#include "platform/interrupt_config.h"

#define SECTONSECS      1000000000ull
#define SECTOUSECS      1000000ull
#define SECTOMSECS      1000ull

// All generic timers possibly defined in the Arm architecture:
// (suffix, time source suffix, level, irqid)
#define NS_PHYS_TIMER           (p,   p,   0,   GIC_IRQID_NS_PHYS_TIMER)
#define NS_VIRT_TIMER           (v,   v,   0,   GIC_IRQID_NS_VIRT_TIMER)
#define NS_PHYS_HYP_TIMER       (hp,  p,   2,   GIC_IRQID_NS_PHYS_HYP_TIMER)
#define NS_VIRT_HYP_TIMER       (hv,  v,   2,   GIC_IRQID_NS_VIRT_HYP_TIMER)
#define SEC_PHYS_TIMER          (ps,  p,   1,   GIC_IRQID_SEC_PHYS_TIMER)
#define SEC_PHYS_HYP_TIMER      (hps, p,   2,   GIC_IRQID_SEC_PHYS_HYP_TIMER)
#define SEC_VIRT_HYP_TIMER      (hvs, v,   2,   GIC_IRQID_SEC_VIRT_HYP_TIMER)

#define TIMER_IRQID_FIELDS(ign, ign2, ign3, id)           id
#define TIMER_COUNTER_REG_FIELDS(ign, ts, ign2, ign3)     cnt##ts##ct_el0
#define TIMER_CTL_REG_FIELDS(t, ign, el, ign2)            cnt##t##_ctl_el##el
#define TIMER_CVAL_REG_FIELDS(t, ign, el, ign2)           cnt##t##_cval_el##el
#define TIMER_TVAL_REG_FIELDS(t, ign, el, ign2)           cnt##t##_tval_el##el

#define TIMER_IRQID(name)                           EVAL(TIMER_IRQID_FIELDS name)
#define TIMER_COUNTER_REG(name)                     EVAL(TIMER_COUNTER_REG_FIELDS name)
#define TIMER_CTL_REG(name)                         EVAL(TIMER_CTL_REG_FIELDS name)
#define TIMER_CVAL_REG(name)                        EVAL(TIMER_CVAL_REG_FIELDS name)
#define TIMER_TVAL_REG(name)                        EVAL(TIMER_TVAL_REG_FIELDS name)

#define TIMER_CTL_ISTATUS                           BITL(2)
#define TIMER_CTL_IMASK                             BITL(1)
#define TIMER_CTL_ENABLE                            BITL(0)

#define CURRENT_TIMER                               NS_PHYS_HYP_TIMER

extern u64 g_timerFreq;

void timerInit(void);
void timerInterruptHandler(void);

static inline u64 timerGetSystemTick(void)
{
    return GET_SYSREG(TIMER_COUNTER_REG(CURRENT_TIMER));
}

static inline bool timerGetInterruptStatus(void)
{
    return (GET_SYSREG(TIMER_CTL_REG(CURRENT_TIMER)) & TIMER_CTL_ISTATUS) != 0;
}

static inline u64 timerGetSystemTimeNs(void)
{
    return timerGetSystemTick() * SECTONSECS / g_timerFreq;
}

static inline u64 timerGetSystemTimeMs(void)
{
    return timerGetSystemTick() * SECTOMSECS / g_timerFreq;
}

static inline void timerConfigure(bool enabled, bool interruptMasked)
{
    u64 ebit = enabled          ? TIMER_CTL_ENABLE : 0;
    u64 mbit = interruptMasked  ? TIMER_CTL_IMASK : 0;
    SET_SYSREG(TIMER_CTL_REG(CURRENT_TIMER), mbit | ebit);
}

static inline void timerSetTimeoutTicks(u64 ticks)
{
    SET_SYSREG(TIMER_CVAL_REG(CURRENT_TIMER), timerGetSystemTick() + ticks);
    timerConfigure(true, false);
}

static inline void timerSetTimeoutNs(u64 ns)
{
    timerSetTimeoutTicks(ns * g_timerFreq / SECTONSECS);
}

static inline void timerSetTimeoutMs(u64 ms)
{
    timerSetTimeoutTicks(ms * g_timerFreq / SECTOMSECS);
}

static inline void timerSetTimeoutUs(u64 us)
{
    timerSetTimeoutTicks(us * g_timerFreq / SECTOUSECS);
}

void timerWaitUsecs(u64 us);
