/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

// Lots of code from:
/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "hvisor_gdb_defines_internal.hpp"
#include "hvisor_gdb_packet_data.hpp"

#include "../hvisor_hw_breakpoint_manager.hpp"
#include "../hvisor_sw_breakpoint_manager.hpp"
#include "../hvisor_watchpoint_manager.hpp"

#include "../hvisor_fpu_register_cache.hpp"

#include "../debug_manager.h"

namespace {

    TEMPORARY char g_gdbWorkBuffer[GDB_WORK_BUF_LEN];
    TEMPORARY char g_gdbBuffer[GDB_BUF_LEN + 4 + 1];

}

namespace ams::hvisor::gdb {

    void Context::Disconnect()
    {
        Detach();
        auto *iface = m_transportInterface;

        *this = {};
        m_transportInterface = iface;
    }

    void Context::Initialize(TransportInterfaceType ifaceType, u32 ifaceId, u32 ifaceFlags)
    {
        m_workBuffer = g_gdbWorkBuffer;
        m_buffer = g_gdbBuffer;
        /*m_transportInterface = transportInterfaceCreate(
            ifaceType,
            ifaceId,
            ifaceFlags,
            GDB_ReceiveDataCallback,
            GDB_ProcessDataCallback,
            ctx
        );*/
    }

    void Context::Attach()
    {
        // TODO: move the debug traps enable here?

        m_attachedCoreList = CoreContext::GetActiveCoreMask();

        // We're in full-stop mode at this point
        // Break cores, but don't send the debug event (it will be fetched with '?')
        // Initialize lastDebugEvent

        debugManagerSetReportingEnabled(true);
        m_sendOwnDebugEventDisallowed = true;

        BreakAllCores();

        DebugEventInfo *info = debugManagerGetDebugEvent(currentCoreCtx->GetCoreId());
        info->preprocessed = true;
        info->handled = true;
        m_lastDebugEvent = info;

        m_state = State::Attached;

        m_sendOwnDebugEventDisallowed = false;
    }

    void Context::Detach()
    {
        WatchpointManager::GetInstance().RemoveAll();
        HwBreakpointManager::GetInstance().RemoveAll();
        SwBreakpointManager::GetInstance().RemoveAll(true);

        // Reports to gdb are prevented because of "detaching" state?

        // TODO: disable debug traps

        m_currentHioRequestTargetAddr = 0;
        memset(&m_currentHioRequest, 0, sizeof(PackedGdbHioRequest));

        debugManagerSetReportingEnabled(false);
        debugManagerContinueCores(CoreContext::GetActiveCoreMask());
    }

    void Context::MigrateRxIrq(u32 coreId) const
    {
        FpuRegisterCache::GetInstance().CleanInvalidate();
        //transportInterfaceSetInterruptAffinity(ctx->transportInterface, BIT(coreId));
    }

    GDB_DEFINE_HANDLER(Unsupported)
    {
        return ReplyEmpty();
    }

#define COMMAND_CASE(letter, method) case letter: return GDB_HANDLER(method)();

    int Context::ProcessPacket()
    {
        m_commandLetter = m_commandData[0];
        m_commandData.remove_prefix(1);

        switch (m_commandLetter) {
            COMMAND_CASE('?', GetStopReason)
            //COMMAND_CASE('c', ContinueOrStepDeprecated)
            //COMMAND_CASE('C', ContinueOrStepDeprecated)
            COMMAND_CASE('D', Detach)
            COMMAND_CASE('F', HioReply)
            COMMAND_CASE('g', ReadRegisters)
            COMMAND_CASE('G', WriteRegisters)
            COMMAND_CASE('H', SetThreadId)
            COMMAND_CASE('k', Kill)
            COMMAND_CASE('m', ReadMemory)
            COMMAND_CASE('M', WriteMemory)
            COMMAND_CASE('p', ReadRegister)
            COMMAND_CASE('P', WriteRegister)
            COMMAND_CASE('q', Query)
            COMMAND_CASE('Q', Query)
            //COMMAND_CASE('s', ContinueOrStepDeprecated)
            //COMMAND_CASE('S', ContinueOrStepDeprecated)
            COMMAND_CASE('T', IsThreadAlive)
            COMMAND_CASE('v', VerboseCommand)
            COMMAND_CASE('X', WriteMemoryRaw)
            COMMAND_CASE('z', ToggleStopPoint)
            COMMAND_CASE('Z', ToggleStopPoint)

            default:
                return HandleUnsupported();
        }
    }

#undef COMMAND_CASE

