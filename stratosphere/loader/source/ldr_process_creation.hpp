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