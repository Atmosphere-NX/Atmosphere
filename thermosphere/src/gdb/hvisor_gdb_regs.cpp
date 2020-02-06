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

/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "hvisor_gdb_defines_internal.hpp"
#include "hvisor_gdb_packet_data.hpp"

#include "../exceptions.h"
#include "../fpu.h"

namespace {

    auto GetRegisterPointerAndSize(unsigned long id, ExceptionStackFrame *frame, FpuRegisterCache *fpuRegCache)
    {
        void *outPtr = nullptr;
        size_t outSz = 0;

        switch (id) {
            case 0 ... 30:
                outPtr = &frame->x[id];
                outSz = 8;
                break;
            case 31:
                outPtr = exceptionGetSpPtr(frame);
                outSz = 8;
                break;
            case 32:
                outPtr = &frame->spsr_el2;
                outSz = 4;
                break;
            case 33 ... 64:
                outPtr = &fpuRegCache->q[id - 33];
                outSz = 16;
                break;
            case 65:
                outPtr = &fpuRegCache->fpsr;
                outSz = 4;
                break;
            case 66:
                outPtr = &fpuRegCache->fpcr;
                outSz = 4;
                break;
            default:
                __builtin_unreachable();
                break;
        }

        return std::tuple{outPtr, outSz};
    }

}

namespace ams::hvisor::gdb {

    // Note: GDB treats cpsr, fpsr, fpcr as 32-bit integers...
    GDB_DEFINE_HANDLER(ReadRegisters)
    {
        ENSURE(m_selectedCoreId == currentCoreCtx->coreId);

        ExceptionStackFrame *frame = currentCoreCtx->guestFrame;
        FpuRegisterCache *fpuRegCache = fpuReadRegisters();

        char *buf = GetInPlaceOutputBuffer();

        size_t n = 0;

        struct {
            u64 sp;
            u64 pc;
            u32 cpsr;
        } cpuSprs = {
            .sp = *exceptionGetSpPtr(frame),
            .pc = frame->elr_el2,
            .cpsr = static_cast<u32>(frame->spsr_el2),
        };

        u32 fpuSprs[2] = {
            static_cast<u32>(fpuRegCache->fpsr),
            static_cast<u32>(fpuRegCache->fpcr),
        };

        n += EncodeHex(buf + n, frame->x, sizeof(frame->x));
        n += EncodeHex(buf + n, &cpuSprs, 8+8+4);
        n += EncodeHex(buf + n, fpuRegCache->q, sizeof(fpuRegCache->q));
        n += EncodeHex(buf + n, fpuSprs, sizeof(fpuSprs));

        return SendPacket(std::string_view{buf, n});
    }

    GDB_DEFINE_HANDLER(WriteRegisters)
    {
        ENSURE(m_selectedCoreId == currentCoreCtx->coreId);

        ExceptionStackFrame *frame = currentCoreCtx->guestFrame;
        FpuRegisterCache *fpuRegCache = fpuGetRegisterCache();

        char *tmp = GetWorkBuffer();

        size_t n = 0;

        struct {
            u64 sp;
            u64 pc;
            u32 cpsr;
        } cpuSprs;

        u32 fpuSprs[2];

        struct {
            void *dst;
            size_t sz;
        } infos[4] = {
            { frame->x,         sizeof(frame->x)        },
            { &cpuSprs,         8+8+4                   },
            { fpuRegCache->q,   sizeof(fpuRegCache->q)  },
            { fpuSprs,          sizeof(fpuSprs)         },
        };

        // Parse & return on error
        for (const auto &info: infos) {
            // Fuck std::string_view.substr throwing exceptions
            if (DecodeHex(tmp + n, m_commandData.data(), info.sz) != info.sz) {
                return ReplyErrno(EILSEQ);
            }
            m_commandData.remove_prefix(2 * info.sz);
            n += info.sz;
        }

        // Copy. Note: we don't check if cpsr (spsr_el2) was modified to return to EL2...
        n = 0;
        for (const auto &info: infos) {
            std::copy(tmp + n, tmp + n + info.sz, info.dst);
            n += info.sz;
        }

        *exceptionGetSpPtr(frame) = cpuSprs.sp;
        frame->elr_el2 = cpuSprs.pc;
        frame->spsr_el2 = cpuSprs.cpsr;
        fpuRegCache->fpsr = fpuSprs[0];
        fpuRegCache->fpcr = fpuSprs[1];
        fpuCommitRegisters();

        return ReplyOk();
    }

    GDB_DEFINE_HANDLER(ReadRegister)
    {
        ENSURE(m_selectedCoreId == currentCoreCtx->coreId);

        ExceptionStackFrame *frame = currentCoreCtx->guestFrame;
        FpuRegisterCache *fpuRegCache = nullptr;

        auto [nread, gdbRegNum] = ParseHexIntegerList<1>(m_commandData);
        if (nread == 0) {
            return ReplyErrno(EILSEQ);
        }

        // Check the register number
        if (gdbRegNum >= 31 + 3 + 32 + 2) {
            return ReplyErrno(EINVAL);
        }

        if (gdbRegNum > 31 + 3) {
            // FPU register -- must read the FPU registers first
            fpuRegCache = fpuReadRegisters();
        }

        return std::apply(SendHexPacket, GetRegisterPointerAndSize(gdbRegNum, frame, fpuRegCache));
    }

    GDB_DEFINE_HANDLER(WriteRegister)
    {
        ENSURE(m_selectedCoreId == currentCoreCtx->coreId);

        char *tmp = GetWorkBuffer();
        ExceptionStackFrame *frame = currentCoreCtx->guestFrame;
        FpuRegisterCache *fpuRegCache = fpuGetRegisterCache();

        auto [nread, gdbRegNum] = ParseHexIntegerList<1>(m_commandData, '=');
        if (nread == 0) {
            return ReplyErrno(EILSEQ);
        }
        m_commandData.remove_prefix(nread);

        // Check the register number
        if (gdbRegNum >= 31 + 3 + 32 + 2) {
            return ReplyErrno(EINVAL);
        }

        auto [regPtr, sz] = GetRegisterPointerAndSize(gdbRegNum, frame, fpuRegCache);

        // Decode, check for errors
        if (m_commandData.size() != 2 * sz || DecodeHex(tmp, m_commandData) != sz) {
            return ReplyErrno(EILSEQ);
        }

        std::copy(tmp, tmp + sz, regPtr);

        if (gdbRegNum > 31 + 3) {
            // FPU register -- must commit the FPU registers
            fpuCommitRegisters();
        }

        return ReplyOk();
    }

}
