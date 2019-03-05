/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch.h>

#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"
#include "ldr_npdm.hpp"

/* Utilities for Process Creation, for Loader. */

class ProcessCreation {
    public:
        struct ProcessInfo {
            u8 name[12];
            u32 process_category;
            u64 title_id;
            u64 code_addr;
            u32 code_num_pages;
            u32 process_flags;
            Handle reslimit_h;
            u32 system_resource_num_pages;
        };
        static Result InitializeProcessInfo(NpdmUtils::NpdmInfo *npdm, Handle reslimit_h, u64 arg_flags, ProcessInfo *out_proc_info);
        static Result CreateProcess(Handle *out_process_h, u64 index, char *nca_path, LaunchQueue::LaunchItem *launch_item, u64 arg_flags, Handle reslimit_h);
};