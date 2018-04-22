#pragma once
#include <switch.h>

#include "ldr_registration.hpp"
#include "ldr_launch_queue.hpp"

/* Utilities for Process Creation, for Loader. */

class ProcessCreation {
    public:
        static Result CreateProcess(Handle *out_process_h, u64 index, char *nca_path, LaunchQueue::LaunchItem *launch_item, u64 flags, Handle reslimit_h);
};