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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_initial_process_reader.hpp>

namespace ams::kern {

    constexpr u32 InitialProcessBinaryMagic      = util::FourCC<'I','N','I','1'>::Code;
    constexpr size_t InitialProcessBinarySizeMax = 12_MB;

    struct InitialProcessBinaryHeader {
        u32 magic;
        u32 size;
        u32 num_processes;
        u32 reserved;
    };

    NOINLINE void CopyInitialProcessBinaryToKernelMemory();
    NOINLINE void CreateAndRunInitialProcesses();

    u64 GetInitialProcessIdMin();
    u64 GetInitialProcessIdMax();
    size_t GetInitialProcessesSecureMemorySize();

}
