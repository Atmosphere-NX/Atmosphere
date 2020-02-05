/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <cstdio>

#include "hvisor_gdb_defines_internal.hpp"
#include "hvisor_gdb_packet_data.hpp"

#include "../core_ctx.h"

namespace {
}

namespace ams::hvisor::gdb {

    int ConvertTidToCoreId(unsigned long tid)
    {
        switch (tid) {
            case ULONG_MAX:
                return -1;
            case 0:
                return currentCoreCtx->coreId;
            default:
                return currentCoreCtx->coreId - 1;
        }
    }

    std::optional<int> ParseConvertExactlyOneTid(std::string_view str)
    {
        if (str.size() == 2 && str[0] == '-' && str[1] == '1') {
            return -1;
        } else  {
            auto [n, tid] = ParseHexIntegerList<1>(str);
            if (n != 0 && tid < MAX_CORE + 1) {
                return ConvertTidToCoreId(tid);
            } else {
                return {};
            }
        }
    }

    // Hg<tid>, Hc<tid>
    GDB_DEFINE_HANDLER(SetThreadId)
    {
        if (!m_commandData.starts_with('g') && !m_commandData.starts_with('c')) {
            return ReplyErrno(EINVAL);
        }

        char kind = m_commandData[0];
        m_commandData.remove_prefix(1);

        auto coreIdOpt = ParseConvertExactlyOneTid(m_commandData);
        if (!coreIdOpt) {
            return ReplyErrno(EILSEQ);
        }

        int coreId = *coreIdOpt;
        if (kind == 'g') {
            if (coreId = -1) {
                return ReplyErrno(EINVAL);
            }
            m_selectedCoreId = coreId;
            MigrateRxIrq(m_selectedCoreId);
        } else {
            m_selectedCoreIdForContinuing = coreId;
        }

        return ReplyOk();
    }

    GDB_DEFINE_HANDLER(IsThreadAlive)
    {
        int coreId = ParseConvertExactlyOneTid(m_commandData).value_or(-1);
        if (coreId < 0) {
            return ReplyErrno(EILSEQ);
        }

        // Is the core off?
        if (m_attachedCoreList & BIT(coreId)) {
            return ReplyOk();
        } else {
            return ReplyErrno(ESRCH);
        }
    }

    GDB_DEFINE_QUERY_HANDLER(CurrentThreadId)
    {
        return SendFormattedPacket("QC%x", 1 + currentCoreCtx->coreId);
    }

    GDB_DEFINE_QUERY_HANDLER(fThreadInfo)
    {
        // We have made our GDB packet big enough to list all the thread ids (coreIds + 1 for each coreId)
        char *buf = GetInPlaceOutputBuffer();
        size_t n = 0;

        for (int coreId: util::BitsOf{m_attachedCoreList}) {
            n += sprintf(buf + n, "%lx,", 1u + coreId);
        }

        // Remove trailing comma
        --n;

        return SendStreamData(std::string_view{buf, n}, 0, n, true);
    }

    GDB_DEFINE_QUERY_HANDLER(sThreadInfo)
    {
        // We have made our GDB packet big enough to list all the thread ids (coreIds + 1 for each coreId) in fThreadInfo
        // Note: we assume GDB doesn't accept notifications during the sequence transfer...
        return SendPacket("l");
    }

    GDB_DEFINE_QUERY_HANDLER(ThreadEvents)
    {
        if (m_commandData.size() != 1) {
            return ReplyErrno(EILSEQ);
        }

        switch (m_commandData[0]) {
            case '0':
                m_catchThreadEvents = false;
                return ReplyOk();
            case '1':
                m_catchThreadEvents = true;
                return ReplyOk();
            default:
                return ReplyErrno(EILSEQ);
        }
    }

    GDB_DEFINE_QUERY_HANDLER(ThreadExtraInfo)
    {
        int coreId = ParseConvertExactlyOneTid(m_commandData).value_or(-1);
        if (coreId < 0) {
            return ReplyErrno(EILSEQ);
        }

        size_t n = sprintf(m_workBuffer, "TODO");

        return SendHexPacket(m_workBuffer, n);
    }
}
