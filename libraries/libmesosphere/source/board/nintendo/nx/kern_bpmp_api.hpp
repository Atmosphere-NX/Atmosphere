/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License
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
#include <mesosphere.hpp>

/* Message Flags */
#define BPMP_MSG_DO_ACK            (1 << 0)
#define BPMP_MSG_RING_DOORBELL  (1 << 1)

/* Messages */
#define MRQ_PING            0
#define MRQ_ENABLE_SUSPEND  17
#define MRQ_CPU_PMIC_SELECT 28

/* BPMP Power states. */
#define TEGRA_BPMP_PM_CC1 9
#define TEGRA_BPMP_PM_CC4 12
#define TEGRA_BPMP_PM_CC6 14
#define TEGRA_BPMP_PM_CC7 15
#define TEGRA_BPMP_PM_SC1 17
#define TEGRA_BPMP_PM_SC2 18
#define TEGRA_BPMP_PM_SC3 19
#define TEGRA_BPMP_PM_SC4 20
#define TEGRA_BPMP_PM_SC7 23

/* Channel states. */
#define CH_MASK(ch) (0x3u << ((ch) * 2))
#define SL_SIGL(ch) (0x0u << ((ch) * 2))
#define SL_QUED(ch) (0x1u << ((ch) * 2))
#define MA_FREE(ch) (0x2u << ((ch) * 2))
#define MA_ACKD(ch) (0x3u << ((ch) * 2))

constexpr inline int MessageSize = 0x80;
constexpr inline int MessageDataSizeMax = 0x78;

struct MailboxData {
    s32 code;
    s32 flags;
    u8 data[MessageDataSizeMax];
};

static_assert(ams::util::is_pod<MailboxData>::value);
static_assert(sizeof(MailboxData) == MessageSize);

struct ChannelData {
    MailboxData *ib;
    MailboxData *ob;
};