    /*
    static const struct{
        char command;
        GDBCommandHandler handler;
    } gdbCommandHandlers[] = {
        { '?', GDB_HANDLER(GetStopReason) },
        //{ '!', GDB_HANDLER(EnableExtendedMode) }, // note: stubbed
        //{ 'c', GDB_HANDLER(ContinueOrStepDeprecated) },
        //{ 'C', GDB_HANDLER(ContinueOrStepDeprecated) },
        { 'D', GDB_HANDLER(Detach) },
        { 'F', GDB_HANDLER(HioReply) },
        { 'g', GDB_HANDLER(ReadRegisters) },
        { 'G', GDB_HANDLER(WriteRegisters) },
        { 'H', GDB_HANDLER(SetThreadId) },
        { 'k', GDB_HANDLER(Kill) },
        { 'm', GDB_HANDLER(ReadMemory) },
        { 'M', GDB_HANDLER(WriteMemory) },
        { 'p', GDB_HANDLER(ReadRegister) },
        { 'P', GDB_HANDLER(WriteRegister) },
        { 'q', GDB_HANDLER(ReadQuery) },
        { 'Q', GDB_HANDLER(WriteQuery) },
        //{ 's', GDB_HANDLER(ContinueOrStepDeprecated) },
        //{ 'S', GDB_HANDLER(ContinueOrStepDeprecated) },
        { 'T', GDB_HANDLER(IsThreadAlive) },
        { 'v', GDB_HANDLER(VerboseCommand) },
        { 'X', GDB_HANDLER(WriteMemoryRaw) },
        { 'z', GDB_HANDLER(ToggleStopPoint) },
        { 'Z', GDB_HANDLER(ToggleStopPoint) },
    };

    static inline GDBCommandHandler GDB_GetCommandHandler(char command)
    {
        static const u32 nbHandlers = sizeof(gdbCommandHandlers) / sizeof(gdbCommandHandlers[0]);

        size_t i;
        for (i = 0; i < nbHandlers && gdbCommandHandlers[i].command != command; i++);

        return i < nbHandlers ? gdbCommandHandlers[i].handler : GDB_HANDLER(Unsupported);
    }

    static int GDB_ProcessPacket(GDBContext *ctx, size_t len)
    {
        int ret;

        ENSURE(ctx->state != GDB_STATE_DISCONNECTED);

        // Handle the packet...
        if (ctx->buffer[0] == '\x03') {
            GDB_BreakAllCores(ctx);
            ret = 0;
        } else {
            GDBCommandHandler handler = GDB_GetCommandHandler(ctx->buffer[1]);
            ctx->commandData = ctx->buffer + 2;
            ret = handler(ctx);
        }


        // State changes...
        if (ctx->state == GDB_STATE_DETACHING) {
            return -1;
        }

        return ret;
    }

    static size_t GDB_ReceiveDataCallback(TransportInterface *iface, void *ctxVoid)
    {
        return (size_t)GDB_ReceivePacket((GDBContext *)ctxVoid);
    }

    static void GDB_ProcessDataCallback(TransportInterface *iface, void *ctxVoid, size_t sz)
    {
        int r = (int)sz;
        GDBContext *ctx = (GDBContext *)ctxVoid;

        if (r == -1) {
            // Not sure if GDB has something to forcefully close connections over UART...
            char c = '\x04'; // ctrl-D
            transportInterfaceWriteData(iface, &c, 1);
            GDB_Disconnect(ctx);
        }

        r = GDB_ProcessPacket(ctx, sz);
        if (r == -1) {
            GDB_Disconnect(ctx);
        }
    }

    void GDB_AcquireContext(GDBContext *ctx)
    {
        transportInterfaceAcquire(ctx->transportInterface);
    }

    void GDB_ReleaseContext(GDBContext *ctx)
    {
        transportInterfaceRelease(ctx->transportInterface);
    }
*/
}
